// This file is part of the FidelityFX Super Resolution 3.0 Unreal Engine Plugin.
//
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "FFXFSR3TemporalUpscaler.h"
#include "FFXFSR3TemporalUpscalerProxy.h"
#include "FFXFSR3TemporalUpscaling.h"
#include "FFXFSR3Include.h"
#include "FFXFSR3TemporalUpscalerHistory.h"
#include "SceneTextureParameters.h"
#include "TranslucentRendering.h"
#include "ScenePrivate.h"
#include "LogFFXFSR3.h"
#include "LegacyScreenPercentageDriver.h"
#include "PlanarReflectionSceneProxy.h"
#include "ScreenSpaceRayTracing.h"
#include "Serialization/MemoryImage.h"
#include "Serialization/MemoryLayout.h"
#include "FXSystem.h"
#include "PostProcess/SceneRenderTargets.h"
#include "HAL/IConsoleManager.h"
#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "DataDrivenShaderPlatformInfo.h"
#endif
#if UE_VERSION_AT_LEAST(5, 3, 0)
#include "FXRenderingUtils.h"
#endif
#include "FFXFSR3Settings.h"

//------------------------------------------------------------------------------------------------------
// GPU statistics for the FSR3 passes.
//------------------------------------------------------------------------------------------------------
DECLARE_GPU_STAT(FidelityFXSuperResolution3Pass)
DECLARE_GPU_STAT_NAMED(FidelityFXFSR3Dispatch, TEXT("FidelityFX FSR3 Dispatch"));

//------------------------------------------------------------------------------------------------------
// Quality mode definitions
//------------------------------------------------------------------------------------------------------
static const FfxFsr3UpscalerQualityMode LowestResolutionQualityMode = FFX_FSR3UPSCALER_QUALITY_MODE_ULTRA_PERFORMANCE;
static const FfxFsr3UpscalerQualityMode HighestResolutionQualityMode = FFX_FSR3UPSCALER_QUALITY_MODE_QUALITY;

//------------------------------------------------------------------------------------------------------
// To enforce quality modes we have to save the existing screen percentage so we can restore it later.
//------------------------------------------------------------------------------------------------------
float FFXFSR3TemporalUpscaler::SavedScreenPercentage{ 100.0f };

//------------------------------------------------------------------------------------------------------
// Unreal shader to convert from the Velocity texture format to the Motion Vectors used by FSR3.
//------------------------------------------------------------------------------------------------------
class FFXFSR3ConvertVelocityCS : public FGlobalShader
{
public:
	static const int ThreadgroupSizeX = 8;
	static const int ThreadgroupSizeY = 8;
	static const int ThreadgroupSizeZ = 1;

	DECLARE_GLOBAL_SHADER(FFXFSR3ConvertVelocityCS);
	SHADER_USE_PARAMETER_STRUCT(FFXFSR3ConvertVelocityCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		RDG_TEXTURE_ACCESS(DepthTexture, ERHIAccess::SRVCompute)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputDepth)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputVelocity)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), ThreadgroupSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), ThreadgroupSizeY);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEZ"), ThreadgroupSizeZ);
		OutEnvironment.SetDefine(TEXT("COMPUTE_SHADER"), 1);
		OutEnvironment.SetDefine(TEXT("UNREAL_ENGINE_MAJOR_VERSION"), ENGINE_MAJOR_VERSION);
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXFSR3ConvertVelocityCS, "/Plugin/FSR3/Private/PostProcessFFX_FSR3ConvertVelocity.usf", "MainCS", SF_Compute);

//------------------------------------------------------------------------------------------------------
// Unreal shader to generate mask textures for translucency & reactivity to be used in FSR3.
//------------------------------------------------------------------------------------------------------
class FFXFSR3CreateReactiveMaskCS : public FGlobalShader
{
public:
	static const int ThreadgroupSizeX = 8;
	static const int ThreadgroupSizeY = 8;
	static const int ThreadgroupSizeZ = 1;

	DECLARE_GLOBAL_SHADER(FFXFSR3CreateReactiveMaskCS);
	SHADER_USE_PARAMETER_STRUCT(FFXFSR3CreateReactiveMaskCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		RDG_TEXTURE_ACCESS(DepthTexture, ERHIAccess::SRVCompute)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, GBufferB)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, GBufferD)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, ReflectionTexture)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputDepth)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneColor)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneColorPreAlpha)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, LumenSpecular)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputVelocity)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, ReactiveMask)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, CompositeMask)
		SHADER_PARAMETER_SAMPLER(SamplerState, Sampler)
		SHADER_PARAMETER(float, FurthestReflectionCaptureDistance)
		SHADER_PARAMETER(float, ReactiveMaskReflectionScale)
		SHADER_PARAMETER(float, ReactiveMaskRoughnessScale)
		SHADER_PARAMETER(float, ReactiveMaskRoughnessBias)
		SHADER_PARAMETER(float, ReactiveMaskReflectionLumaBias)
		SHADER_PARAMETER(float, ReactiveHistoryTranslucencyBias)
		SHADER_PARAMETER(float, ReactiveHistoryTranslucencyLumaBias)
		SHADER_PARAMETER(float, ReactiveMaskTranslucencyBias)
		SHADER_PARAMETER(float, ReactiveMaskTranslucencyLumaBias)
		SHADER_PARAMETER(float, ReactiveMaskTranslucencyMaxDistance)
		SHADER_PARAMETER(float, ForceLitReactiveValue)
		SHADER_PARAMETER(uint32, ReactiveShadingModelID)
		SHADER_PARAMETER(uint32, LumenSpecularCurrentFrame)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), ThreadgroupSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), ThreadgroupSizeY);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEZ"), ThreadgroupSizeZ);
		OutEnvironment.SetDefine(TEXT("COMPUTE_SHADER"), 1);
		OutEnvironment.SetDefine(TEXT("UNREAL_ENGINE_MAJOR_VERSION"), ENGINE_MAJOR_VERSION);
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXFSR3CreateReactiveMaskCS, "/Plugin/FSR3/Private/PostProcessFFX_FSR3CreateReactiveMask.usf", "MainCS", SF_Compute);

//------------------------------------------------------------------------------------------------------
// Unreal shader to blend hair which is dithered and FSR3 doesn't handle that well.
//------------------------------------------------------------------------------------------------------
class FFXFSR3DeDitherCS : public FGlobalShader
{
public:
	static const int ThreadgroupSizeX = 8;
	static const int ThreadgroupSizeY = 8;
	static const int ThreadgroupSizeZ = 1;

	DECLARE_GLOBAL_SHADER(FFXFSR3DeDitherCS);
	SHADER_USE_PARAMETER_STRUCT(FFXFSR3DeDitherCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, GBufferB)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneColor)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, BlendSceneColor)
		SHADER_PARAMETER(uint32, FullDeDither)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), ThreadgroupSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), ThreadgroupSizeY);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEZ"), ThreadgroupSizeZ);
		OutEnvironment.SetDefine(TEXT("COMPUTE_SHADER"), 1);
		OutEnvironment.SetDefine(TEXT("UNREAL_ENGINE_MAJOR_VERSION"), ENGINE_MAJOR_VERSION);
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXFSR3DeDitherCS, "/Plugin/FSR3/Private/PostProcessFFX_FSR3DeDither.usf", "MainCS", SF_Compute);

#if UE_VERSION_AT_LEAST(5, 2, 0)
//------------------------------------------------------------------------------------------------------
// Unreal shader to copy EyeAdaptationBuffer data to Exposure texture.
//------------------------------------------------------------------------------------------------------
class FFXFSR3CopyExposureCS : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FFXFSR3CopyExposureCS);
	SHADER_USE_PARAMETER_STRUCT(FFXFSR3CopyExposureCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>, EyeAdaptationBuffer)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, ExposureTexture)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("COMPUTE_SHADER"), 1);
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXFSR3CopyExposureCS, "/Plugin/FSR3/Private/PostProcessFFX_FSR3CopyExposure.usf", "MainCS", SF_Compute);
#endif // UE_VERSION_AT_LEAST(5, 2, 0)

//------------------------------------------------------------------------------------------------------
// Map of ScreenSpaceReflection shaders so that FSR3 can swizzle the shaders inside the GlobalShaderMap.
// This is necessary so that FSR3 can access the ScreenSpaceReflection data through the ReflectionDenoiser plugin without changing their appearance. 
//------------------------------------------------------------------------------------------------------
struct FFXFSR3ShaderMapSwapState
{
	const FGlobalShaderMapContent* Content;
	bool bSwapped;

	static const FFXFSR3ShaderMapSwapState Default;
};
const FFXFSR3ShaderMapSwapState FFXFSR3ShaderMapSwapState::Default = { nullptr, false };

//------------------------------------------------------------------------------------------------------
// This object isn't conceptually linked to individual TemporalUpscalers.  it contains information about the state of an object in the global shader map,
// and that information needs to be consistent across all TemporalUpscalers that might currently exist.
//------------------------------------------------------------------------------------------------------
static TMap<class FGlobalShaderMap*, FFXFSR3ShaderMapSwapState> SSRShaderMapSwapState;

//------------------------------------------------------------------------------------------------------
// The FFXFSR3ShaderMapContent structure allows access to the internals of FShaderMapContent so that FSR3 can swap the Default & Denoised variants of ScreenSpaceReflections.
//------------------------------------------------------------------------------------------------------
class FFXFSR3ShaderMapContent
{
public:
	DECLARE_TYPE_LAYOUT(FFXFSR3ShaderMapContent, NonVirtual);

	using FMemoryImageHashTable = THashTable<FMemoryImageAllocator>;

	LAYOUT_FIELD(FMemoryImageHashTable, ShaderHash);
	LAYOUT_FIELD(TMemoryImageArray<FHashedName>, ShaderTypes);
	LAYOUT_FIELD(TMemoryImageArray<int32>, ShaderPermutations);
	LAYOUT_FIELD(TMemoryImageArray<TMemoryImagePtr<FShader>>, Shaders);
	LAYOUT_FIELD(TMemoryImageArray<TMemoryImagePtr<FShaderPipeline>>, ShaderPipelines);
	/** The platform this shader map was compiled with */
#if UE_VERSION_AT_LEAST(5, 2, 0)
	LAYOUT_FIELD(FMemoryImageName, ShaderPlatformName);
#else
	LAYOUT_FIELD(TEnumAsByte<EShaderPlatform>, Platform);
#endif
};
static_assert(sizeof(FShaderMapContent) == sizeof(FFXFSR3ShaderMapContent), "FFXFSR3ShaderMapContent must match the layout of FShaderMapContent so we can access the SSR shaders!");

//------------------------------------------------------------------------------------------------------
// Definitions used by the ScreenSpaceReflections shaders needed to perform necessary swizzling.
//------------------------------------------------------------------------------------------------------
class FSSRQualityDim : SHADER_PERMUTATION_ENUM_CLASS("SSR_QUALITY", ESSRQuality);
class FSSROutputForDenoiser : SHADER_PERMUTATION_BOOL("SSR_OUTPUT_FOR_DENOISER");
struct FFXFSR3ScreenSpaceReflectionsPS
{
	using FPermutationDomain = TShaderPermutationDomain<FSSRQualityDim, FSSROutputForDenoiser>;
};

//------------------------------------------------------------------------------------------------------
// In order to access the Lumen reflection data prior to our code executing it is necessary to gain access to FFXFSR3RDGBuilder internals.
//------------------------------------------------------------------------------------------------------
#if UE_VERSION_AT_LEAST(5, 3, 0)
struct FFXFSR3ParallelPassSet : public FRHICommandListImmediate::FQueuedCommandList
{
	FFXFSR3ParallelPassSet() = default;

	TArray<FRDGPass*, FRDGArrayAllocator> Passes;
	IF_RHI_WANT_BREADCRUMB_EVENTS(FRDGBreadcrumbState* BreadcrumbStateBegin{});
	IF_RHI_WANT_BREADCRUMB_EVENTS(FRDGBreadcrumbState* BreadcrumbStateEnd{});
	int8 bInitialized = 0;
	bool bDispatchAfterExecute = false;
	bool bParallelTranslate = false;
};
#endif

class FFXFSR3RDGBuilder : FRDGAllocatorScope
{
public:
	FFXFSR3RDGBuilder(FRHICommandListImmediate& InRHICmdList, FRDGEventName InName = {}, ERDGBuilderFlags InFlags = ERDGBuilderFlags::None)
		: RHICmdList(InRHICmdList)
		, BuilderName(InName)
		, CompilePipe(TEXT("FFXFSR3RDGCompilePipe"))
#if RDG_CPU_SCOPES
		, CPUScopeStacks(Allocator)
#endif
		, GPUScopeStacks(Allocator)
#if RDG_ENABLE_DEBUG
		, UserValidation(Allocator, bParallelExecuteEnabled)
		, BarrierValidation(&Passes, BuilderName)
#endif
#if UE_VERSION_AT_LEAST(5, 2, 0)
		, ExtendResourceLifetimeScope(InRHICmdList)
#endif
	{
	}
	FFXFSR3RDGBuilder(const FFXFSR3RDGBuilder&) = delete;
	~FFXFSR3RDGBuilder()
	{
	}

	FRHICommandListImmediate& RHICmdList;
	struct FFXFSR3BlackBoard
	{
		FRDGAllocator* Allocator;
		TArray<struct FStruct*, FRDGArrayAllocator> Blackboard;
	};
	FFXFSR3BlackBoard Blackboard;

	FRDGTextureRef FindTexture(TCHAR const* Name)
	{
		for (FRDGTextureHandle It = Textures.Begin(); It != Textures.End(); ++It)
		{
			FRDGTextureRef Texture = Textures.Get(It);
			if (FCString::Strcmp(Texture->Name, Name) == 0)
			{
				return Texture;
			}
		}
		return nullptr;
	}

private:
	const FRDGEventName BuilderName;
	FRDGPassRegistry Passes;
	FRDGTextureRegistry Textures;
	FRDGBufferRegistry Buffers;
	FRDGViewRegistry Views;
	FRDGUniformBufferRegistry UniformBuffers;
	TArray<FRDGUniformBufferHandle, FRDGArrayAllocator> UniformBuffersToCreate;
	TSortedMap<FRHITexture*, FRDGTexture*, FRDGArrayAllocator> ExternalTextures;
	TSortedMap<FRHIBuffer*, FRDGBuffer*, FRDGArrayAllocator> ExternalBuffers;
	TMap<FRDGPooledTexture*, FRDGTexture*, FRDGSetAllocator> PooledTextureOwnershipMap;
	TMap<FRDGPooledBuffer*, FRDGBuffer*, FRDGSetAllocator> PooledBufferOwnershipMap;
	TArray<TRefCountPtr<IPooledRenderTarget>, FRDGArrayAllocator> ActivePooledTextures;
	TArray<TRefCountPtr<FRDGPooledBuffer>, FRDGArrayAllocator> ActivePooledBuffers;
	TMap<FRDGBarrierBatchBeginId, FRDGBarrierBatchBegin*, FRDGSetAllocator> BarrierBatchMap;
	FRDGTransitionCreateQueue TransitionCreateQueue;

	template <typename LambdaType>
	UE::Tasks::FTask LaunchCompileTask(const TCHAR* Name, bool bCondition, LambdaType&& Lambda);

	UE::Tasks::FPipe CompilePipe;

	class FPassQueue
	{
		TLockFreePointerListFIFO<FRDGPass, PLATFORM_CACHE_LINE_SIZE> Queue;
		UE::Tasks::FTask LastTask;
	};

	FPassQueue SetupPassQueue;

	TArray<FRDGPassHandle, FRDGArrayAllocator> CullPassStack;

	FRDGPass* ProloguePass;
	FRDGPass* EpiloguePass;

	struct FExtractedTexture
	{
		FRDGTexture* Texture{};
		TRefCountPtr<IPooledRenderTarget>* PooledTexture{};
	};

	TArray<FExtractedTexture, FRDGArrayAllocator> ExtractedTextures;

	struct FExtractedBuffer
	{
		FRDGBuffer* Buffer{};
		TRefCountPtr<FRDGPooledBuffer>* PooledBuffer{};
	};

	TArray<FExtractedBuffer, FRDGArrayAllocator> ExtractedBuffers;

	struct FUploadedBuffer
	{
		bool bUseDataCallbacks;
		bool bUseFreeCallbacks;
		FRDGBuffer* Buffer{};
		const void* Data{};
		uint64 DataSize{};
		FRDGBufferInitialDataCallback DataCallback;
		FRDGBufferInitialDataSizeCallback DataSizeCallback;
		FRDGBufferInitialDataFreeCallback DataFreeCallback;
	};

	TArray<FUploadedBuffer, FRDGArrayAllocator> UploadedBuffers;

#if UE_VERSION_OLDER_THAN(5, 3, 0)
	struct FParallelPassSet : public FRHICommandListImmediate::FQueuedCommandList
	{
		TArray<FRDGPass*, FRDGArrayAllocator> Passes;
		IF_RHI_WANT_BREADCRUMB_EVENTS(FRDGBreadcrumbState* BreadcrumbStateBegin{});
		IF_RHI_WANT_BREADCRUMB_EVENTS(FRDGBreadcrumbState* BreadcrumbStateEnd{});
		int8 bInitialized;
		bool bDispatchAfterExecute;
#if UE_VERSION_AT_LEAST(5, 2, 0)
		bool bParallelTranslate;
#endif
	};
#endif

#if UE_VERSION_AT_LEAST(5, 3, 0)
	TArray<FFXFSR3ParallelPassSet, FRDGArrayAllocator> ParallelPassSets;
#else
	TArray<FParallelPassSet, FRDGArrayAllocator> ParallelPassSets;
#endif

	TArray<UE::Tasks::FTask, FRDGArrayAllocator> ParallelExecuteEvents;

	TArray<UE::Tasks::FTask, FRDGArrayAllocator> ParallelSetupEvents;

	TArray<FRHITrackedAccessInfo, FRDGArrayAllocator> EpilogueResourceAccesses;

	TArray<FRDGViewableResource*, FRDGArrayAllocator> AccessModeQueue;
	TSet<FRDGViewableResource*, DefaultKeyFuncs<FRDGViewableResource*>, FRDGSetAllocator> ExternalAccessResources;

	FRDGTextureSubresourceStateIndirect ScratchTextureState;

	EAsyncComputeBudget AsyncComputeBudgetScope;
	EAsyncComputeBudget AsyncComputeBudgetState;

	FRHICommandList* RHICmdListBufferUploads;

	IF_RDG_CPU_SCOPES(FRDGCPUScopeStacks CPUScopeStacks);
	FRDGGPUScopeStacksByPipeline GPUScopeStacks;
	IF_RHI_WANT_BREADCRUMB_EVENTS(FRDGBreadcrumbState* BreadcrumbState{});

	IF_RDG_ENABLE_TRACE(FRDGTrace Trace);

	bool bFlushResourcesRHI;
	bool bParallelExecuteEnabled;
	bool bParallelSetupEnabled;
#if UE_VERSION_AT_LEAST(5, 2, 0)
	bool bFinalEventScopeActive;
#endif

#if RDG_ENABLE_DEBUG
	FRDGUserValidation UserValidation;
	FRDGBarrierValidation BarrierValidation;
#endif

	struct FAuxiliaryPass
	{
		uint8 Clobber;
		uint8 Visualize;
		uint8 Dump;
		uint8 FlushAccessModeQueue;
	} AuxiliaryPasses;

#if WITH_MGPU
#if UE_VERSION_OLDER_THAN(5, 2, 0)
	FName NameForTemporalEffect;
	bool bWaitedForTemporalEffect;
#endif
	bool bForceCopyCrossGPU;
#endif // WITH_MGPU

	uint32 AsyncComputePassCount;
	uint32 RasterPassCount;

	IF_RDG_CMDLIST_STATS(TStatId CommandListStatScope);
	IF_RDG_CMDLIST_STATS(TStatId CommandListStatState);

	IRHITransientResourceAllocator* TransientResourceAllocator;

#if UE_VERSION_AT_LEAST(5, 2, 0)
	FRHICommandListScopedExtendResourceLifetime ExtendResourceLifetimeScope;
#endif
};
static_assert(sizeof(FRDGBuilder) == sizeof(FFXFSR3RDGBuilder), "FFXFSR3RDGBuilder must match the layout of FRDGBuilder so we can access the Lumen reflection texture!");
#if UE_VERSION_AT_LEAST(5, 4, 0)
#error "Unsupported Unreal Engine 5 version - update the definition for FFXFSR3RDGBuilder."
#endif


//------------------------------------------------------------------------------------------------------
// Internal function definitions
// Many of these are replicas of UE functions used in the denoiser API implementation so that we match the default engine behaviour.
//------------------------------------------------------------------------------------------------------
static bool FFXFSR3HasDeferredPlanarReflections(const FViewInfo& View)
{
	if (View.bIsPlanarReflection || View.bIsReflectionCapture)
	{
		return false;
	}

	// Prevent rendering unsupported views when ViewIndex >= GMaxPlanarReflectionViews
	// Planar reflections in those views will fallback to other reflection methods
	{
		int32 ViewIndex = INDEX_NONE;

		View.Family->Views.Find(&View, ViewIndex);

		if (ViewIndex >= GMaxPlanarReflectionViews)
		{
			return false;
		}
	}

	bool bAnyVisiblePlanarReflections = false;
	FScene* Scene = (FScene*)View.Family->Scene;
	for (int32 PlanarReflectionIndex = 0; PlanarReflectionIndex < Scene->PlanarReflections.Num(); PlanarReflectionIndex++)
	{
		FPlanarReflectionSceneProxy* ReflectionSceneProxy = Scene->PlanarReflections[PlanarReflectionIndex];

		if (View.ViewFrustum.IntersectBox(ReflectionSceneProxy->WorldBounds.GetCenter(), ReflectionSceneProxy->WorldBounds.GetExtent()))
		{
			bAnyVisiblePlanarReflections = true;
			break;
		}
	}

	bool bComposePlanarReflections = Scene->PlanarReflections.Num() > 0 && bAnyVisiblePlanarReflections;

	return bComposePlanarReflections;
}

static bool FFXFSR3ShouldRenderRayTracingEffect(bool bEffectEnabled)
{
	if (!IsRayTracingEnabled())
	{
		return false;
	}

	static auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.ForceAllRayTracingEffects"));
	const int32 OverrideMode = CVar != nullptr ? CVar->GetInt() : -1;

	if (OverrideMode >= 0)
	{
		return OverrideMode > 0;
	}
	else
	{
		return bEffectEnabled;
	}
}

static int32 FFXFSR3GetRayTracingReflectionsSamplesPerPixel(const FViewInfo& View)
{
	static IConsoleVariable* RayTracingReflectionSamplesPerPixel = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.Reflections.SamplesPerPixel"));
	return RayTracingReflectionSamplesPerPixel && RayTracingReflectionSamplesPerPixel->GetInt() >= 0 ? RayTracingReflectionSamplesPerPixel->GetInt() : View.FinalPostProcessSettings.RayTracingReflectionsSamplesPerPixel;
}

static bool FFXFSR3ShouldRenderRayTracingReflections(const FViewInfo& View)
{
	const bool bThisViewHasRaytracingReflections = View.FinalPostProcessSettings.ReflectionMethod == EReflectionMethod::RayTraced;

	static IConsoleVariable* RayTracingReflections = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.Reflections"));
	const bool bReflectionsCvarEnabled = RayTracingReflections && RayTracingReflections->GetInt() < 0
		? bThisViewHasRaytracingReflections
		: (RayTracingReflections && RayTracingReflections->GetInt() != 0);

	const bool bReflectionPassEnabled = bReflectionsCvarEnabled && (FFXFSR3GetRayTracingReflectionsSamplesPerPixel(View) > 0);

	return FFXFSR3ShouldRenderRayTracingEffect(bReflectionPassEnabled);
}

bool IsFFXFSR3SSRTemporalPassRequired(const FViewInfo& View)
{
	static const auto CVarSSRTemporalEnabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SSR.Temporal"));
	
	if (!View.State)
	{
		return false;
	}
	return View.AntiAliasingMethod != AAM_TemporalAA || (CVarSSRTemporalEnabled && CVarSSRTemporalEnabled->GetValueOnAnyThread() != 0);
}

static inline float FFXFSR3GetScreenResolutionFromScalingMode(FfxFsr3UpscalerQualityMode QualityMode)
{
	return 1.0f / ffxFsr3UpscalerGetUpscaleRatioFromQualityMode(QualityMode);
}

//------------------------------------------------------------------------------------------------------
// Whether to use Lumen reflection data or not.
//------------------------------------------------------------------------------------------------------
static bool IsUsingLumenReflections(const FViewInfo& View)
{
	const FSceneViewState* ViewState = View.ViewState;
	if (ViewState && View.Family->Views.Num() == 1)
	{
		static const auto CVarLumenEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.Supported"));
		static const auto CVarLumenReflectionsEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.Reflections.Allow"));
		return FDataDrivenShaderPlatformInfo::GetSupportsLumenGI(View.GetShaderPlatform())
			&& !IsForwardShadingEnabled(View.GetShaderPlatform())
			&& !View.bIsPlanarReflection
			&& !View.bIsSceneCapture
			&& !View.bIsReflectionCapture
			&& View.State
			&& View.FinalPostProcessSettings.ReflectionMethod == EReflectionMethod::Lumen
			&& View.Family->EngineShowFlags.LumenReflections
			&& CVarLumenEnabled
			&& CVarLumenEnabled->GetInt()
			&& CVarLumenReflectionsEnabled
			&& CVarLumenReflectionsEnabled->GetInt();
	}

	return false;
}

//------------------------------------------------------------------------------------------------------
// Definition of inputs & outputs for the FSR3 FX pass used to copy the SceneColor.
//------------------------------------------------------------------------------------------------------
struct FFXFSR3FXPass
{
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		RDG_TEXTURE_ACCESS(InputColorTexture, ERHIAccess::CopySrc)
		RDG_TEXTURE_ACCESS(OutputColorTexture, ERHIAccess::CopyDest)
	END_SHADER_PARAMETER_STRUCT()
};

//------------------------------------------------------------------------------------------------------
// The only way to gather all translucency contribution is to compare the SceneColor data prior and after translucency.
// This requires using the FFXSystemInterface which provides a callback invoked after completing opaque rendering of SceneColor.
//------------------------------------------------------------------------------------------------------
class FFXFSR3FXSystem : public FFXSystemInterface
{
	FGPUSortManager* GPUSortManager;
	FFXFSR3TemporalUpscaler* Upscaler;
	FRHIUniformBuffer* SceneTexturesUniformParams = nullptr;
public:
	static const FName FXName;

	FFXSystemInterface* GetInterface(const FName& InName) final
	{
		return InName == FFXFSR3FXSystem::FXName ? this : nullptr;
	}

	void Tick(UWorld*, float DeltaSeconds) final {}

#if WITH_EDITOR
	void Suspend() final {}

	void Resume() final {}
#endif // #if WITH_EDITOR

	void DrawDebug(FCanvas* Canvas) final {}

	void AddVectorField(UVectorFieldComponent* VectorFieldComponent) final {}

	void RemoveVectorField(UVectorFieldComponent* VectorFieldComponent) final {}

	void UpdateVectorField(UVectorFieldComponent* VectorFieldComponent) final {}

#if UE_VERSION_AT_LEAST(5, 3, 0)
	void PreInitViews(FRDGBuilder&, bool, const TArrayView<const FSceneViewFamily*>&, const FSceneViewFamily*) final {};
	void PostInitViews(FRDGBuilder&, TConstStridedView<FSceneView>, bool) final {};
#else
	void PreInitViews(FRDGBuilder&, bool) final {}

	void PostInitViews(FRDGBuilder&, TArrayView<const FViewInfo, int32>, bool) final {}
#endif

	bool UsesGlobalDistanceField() const final { return false; }

	bool UsesDepthBuffer() const final { return false; }

	bool RequiresEarlyViewUniformBuffer() const final { return false; }

	bool RequiresRayTracingScene() const final { return false; }

#if UE_VERSION_AT_LEAST(5, 3, 0)
	void PreRender(FRDGBuilder&, TConstStridedView<FSceneView>, FSceneUniformBuffer&, bool) final {};
	void PostRenderOpaque(FRDGBuilder& GraphBuilder, TConstStridedView<FSceneView> Views, FSceneUniformBuffer& SceneUniformBuffer, bool bAllowGPUParticleUpdate) final
#else
	void PreRender(FRDGBuilder&, TConstArrayView<FViewInfo>, bool) final {}
	void PostRenderOpaque(FRDGBuilder& GraphBuilder, TConstArrayView<FViewInfo> Views, bool bAllowGPUParticleUpdate) final
#endif
	{
		static const auto CVarFSR3Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.Enabled"));
		if (CVarFSR3CreateReactiveMask.GetValueOnRenderThread() && Upscaler->IsApiSupported() && (CVarFSR3Enabled && CVarFSR3Enabled->GetValueOnRenderThread()) && Views.Num() > 0)
		{
			const FSceneTextures* SceneTextures = nullptr;
			FIntPoint SceneColorSize = FIntPoint::ZeroValue;
			for (auto const& SceneView : Views)
			{
#if UE_VERSION_AT_LEAST(5, 3, 0)
				if (SceneView.bIsViewInfo == false)
					continue;

				const FViewInfo& View = (FViewInfo&)(SceneView);
				if (!SceneTextures)
				{
					SceneTextures = ((FViewFamilyInfo*)View.Family)->GetSceneTexturesChecked();
				}
#else
				auto const& View = SceneView;
#endif
				SceneColorSize.X = FMath::Max(SceneColorSize.X, View.ViewRect.Max.X);
				SceneColorSize.Y = FMath::Max(SceneColorSize.Y, View.ViewRect.Max.Y);
			}
			check(SceneColorSize.X > 0 && SceneColorSize.Y > 0);

#if UE_VERSION_AT_LEAST(5, 3, 0)
			FRHIUniformBuffer* ViewUniformBuffer = SceneUniformBuffer.GetBufferRHI(GraphBuilder);
#else
			FRHIUniformBuffer* ViewUniformBuffer = GetReferenceViewUniformBuffer(Views);
			SceneTextures = GetViewFamilyInfo(Views).GetSceneTexturesChecked();
#endif
			check(SceneTextures);

			FRDGTextureMSAA PreAlpha = SceneTextures->Color;
			auto const& Config = SceneTextures->Config;
			EPixelFormat SceneColorFormat = Config.ColorFormat;
			uint32 NumSamples = Config.NumSamples;

			FIntPoint QuantizedSize;
			QuantizeSceneBufferSize(SceneColorSize, QuantizedSize);

			if (Upscaler->SceneColorPreAlpha.GetReference())
			{
				if (Upscaler->SceneColorPreAlpha->GetSizeX() != QuantizedSize.X
					|| Upscaler->SceneColorPreAlpha->GetSizeY() != QuantizedSize.Y
					|| Upscaler->SceneColorPreAlpha->GetFormat() != SceneColorFormat
					|| Upscaler->SceneColorPreAlpha->GetNumSamples() != NumSamples)
				{
					Upscaler->SceneColorPreAlpha.SafeRelease();
					Upscaler->SceneColorPreAlphaRT.SafeRelease();
				}
			}

			if (Upscaler->SceneColorPreAlpha.GetReference() == nullptr)
			{
				FRHITextureCreateDesc SceneColorPreAlphaCreateDesc = FRHITextureCreateDesc::Create2D(TEXT("FFXFSR3SceneColorPreAlpha"), QuantizedSize.X, QuantizedSize.Y, SceneColorFormat);
				SceneColorPreAlphaCreateDesc.SetNumMips(1);
				SceneColorPreAlphaCreateDesc.SetNumSamples(NumSamples);
				SceneColorPreAlphaCreateDesc.SetFlags((ETextureCreateFlags)(ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource));
				Upscaler->SceneColorPreAlpha = RHICreateTexture(SceneColorPreAlphaCreateDesc);
				Upscaler->SceneColorPreAlphaRT = CreateRenderTarget(Upscaler->SceneColorPreAlpha.GetReference(), TEXT("FFXFSR3SceneColorPreAlpha"));
			}

			FFXFSR3FXPass::FParameters* PassParameters = GraphBuilder.AllocParameters<FFXFSR3FXPass::FParameters>();
			FRDGTextureRef SceneColorPreAlphaRDG = GraphBuilder.RegisterExternalTexture(Upscaler->SceneColorPreAlphaRT);
			PassParameters->InputColorTexture = PreAlpha.Target;
			PassParameters->OutputColorTexture = SceneColorPreAlphaRDG;

			GraphBuilder.AddPass(RDG_EVENT_NAME("FFXFSR3FXSystem::PostRenderOpaque"), PassParameters, ERDGPassFlags::Copy,
				[this, PassParameters, ViewUniformBuffer, PreAlpha](FRHICommandListImmediate& RHICmdList)
				{
					PassParameters->InputColorTexture->MarkResourceAsUsed();
					PassParameters->OutputColorTexture->MarkResourceAsUsed();
					Upscaler->PreAlpha = PreAlpha;
					Upscaler->CopyOpaqueSceneColor(RHICmdList, ViewUniformBuffer, nullptr, this->SceneTexturesUniformParams);
				}
			);
		}
	}

#if UE_VERSION_AT_LEAST(5, 2, 0)
	void SetSceneTexturesUniformBuffer(const TUniformBufferRef<FSceneTextureUniformParameters>& InSceneTexturesUniformParams) final { SceneTexturesUniformParams = InSceneTexturesUniformParams; }
#else
	void SetSceneTexturesUniformBuffer(FRHIUniformBuffer* InSceneTexturesUniformParams) final { SceneTexturesUniformParams = InSceneTexturesUniformParams; }
#endif

	FGPUSortManager* GetGPUSortManager() const 
	{
		return GPUSortManager;
	}

	FFXFSR3FXSystem(FFXFSR3TemporalUpscaler* InUpscaler, FGPUSortManager* InGPUSortManager)
	: GPUSortManager(InGPUSortManager)
		, Upscaler(InUpscaler)
	{
		check(GPUSortManager && Upscaler);
	}
	~FFXFSR3FXSystem() {}
};
FName const FFXFSR3FXSystem::FXName(TEXT("FFXFSR3FXSystem"));

//------------------------------------------------------------------------------------------------------
// Definition of inputs & outputs for the FSR3 pass used by the native backends.
//------------------------------------------------------------------------------------------------------
struct FFXFSR3Pass
{
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		RDG_TEXTURE_ACCESS(ColorTexture, ERHIAccess::SRVMask)
		RDG_TEXTURE_ACCESS(DepthTexture, ERHIAccess::SRVMask)
		RDG_TEXTURE_ACCESS(VelocityTexture, ERHIAccess::SRVMask)
		RDG_TEXTURE_ACCESS(ExposureTexture, ERHIAccess::SRVMask)
		RDG_TEXTURE_ACCESS(ReactiveMaskTexture, ERHIAccess::SRVMask)
		RDG_TEXTURE_ACCESS(CompositeMaskTexture, ERHIAccess::SRVMask)
		RDG_TEXTURE_ACCESS(OutputTexture, ERHIAccess::UAVMask)
	END_SHADER_PARAMETER_STRUCT()
};

//------------------------------------------------------------------------------------------------------
// FFXFSR3TemporalUpscaler implementation.
//------------------------------------------------------------------------------------------------------
FFXFSR3TemporalUpscaler::FFXFSR3TemporalUpscaler()
: Api(EFFXBackendAPI::Unknown)
, ApiAccessor(nullptr)
, CurrentGraphBuilder(nullptr)
, WrappedDenoiser(nullptr)
, ReflectionTexture(nullptr)
{
	FMemory::Memzero(PostInputs);

	PreAlpha.Target = nullptr;
	PreAlpha.Resolve = nullptr;

#if WITH_EDITOR
	bEnabledInEditor = true;
#endif

	FFXFSR3TemporalUpscaler* self = this;
	FFXSystemInterface::RegisterCustomFXSystem(
		FFXFSR3FXSystem::FXName, 
		FCreateCustomFXSystemDelegate::CreateLambda([self](ERHIFeatureLevel::Type InFeatureLevel, EShaderPlatform InShaderPlatform, FGPUSortManager* InGPUSortManager) -> FFXSystemInterface*
	{
		return new FFXFSR3FXSystem(self, InGPUSortManager);
	}));

	FConsoleVariableDelegate EnabledChangedDelegate = FConsoleVariableDelegate::CreateStatic(&FFXFSR3TemporalUpscaler::OnChangeFFXFSR3Enabled);
	IConsoleVariable* CVarEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR3.Enabled"));
	CVarEnabled->SetOnChangedCallback(EnabledChangedDelegate);

	FConsoleVariableDelegate QualityModeChangedDelegate = FConsoleVariableDelegate::CreateStatic(&FFXFSR3TemporalUpscaler::OnChangeFFXFSR3QualityMode);
	CVarFSR3QualityMode->SetOnChangedCallback(QualityModeChangedDelegate);

	if (CVarEnabled->GetBool())
	{
		SaveScreenPercentage();
		UpdateScreenPercentage();
	}
	GEngine->GetDynamicResolutionCurrentStateInfos(DynamicResolutionStateInfos);
}

FFXFSR3TemporalUpscaler::~FFXFSR3TemporalUpscaler()
{
	DeferredCleanup();
	FFXSystemInterface::UnregisterCustomFXSystem(FFXFSR3FXSystem::FXName);
}

const TCHAR* FFXFSR3TemporalUpscaler::GetDebugName() const
{
	return FFXFSR3TemporalUpscalerHistory::GetUpscalerName();
}

void FFXFSR3TemporalUpscaler::ReleaseState(FSR3StateRef State)
{
	FScopeLock Lock(&Mutex);
	if (!AvailableStates.Contains(State))
	{
		AvailableStates.Add(State);
	}
}

void FFXFSR3TemporalUpscaler::DeferredCleanup() const
{
	FScopeLock Lock(&Mutex);
	AvailableStates.Empty();
}

float FFXFSR3TemporalUpscaler::GetResolutionFraction(uint32 Mode)
{
	float ResolutionFraction = 1.f;
	if (Mode != 0)
	{
		FfxFsr3UpscalerQualityMode QualityMode = FMath::Clamp<FfxFsr3UpscalerQualityMode>((FfxFsr3UpscalerQualityMode)Mode, HighestResolutionQualityMode, LowestResolutionQualityMode);
		ResolutionFraction = FFXFSR3GetScreenResolutionFromScalingMode(QualityMode);
	}
	return ResolutionFraction;
}

#if DO_CHECK || DO_GUARD_SLOW || DO_ENSURE
void FFXFSR3TemporalUpscaler::OnFSRMessage(FfxMsgType type, const wchar_t* message)
{
	if (type == FFX_MESSAGE_TYPE_ERROR)
	{
		UE_LOG(LogFSR3, Error, TEXT("%s"), message);
	}
	else if (type == FFX_MESSAGE_TYPE_WARNING)
	{
		UE_LOG(LogFSR3, Warning, TEXT("%s"), message);
	}
}
#endif // DO_CHECK || DO_GUARD_SLOW || DO_ENSURE

void FFXFSR3TemporalUpscaler::SaveScreenPercentage()
{
	SavedScreenPercentage = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.ScreenPercentage"))->GetValueOnGameThread();
}

void FFXFSR3TemporalUpscaler::UpdateScreenPercentage()
{
	float ResolutionFraction = GetResolutionFraction(CVarFSR3QualityMode.GetValueOnGameThread());
	static IConsoleVariable* ScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	ScreenPercentage->Set(ResolutionFraction * 100.0f);
}

void FFXFSR3TemporalUpscaler::RestoreScreenPercentage()
{
	static IConsoleVariable* ScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	ScreenPercentage->Set(SavedScreenPercentage);
}

void FFXFSR3TemporalUpscaler::OnChangeFFXFSR3Enabled(IConsoleVariable* Var)
{
	static const auto CVarFSR3Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.Enabled"));
	if (CVarFSR3Enabled->GetValueOnGameThread())
	{
		SaveScreenPercentage();
		UpdateScreenPercentage();
	}
	else
	{
		RestoreScreenPercentage();
	}
}

void FFXFSR3TemporalUpscaler::OnChangeFFXFSR3QualityMode(IConsoleVariable* Var)
{
	static const auto CVarFSR3Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.Enabled"));
	if (CVarFSR3Enabled->GetValueOnGameThread())
	{
		UpdateScreenPercentage();
	}
}

FRDGBuilder* FFXFSR3TemporalUpscaler::GetGraphBuilder()
{
	return CurrentGraphBuilder;
}

void FFXFSR3TemporalUpscaler::Initialize() const
{
	if (Api == EFFXBackendAPI::Unknown)
	{
		FString RHIName = GDynamicRHI->GetName();

		// Prefer the native backends unless they've been disabled
#if FFX_ENABLE_DX12
		static const auto CVarFSR3DX12Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.UseNativeDX12"));
		if (RHIName == FFXFSR3Strings::D3D12 && ((CVarFSR3DX12Enabled && CVarFSR3DX12Enabled->GetValueOnGameThread()) || FParse::Param(FCommandLine::Get(), TEXT("fsr3native"))))
		{
			IFFXSharedBackendModule* DX12Backend = FModuleManager::GetModulePtr<IFFXSharedBackendModule>(TEXT("FFXD3D12Backend"));
			if (DX12Backend)
			{
				ApiAccessor = DX12Backend->GetBackend();
				if (ApiAccessor)
				{
					Api = EFFXBackendAPI::D3D12;
				}
			}
		}
#endif
		// The fallback implementation requires SM5
		static const auto CVarUseRHI = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.UseRHI"));
		if (!ApiAccessor && (CVarUseRHI && CVarUseRHI->GetValueOnAnyThread()) && IsFeatureLevelSupported(GMaxRHIShaderPlatform, ERHIFeatureLevel::SM5))
		{
			IFFXSharedBackendModule* RHIBackend = FModuleManager::GetModulePtr<IFFXSharedBackendModule>(TEXT("FFXRHIBackend"));
			if (RHIBackend)
			{
				ApiAccessor = RHIBackend->GetBackend();
				if (ApiAccessor)
				{
					Api = EFFXBackendAPI::Unreal;
				}
			}
		}
		
		if (!ApiAccessor)
		{
			Api = EFFXBackendAPI::Unsupported;
			UE_LOG(LogFSR3, Error, TEXT("FSR3 Temporal Upscaler not supported by '%s' rhi"), *RHIName);
		}

		if (IsApiSupported())
		{
			// Wrap any existing denoiser API as we override this to be able to generate the reactive mask.
			WrappedDenoiser = GScreenSpaceDenoiser;
			if (!WrappedDenoiser)
			{
				WrappedDenoiser = IScreenSpaceDenoiser::GetDefaultDenoiser();
			}
			check(WrappedDenoiser);
			GScreenSpaceDenoiser = this;
		}
	}
}

IFFXFSR3TemporalUpscaler::FOutputs FFXFSR3TemporalUpscaler::AddPasses(
	FRDGBuilder& GraphBuilder,
	const FFXFSR3View& SceneView,
	const FFXFSR3PassInput& PassInputs) const
{
#if UE_VERSION_AT_LEAST(5, 3, 0)
	const FViewInfo& View = (FViewInfo&)(SceneView);
#else
	const FFXFSR3View& View = SceneView;
#endif

	// In the MovieRenderPipeline the output extents can be smaller than the input, FSR3 doesn't handle that.
	// In that case we shall fall back to the default upscaler so we render properly.
	FIntPoint InputExtents = View.ViewRect.Size();
	FIntPoint InputExtentsQuantized;
	FIntPoint OutputExtents = View.GetSecondaryViewRectSize();
	FIntPoint OutputExtentsQuantized;

	Initialize();

#if UE_VERSION_AT_LEAST(5, 2, 0)
	bool const bValidEyeAdaptation = View.HasValidEyeAdaptationBuffer();
#else
	bool const bValidEyeAdaptation = View.HasValidEyeAdaptationTexture();
#endif
	bool const bRequestedAutoExposure = static_cast<bool>(CVarFSR3AutoExposure.GetValueOnRenderThread());
	bool const bUseAutoExposure = bRequestedAutoExposure || !bValidEyeAdaptation;

	PreAlpha.Target = nullptr;
	PreAlpha.Resolve = nullptr;

#if UE_VERSION_AT_LEAST(5, 3, 0)
	// The API must be supported, the underlying code has to handle downscaling as well as upscaling.
	check(IsApiSupported() && (View.PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale));
#else
	if (IsApiSupported() && (View.PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale) && (InputExtents.X <= OutputExtents.X) && (InputExtents.Y <= OutputExtents.Y))
#endif
	{
		ITemporalUpscaler::FOutputs Outputs;

		RDG_GPU_STAT_SCOPE(GraphBuilder, FidelityFXSuperResolution3Pass);
		RDG_EVENT_SCOPE(GraphBuilder, "FidelityFXSuperResolution3Pass");

		CurrentGraphBuilder = &GraphBuilder;

		const bool CanWritePrevViewInfo = !View.bStatePrevViewInfoIsReadOnly && View.ViewState;

		bool bHistoryValid = View.PrevViewInfo.TemporalAAHistory.IsValid() && View.ViewState && !View.bCameraCut;

#if UE_VERSION_AT_LEAST(5, 3, 0)
		FRDGTextureRef SceneColor = PassInputs.SceneColor.Texture;
		FRDGTextureRef SceneDepth = PassInputs.SceneDepth.Texture;
		FRDGTextureRef VelocityTexture = PassInputs.SceneVelocity.Texture;
#else
		FRDGTextureRef SceneColor = PassInputs.SceneColorTexture;
		FRDGTextureRef SceneDepth = PassInputs.SceneDepthTexture;
		FRDGTextureRef VelocityTexture = PassInputs.SceneVelocityTexture;
#endif

		// Quantize the buffers to match UE behavior
		QuantizeSceneBufferSize(InputExtents, InputExtentsQuantized);
		QuantizeSceneBufferSize(OutputExtents, OutputExtentsQuantized);

		//------------------------------------------------------------------------------------------------------
		// Create Reactive Mask
		//   Create a reactive mask from separate translucency.
		//------------------------------------------------------------------------------------------------------
		if (!VelocityTexture)
		{
			VelocityTexture = (*PostInputs.SceneTextures)->GBufferVelocityTexture;
		}

		FIntPoint InputTextureExtents = CVarFSR3QuantizeInternalTextures.GetValueOnRenderThread() ? InputExtentsQuantized : InputExtents;
		FRDGTextureSRVDesc DepthDesc = FRDGTextureSRVDesc::Create(SceneDepth);
		FRDGTextureSRVDesc VelocityDesc = FRDGTextureSRVDesc::Create(VelocityTexture);
		FRDGTextureDesc ReactiveMaskDesc = FRDGTextureDesc::Create2D(InputTextureExtents, PF_R8, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable);
		FRDGTextureRef ReactiveMaskTexture = nullptr;
		FRDGTextureDesc CompositeMaskDesc = FRDGTextureDesc::Create2D(InputTextureExtents, PF_R8, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable);
		FRDGTextureRef CompositeMaskTexture = nullptr;
		FRDGTextureDesc SceneColorDesc = FRDGTextureDesc::Create2D(InputTextureExtents, SceneColor->Desc.Format, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable);

		if (CVarFSR3CreateReactiveMask.GetValueOnRenderThread())
		{
			ReactiveMaskTexture = GraphBuilder.CreateTexture(ReactiveMaskDesc, TEXT("FFXFSR3ReactiveMaskTexture"));
			CompositeMaskTexture = GraphBuilder.CreateTexture(CompositeMaskDesc, TEXT("FFXFSR3CompositeMaskTexture"));
			{
				FFXFSR3CreateReactiveMaskCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FFXFSR3CreateReactiveMaskCS::FParameters>();
				PassParameters->Sampler = TStaticSamplerState<SF_Point>::GetRHI();

				FRDGTextureRef GBufferB = (*PostInputs.SceneTextures)->GBufferBTexture;
				if (!GBufferB)
				{
					GBufferB = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
				}

				FRDGTextureRef GBufferD = (*PostInputs.SceneTextures)->GBufferDTexture;
				if (!GBufferD)
				{
					GBufferD = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
				}

				FRDGTextureRef Reflections = ReflectionTexture;
				if (!Reflections)
				{
					Reflections = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
				}

				PassParameters->DepthTexture = SceneDepth;
				PassParameters->InputDepth = GraphBuilder.CreateSRV(DepthDesc);

				FRDGTextureSRVDesc SceneColorSRV = FRDGTextureSRVDesc::Create(SceneColor);
				PassParameters->SceneColor = GraphBuilder.CreateSRV(SceneColorSRV);

				//------------------------------------------------------------------------------------------------------
				// Capturing the scene color pre-alpha requires allocating the texture here, but keeping a reference to it.
				// The texture will be filled in later in the CopyOpaqueSceneColor function.
				//------------------------------------------------------------------------------------------------------
				EPixelFormat SceneColorFormat = SceneColorDesc.Format;

				if (SceneColorPreAlphaRT)
				{
					FRDGTextureRef SceneColorPreAlphaRDG = GraphBuilder.RegisterExternalTexture(SceneColorPreAlphaRT);
					FRDGTextureSRVDesc SceneColorPreAlphaSRV = FRDGTextureSRVDesc::Create(SceneColorPreAlphaRDG);
					PassParameters->SceneColorPreAlpha = GraphBuilder.CreateSRV(SceneColorPreAlphaSRV);
				}
				else
				{
					PassParameters->SceneColorPreAlpha = GraphBuilder.CreateSRV(SceneColorSRV);
				}

				PassParameters->InputVelocity = GraphBuilder.CreateSRV(VelocityDesc);
				
				FRDGTextureRef LumenSpecular;
				FRDGTextureRef CurrentLumenSpecular = nullptr;
#if UE_VERSION_OLDER_THAN(5, 2, 0)
				FFXFSR3RDGBuilder& GraphBulderAccessor = (FFXFSR3RDGBuilder&)GraphBuilder;
				CurrentLumenSpecular = GraphBulderAccessor.FindTexture(TEXT("Lumen.Reflections.SpecularIndirect"));
#endif
				if ((CurrentLumenSpecular || LumenReflections.IsValid()) && bHistoryValid && IsUsingLumenReflections(View))
				{
					LumenSpecular = CurrentLumenSpecular ? CurrentLumenSpecular : GraphBuilder.RegisterExternalTexture(LumenReflections);
				}
				else
				{
					LumenSpecular = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
				}

				FRDGTextureSRVDesc LumenSpecularDesc = FRDGTextureSRVDesc::Create(LumenSpecular);
				PassParameters->LumenSpecular = GraphBuilder.CreateSRV(LumenSpecularDesc);
				PassParameters->LumenSpecularCurrentFrame = (CurrentLumenSpecular && LumenSpecular == CurrentLumenSpecular);

				FRDGTextureSRVDesc GBufferBDesc = FRDGTextureSRVDesc::Create(GBufferB);
				FRDGTextureSRVDesc GBufferDDesc = FRDGTextureSRVDesc::Create(GBufferD);
				FRDGTextureSRVDesc ReflectionsDesc = FRDGTextureSRVDesc::Create(Reflections);
				FRDGTextureUAVDesc ReactiveDesc(ReactiveMaskTexture);
				FRDGTextureUAVDesc CompositeDesc(CompositeMaskTexture);

				PassParameters->GBufferB = GraphBuilder.CreateSRV(GBufferBDesc);
				PassParameters->GBufferD = GraphBuilder.CreateSRV(GBufferDDesc);
				PassParameters->ReflectionTexture = GraphBuilder.CreateSRV(ReflectionsDesc);

				PassParameters->View = View.ViewUniformBuffer;

				PassParameters->ReactiveMask = GraphBuilder.CreateUAV(ReactiveDesc);
				PassParameters->CompositeMask = GraphBuilder.CreateUAV(CompositeDesc);

				PassParameters->FurthestReflectionCaptureDistance = CVarFSR3ReactiveMaskRoughnessForceMaxDistance.GetValueOnRenderThread() ? CVarFSR3ReactiveMaskRoughnessMaxDistance.GetValueOnRenderThread() : FMath::Max(CVarFSR3ReactiveMaskRoughnessMaxDistance.GetValueOnRenderThread(), View.FurthestReflectionCaptureDistance);
				PassParameters->ReactiveMaskReflectionScale = CVarFSR3ReactiveMaskReflectionScale.GetValueOnRenderThread();
				PassParameters->ReactiveMaskRoughnessScale = CVarFSR3ReactiveMaskRoughnessScale.GetValueOnRenderThread();
				PassParameters->ReactiveMaskRoughnessBias = CVarFSR3ReactiveMaskRoughnessBias.GetValueOnRenderThread();
				PassParameters->ReactiveMaskReflectionLumaBias = CVarFSR3ReactiveMaskReflectionLumaBias.GetValueOnRenderThread();
				PassParameters->ReactiveHistoryTranslucencyBias = CVarFSR3ReactiveHistoryTranslucencyBias.GetValueOnRenderThread();
				PassParameters->ReactiveHistoryTranslucencyLumaBias = CVarFSR3ReactiveHistoryTranslucencyLumaBias.GetValueOnRenderThread();
				PassParameters->ReactiveMaskTranslucencyBias = CVarFSR3ReactiveMaskTranslucencyBias.GetValueOnRenderThread();
				PassParameters->ReactiveMaskTranslucencyLumaBias = CVarFSR3ReactiveMaskTranslucencyLumaBias.GetValueOnRenderThread();
				PassParameters->ReactiveMaskTranslucencyMaxDistance = CVarFSR3ReactiveMaskTranslucencyMaxDistance.GetValueOnRenderThread();
				PassParameters->ForceLitReactiveValue = CVarFSR3ReactiveMaskForceReactiveMaterialValue.GetValueOnRenderThread();
				PassParameters->ReactiveShadingModelID = (uint32)CVarFSR3ReactiveMaskReactiveShadingModelID.GetValueOnRenderThread();

				TShaderMapRef<FFXFSR3CreateReactiveMaskCS> ComputeShaderFSR(View.ShaderMap);
				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("FidelityFX-FSR3/CreateReactiveMask (CS)"),
					ComputeShaderFSR,
					PassParameters,
					FComputeShaderUtils::GetGroupCount(FIntVector(InputExtents.X, InputExtents.Y, 1),
						FIntVector(FFXFSR3ConvertVelocityCS::ThreadgroupSizeX, FFXFSR3ConvertVelocityCS::ThreadgroupSizeY, FFXFSR3ConvertVelocityCS::ThreadgroupSizeZ))
				);
			}
#if UE_VERSION_AT_LEAST(5, 2, 0)
			GraphBuilder.QueueTextureExtraction(ReactiveMaskTexture, &ReactiveExtractedTexture);
			GraphBuilder.QueueTextureExtraction(CompositeMaskTexture, &CompositeExtractedTexture);
#endif
		}
		else
		{
			ReactiveMaskTexture = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
			CompositeMaskTexture = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
		}

		// If we are set to de-dither rendering then run the extra pass now - this tries to identify dither patterns and blend them to avoid over-thinning in FSR3.
		// There is specific code for SHADINGMODELID_HAIR pixels which are always dithered.
		if (CVarFSR3DeDitherMode.GetValueOnRenderThread() && (*PostInputs.SceneTextures)->GBufferBTexture)
		{
			FRDGTextureRef TempSceneColor = GraphBuilder.CreateTexture(SceneColorDesc, TEXT("FFXFSR3SubrectColor"));
			FFXFSR3DeDitherCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FFXFSR3DeDitherCS::FParameters>();
			FRDGTextureUAVDesc OutputDesc(TempSceneColor);

			FRDGTextureRef GBufferB = (*PostInputs.SceneTextures)->GBufferBTexture;
			FRDGTextureSRVDesc GBufferBDesc = FRDGTextureSRVDesc::Create(GBufferB);
			PassParameters->GBufferB = GraphBuilder.CreateSRV(GBufferBDesc);

			FRDGTextureSRVDesc SceneColorSRV = FRDGTextureSRVDesc::Create(SceneColor);
			PassParameters->SceneColor = GraphBuilder.CreateSRV(SceneColorSRV);

			PassParameters->View = View.ViewUniformBuffer;

			PassParameters->BlendSceneColor = GraphBuilder.CreateUAV(OutputDesc);

			// Full de-dither requires the proper setting or not running on the Deferred renderer where we can't determine the shading model.
			PassParameters->FullDeDither = (CVarFSR3DeDitherMode.GetValueOnRenderThread() == 1) || (!GBufferB);
			if (!GBufferB)
			{
				GBufferB = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
			}

			TShaderMapRef<FFXFSR3DeDitherCS> ComputeShaderFSR(View.ShaderMap);
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("FidelityFX-FSR3/DeDither (CS)"),
				ComputeShaderFSR,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(FIntVector(SceneColor->Desc.Extent.X, SceneColor->Desc.Extent.Y, 1),
					FIntVector(FFXFSR3DeDitherCS::ThreadgroupSizeX, FFXFSR3DeDitherCS::ThreadgroupSizeY, FFXFSR3DeDitherCS::ThreadgroupSizeZ))
			);

			SceneColor = TempSceneColor;
		}

		//------------------------------------------------------------------------------------------------------
		// Consolidate Motion Vectors
		//   UE4 motion vectors are in sparse format by default.  Convert them to a format consumable by FSR3.
		//------------------------------------------------------------------------------------------------------
		if (!IsValidRef(MotionVectorRT) || MotionVectorRT->GetDesc().Extent.X != InputExtentsQuantized.X || MotionVectorRT->GetDesc().Extent.Y != InputExtentsQuantized.Y)
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(InputExtentsQuantized,
						PF_G16R16F,
						FClearValueBinding::Transparent,
						TexCreate_ShaderResource | TexCreate_UAV,
						TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable,
						false));
			GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, MotionVectorRT, TEXT("FFXFSR3MotionVectorTexture"));
		}

		FRDGTextureRef MotionVectorTexture = GraphBuilder.RegisterExternalTexture(MotionVectorRT);
		{
			FFXFSR3ConvertVelocityCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FFXFSR3ConvertVelocityCS::FParameters>();
			FRDGTextureUAVDesc OutputDesc(MotionVectorTexture);

			PassParameters->DepthTexture = SceneDepth;
			PassParameters->InputDepth = GraphBuilder.CreateSRV(DepthDesc);
			PassParameters->InputVelocity = GraphBuilder.CreateSRV(VelocityDesc);

			PassParameters->View = View.ViewUniformBuffer;

			PassParameters->OutputTexture = GraphBuilder.CreateUAV(OutputDesc);

			TShaderMapRef<FFXFSR3ConvertVelocityCS> ComputeShaderFSR(View.ShaderMap);
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("FidelityFX-FSR3/ConvertVelocity (CS)"),
				ComputeShaderFSR,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(FIntVector(SceneDepth->Desc.Extent.X, SceneDepth->Desc.Extent.Y, 1),
					FIntVector(FFXFSR3ConvertVelocityCS::ThreadgroupSizeX, FFXFSR3ConvertVelocityCS::ThreadgroupSizeY, FFXFSR3ConvertVelocityCS::ThreadgroupSizeZ))
			);
		}

		//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Handle Multiple Viewports
		//   The FSR3 API currently doesn't handle offsetting into buffers.  If the current viewport is not the top left viewport, generate a new texture in which this viewport is at (0,0).
		//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		if (View.ViewRect.Min != FIntPoint::ZeroValue)
		{
			if (!CVarFSR3DeDitherMode.GetValueOnRenderThread())
			{
				FRDGTextureRef TempSceneColor = GraphBuilder.CreateTexture(SceneColorDesc, TEXT("FFXFSR3SubrectColor"));

				AddCopyTexturePass(
					GraphBuilder,
					SceneColor,
					TempSceneColor,
					View.ViewRect.Min,
					FIntPoint::ZeroValue,
					View.ViewRect.Size());

				SceneColor = TempSceneColor;
			}

			FRDGTextureDesc SplitDepthDesc = FRDGTextureDesc::Create2D(InputExtentsQuantized, SceneDepth->Desc.Format, FClearValueBinding::Black, SceneDepth->Desc.Flags);
			FRDGTextureRef TempSceneDepth = GraphBuilder.CreateTexture(SplitDepthDesc, TEXT("FFXFSR3SubrectDepth"));

			AddCopyTexturePass(
					GraphBuilder,
					SceneDepth,
					TempSceneDepth,
					View.ViewRect.Min,
					FIntPoint::ZeroValue,
					View.ViewRect.Size());

			SceneDepth = TempSceneDepth;
		}

		//-------------------
		// Create Resources
		//-------------------
		// Whether alpha channel is supported.
		static const auto CVarPostPropagateAlpha = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.PostProcessing.PropagateAlpha"));
		const bool bSupportsAlpha = (CVarPostPropagateAlpha && CVarPostPropagateAlpha->GetValueOnRenderThread() != 0);
		EPixelFormat OutputFormat = (bSupportsAlpha || (CVarFSR3HistoryFormat.GetValueOnRenderThread() == 0)) ? PF_FloatRGBA : PF_FloatR11G11B10;

		FRDGTextureDesc OutputColorDesc = FRDGTextureDesc::Create2D(OutputExtentsQuantized, OutputFormat, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable);
		FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputColorDesc, TEXT("FFXFSR3OutputTexture"));

		Outputs.FullRes.Texture = OutputTexture;
		Outputs.FullRes.ViewRect = FIntRect(FIntPoint::ZeroValue, View.GetSecondaryViewRectSize());

#if UE_VERSION_OLDER_THAN(5, 3, 0)
		Outputs.HalfRes.Texture = nullptr;
		Outputs.HalfRes.ViewRect = FIntRect::DivideAndRoundUp(Outputs.FullRes.ViewRect, 2);
#endif

		//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Initialize the FSR3 Context
		//   If a context has never been created, or if significant features of the frame have changed since the current context was created, tear down any existing contexts and create a new one matching the current frame.
		//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		FSR3StateRef FSR3State;
#if UE_VERSION_AT_LEAST(5, 3, 0)
		TRefCountPtr<IFFXFSR3CustomTemporalAAHistory> PrevCustomHistory = PassInputs.PrevHistory;
		if (PrevCustomHistory.IsValid() && (PrevCustomHistory->GetDebugName() != GetDebugName()))
		{
			PrevCustomHistory.SafeRelease();
		}
#else
		const TRefCountPtr<IFFXFSR3CustomTemporalAAHistory> PrevCustomHistory = View.PrevViewInfo.CustomTemporalAAHistory;
#endif
		FFXFSR3TemporalUpscalerHistory* CustomHistory = static_cast<FFXFSR3TemporalUpscalerHistory*>(PrevCustomHistory.GetReference());
		bool HasValidContext = CustomHistory && CustomHistory->GetState().IsValid();
		{
			// FSR setup
			FfxFsr3UpscalerContextDescription Params;
			FMemory::Memzero(Params);

			//------------------------------------------------------------------------------------------------------------------------------------------------------------------
			// Describe the Current Frame
			//   Collect the features of the current frame and the current FSR3 history, so we can make decisions about whether any existing FSR3 context is currently usable.
			//------------------------------------------------------------------------------------------------------------------------------------------------------------------

			// FSR settings
			{
				// Engine params:
				Params.flags = 0;
				Params.flags |= bool(ERHIZBuffer::IsInverted) ? FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED  : 0;
				Params.flags |= FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE;
				Params.flags |= ((DynamicResolutionStateInfos.Status == EDynamicResolutionStatus::Enabled) || (DynamicResolutionStateInfos.Status == EDynamicResolutionStatus::DebugForceEnabled)) ? FFX_FSR3UPSCALER_ENABLE_DYNAMIC_RESOLUTION : 0;
				Params.displaySize.height = OutputExtents.Y;
				Params.displaySize.width = OutputExtents.X;
				Params.maxRenderSize.height = InputExtents.Y;
				Params.maxRenderSize.width = InputExtents.X;

				// CVar params:
				// Compute Auto Exposure requires wave operations or D3D12.
				Params.flags |= bUseAutoExposure ? FFX_FSR3UPSCALER_ENABLE_AUTO_EXPOSURE : 0;

#if DO_CHECK || DO_GUARD_SLOW || DO_ENSURE
				// Register message callback
				Params.flags |= FFX_FSR3UPSCALER_ENABLE_DEBUG_CHECKING;
				Params.fpMessage = &FFXFSR3TemporalUpscaler::OnFSRMessage;
#endif // DO_CHECK || DO_GUARD_SLOW || DO_ENSURE
			}

			// We want to reuse FSR3 states rather than recreating them wherever possible as they allocate significant memory for their internal resources.
			// The current custom history is the ideal, but the recently released states can be reused with a simple reset too when the engine cuts the history.
			// This reduces the memory churn imposed by camera cuts.
			if (HasValidContext)
			{
				FfxFsr3UpscalerContextDescription const& CurrentParams = CustomHistory->GetState()->Params;
				if ((CustomHistory->GetState()->LastUsedFrame == GFrameCounterRenderThread) || (CurrentParams.maxRenderSize.width < Params.maxRenderSize.width) || (CurrentParams.maxRenderSize.height < Params.maxRenderSize.height) || (CurrentParams.displaySize.width != Params.displaySize.width) || (CurrentParams.displaySize.height != Params.displaySize.height) || (Params.flags != CurrentParams.flags))
				{
					HasValidContext = false;
				}
				else
				{
					FSR3State = CustomHistory->GetState();
				}
			}
			
			if (!HasValidContext)
			{
				FScopeLock Lock(&Mutex);
				TSet<FSR3StateRef> DisposeStates;
				for (auto& State : AvailableStates)
				{
					FfxFsr3UpscalerContextDescription const& CurrentParams = State->Params;
					if (State->LastUsedFrame == GFrameCounterRenderThread && State->ViewID != View.ViewState->UniqueID)
					{
						// These states can't be reused immediately but perhaps a future frame, otherwise we break split screen.
						continue;
					}
					else if ((CurrentParams.maxRenderSize.width < Params.maxRenderSize.width) || (CurrentParams.maxRenderSize.height < Params.maxRenderSize.height) || (CurrentParams.displaySize.width != Params.displaySize.width) || (CurrentParams.displaySize.height != Params.displaySize.height) || (Params.flags != CurrentParams.flags))
					{
						// States that can't be trivially reused need to just be released to save memory.
						DisposeStates.Add(State);
					}
					else
					{
						FSR3State = State;
						HasValidContext = true;
						bHistoryValid = false;
						break;
					}
				}

				for (auto& State : DisposeStates)
				{
					AvailableStates.Remove(State);
				}
			}

			if (!HasValidContext)
			{
				// For a new context, allocate the necessary scratch memory for the chosen backend
				FSR3State = new FFXFSR3State(ApiAccessor);

				FfxErrorCode Code = ApiAccessor->CreateInterface(FSR3State->Interface, 1);
				check(Code == FFX_OK);
			}

			FSR3State->LastUsedFrame = GFrameCounterRenderThread;
			FSR3State->ViewID = View.ViewState->UniqueID;
			memcpy(&Params.backendInterface, &FSR3State->Interface, sizeof(FfxInterface));

			//-------------------------------------------------------------------------------------------------------------------------------------------------
			// Update History Data (Part 1)
			//   Prepare the view to receive this frame's history data.  This must be done before any attempt to re-create an FSR3 context, if that's needed.
			//-------------------------------------------------------------------------------------------------------------------------------------------------
			if (CanWritePrevViewInfo)
			{
				// Releases the existing history texture inside the wrapper object, this doesn't release NewHistory itself
				View.ViewState->PrevFrameViewInfo.TemporalAAHistory.SafeRelease();

				View.ViewState->PrevFrameViewInfo.TemporalAAHistory.ViewportRect = FIntRect(0, 0, OutputExtents.X, OutputExtents.Y);
				View.ViewState->PrevFrameViewInfo.TemporalAAHistory.ReferenceBufferSize = OutputExtents;

#if UE_VERSION_AT_LEAST(5, 3, 0)
				Outputs.NewHistory = new FFXFSR3TemporalUpscalerHistory(FSR3State, const_cast<FFXFSR3TemporalUpscaler*>(this), MotionVectorRT);
#else
				if (!View.ViewState->PrevFrameViewInfo.CustomTemporalAAHistory.GetReference())
				{
					View.ViewState->PrevFrameViewInfo.CustomTemporalAAHistory = new FFXFSR3TemporalUpscalerHistory(FSR3State, const_cast<FFXFSR3TemporalUpscaler*>(this), MotionVectorRT);
				}
#endif
			}

			//-----------------------------------------------------------------------------------------------------------------------------------------
			// Invalidate FSR3 Contexts
			//   If a context already exists but it is not valid for the current frame's features, clean it up in preparation for creating a new one.
			//-----------------------------------------------------------------------------------------------------------------------------------------
			if (HasValidContext)
			{
				FfxFsr3UpscalerContextDescription const& CurrentParams = FSR3State->Params;

				// Display size must match for splitscreen to work.
				if ((CurrentParams.maxRenderSize.width < Params.maxRenderSize.width) || (CurrentParams.maxRenderSize.height < Params.maxRenderSize.height) || (CurrentParams.displaySize.width != Params.displaySize.width) || (CurrentParams.displaySize.height != Params.displaySize.height) || (Params.flags != CurrentParams.flags) || memcmp(&Params.backendInterface, &CurrentParams.backendInterface, sizeof(FfxInterface)))
				{
					FSR3State->ReleaseResources();
					ffxFsr3UpscalerContextDestroy(&FSR3State->Fsr3);
					HasValidContext = false;
					bHistoryValid = false;
				}
			}

			//------------------------------------------------------
			// Create FSR3 Contexts
			//   If no valid context currently exists, create one.
			//------------------------------------------------------
			if (!HasValidContext)
			{
				FfxErrorCode ErrorCode = ffxFsr3UpscalerContextCreate(&FSR3State->Fsr3, &Params);
				check(ErrorCode == FFX_OK);
				ErrorCode = (ErrorCode == FFX_OK) ? FSR3State->CreateResources() : ErrorCode;
				check(ErrorCode == FFX_OK);
				if (ErrorCode == FFX_OK)
				{
					FMemory::Memcpy(FSR3State->Params, Params);
				}
			}
		}

		//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Organize Inputs (Part 1)
		//   Some inputs FSR3 requires are available now, but will no longer be directly available once we get inside the RenderGraph.  Go ahead and collect the ones we can.
		//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
		FfxFsr3UpscalerDispatchDescription* Fsr3DispatchParamsPtr = new FfxFsr3UpscalerDispatchDescription;
		FfxFsr3UpscalerDispatchDescription& Fsr3DispatchParams = *Fsr3DispatchParamsPtr;
		FMemory::Memzero(Fsr3DispatchParams);
		{
			// Whether to abandon the history in the state on camera cuts
			Fsr3DispatchParams.reset = !bHistoryValid;

			// CVar parameters:
			Fsr3DispatchParams.enableSharpening = (CVarFSR3Sharpness.GetValueOnRenderThread() != 0.0f);
			Fsr3DispatchParams.sharpness = FMath::Clamp(CVarFSR3Sharpness.GetValueOnRenderThread(), 0.0f, 1.0f);

			// Engine parameters:
			Fsr3DispatchParams.frameTimeDelta = View.Family->Time.GetDeltaWorldTimeSeconds() * 1000.f;
			Fsr3DispatchParams.jitterOffset.x = View.TemporalJitterPixels.X;
			Fsr3DispatchParams.jitterOffset.y = View.TemporalJitterPixels.Y;
			Fsr3DispatchParams.preExposure = View.PreExposure;

			Fsr3DispatchParams.renderSize.width = InputExtents.X;
			Fsr3DispatchParams.renderSize.height = InputExtents.Y;

			// @todo parameters for motion vectors - these should be a scale but FSR3 seems to treat them as resolution?
			Fsr3DispatchParams.motionVectorScale.x = InputExtents.X;
			Fsr3DispatchParams.motionVectorScale.y = InputExtents.Y;

			// Untested parameters:
			Fsr3DispatchParams.cameraFovAngleVertical = View.ViewMatrices.ComputeHalfFieldOfViewPerAxis().Y * 2.0f;

			// Unused parameters:
			if (bool(ERHIZBuffer::IsInverted))
			{
				Fsr3DispatchParams.cameraNear = FLT_MAX;
				Fsr3DispatchParams.cameraFar = View.ViewMatrices.ComputeNearPlane();
			}
			else
			{
				Fsr3DispatchParams.cameraNear = View.ViewMatrices.ComputeNearPlane();
				Fsr3DispatchParams.cameraFar = FLT_MAX;
			}
		}

		//------------------------------
		// Add FSR3 to the RenderGraph
		//------------------------------
		FFXFSR3Pass::FParameters* PassParameters = GraphBuilder.AllocParameters<FFXFSR3Pass::FParameters>();
		PassParameters->ColorTexture = SceneColor;
		PassParameters->DepthTexture = SceneDepth;
		PassParameters->VelocityTexture = MotionVectorTexture;
		if (bValidEyeAdaptation)
		{
#if UE_VERSION_AT_LEAST(5, 2, 0)
			FRDGTextureDesc ExposureDesc = FRDGTextureDesc::Create2D({ 1,1 }, PF_A32B32G32R32F, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV);
			FRDGTextureRef ExposureTexture = GraphBuilder.CreateTexture(ExposureDesc, TEXT("FSR3ExposureTexture"));

			FFXFSR3CopyExposureCS::FParameters* ExposureCopyPassParameters = GraphBuilder.AllocParameters<FFXFSR3CopyExposureCS::FParameters>();

			ExposureCopyPassParameters->EyeAdaptationBuffer = GraphBuilder.CreateSRV(GetEyeAdaptationBuffer(GraphBuilder, View));
			ExposureCopyPassParameters->ExposureTexture = GraphBuilder.CreateUAV(ExposureTexture);

			TShaderMapRef<FFXFSR3CopyExposureCS> ComputeShaderFSR(View.ShaderMap);
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("FidelityFX-FSR3/CopyExposure (CS)"),
				ComputeShaderFSR,
				ExposureCopyPassParameters,
				FComputeShaderUtils::GetGroupCount(FIntVector(1, 1, 1), FIntVector(1, 1, 1))
			);
			PassParameters->ExposureTexture = ExposureTexture;
#else // !UE_VERSION_AT_LEAST
			PassParameters->ExposureTexture = GetEyeAdaptationTexture(GraphBuilder, View);
#endif
		}
		PassParameters->ReactiveMaskTexture = ReactiveMaskTexture;
		PassParameters->CompositeMaskTexture = CompositeMaskTexture;
		PassParameters->OutputTexture = OutputTexture;

		auto* ApiAccess = ApiAccessor;
		auto CurrentApi = Api;
		if (CurrentApi == EFFXBackendAPI::Unreal)
		{
			Fsr3DispatchParams.color = ApiAccess->GetNativeResource(PassParameters->ColorTexture.GetTexture(), FFX_RESOURCE_STATE_COMPUTE_READ);
			Fsr3DispatchParams.depth = ApiAccess->GetNativeResource(PassParameters->DepthTexture.GetTexture(), FFX_RESOURCE_STATE_COMPUTE_READ);
			Fsr3DispatchParams.motionVectors = ApiAccess->GetNativeResource(PassParameters->VelocityTexture.GetTexture(), FFX_RESOURCE_STATE_COMPUTE_READ);
			if (PassParameters->ExposureTexture)
			{
				Fsr3DispatchParams.exposure = ApiAccess->GetNativeResource(PassParameters->ExposureTexture.GetTexture(), FFX_RESOURCE_STATE_COMPUTE_READ);
			}
			if (PassParameters->ReactiveMaskTexture)
			{
				Fsr3DispatchParams.reactive = ApiAccess->GetNativeResource(PassParameters->ReactiveMaskTexture.GetTexture(), FFX_RESOURCE_STATE_COMPUTE_READ);
			}
			if (PassParameters->CompositeMaskTexture)
			{
				Fsr3DispatchParams.transparencyAndComposition = ApiAccess->GetNativeResource(PassParameters->CompositeMaskTexture.GetTexture(), FFX_RESOURCE_STATE_COMPUTE_READ);
			}
			Fsr3DispatchParams.output = ApiAccess->GetNativeResource(PassParameters->OutputTexture.GetTexture(), FFX_RESOURCE_STATE_UNORDERED_ACCESS);
			Fsr3DispatchParams.commandList = (FfxCommandList)CurrentGraphBuilder;

			Fsr3DispatchParams.dilatedDepth = FSR3State->Fsr3Resources->dilatedDepth.Resource;
			Fsr3DispatchParams.dilatedMotionVectors = FSR3State->Fsr3Resources->dilatedMotionVectors.Resource;
			Fsr3DispatchParams.reconstructedPrevNearestDepth = FSR3State->Fsr3Resources->reconstructedPrevNearestDepth.Resource;

			ApiAccessor->SetFeatureLevel(FSR3State->Interface, View.GetFeatureLevel());
			FfxErrorCode Code = ffxFsr3UpscalerContextDispatch(&FSR3State->Fsr3, &Fsr3DispatchParams);
			check(Code == FFX_OK);
			delete Fsr3DispatchParamsPtr;
		}
		else
		{
			GraphBuilder.AddPass(RDG_EVENT_NAME("FidelityFX-FSR3"), PassParameters, ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass, [&View, &PassInputs, CurrentApi, ApiAccess, PassParameters, PrevCustomHistory, Fsr3DispatchParamsPtr, FSR3State](FRHICommandListImmediate& RHICmdList)
			{
				//----------------------------------------------------------
				// Organize Inputs (Part 2)
				//   The remaining inputs FSR3 requires are available now.
				//----------------------------------------------------------
				FfxFsr3UpscalerDispatchDescription DispatchParams;
				FMemory::Memcpy(DispatchParams, *Fsr3DispatchParamsPtr);
				delete Fsr3DispatchParamsPtr;

				DispatchParams.color = ApiAccess->GetNativeResource(PassParameters->ColorTexture->GetRHI(), FFX_RESOURCE_STATE_COMPUTE_READ);
				DispatchParams.depth = ApiAccess->GetNativeResource(PassParameters->DepthTexture->GetRHI(), FFX_RESOURCE_STATE_COMPUTE_READ);
				DispatchParams.motionVectors = ApiAccess->GetNativeResource(PassParameters->VelocityTexture->GetRHI(), FFX_RESOURCE_STATE_COMPUTE_READ);

				if (PassParameters->ExposureTexture)
				{
					DispatchParams.exposure = ApiAccess->GetNativeResource(PassParameters->ExposureTexture->GetRHI(), FFX_RESOURCE_STATE_COMPUTE_READ);
				}
				DispatchParams.output = ApiAccess->GetNativeResource(PassParameters->OutputTexture->GetRHI(), FFX_RESOURCE_STATE_UNORDERED_ACCESS);
				if (PassParameters->ReactiveMaskTexture)
				{
					DispatchParams.reactive = ApiAccess->GetNativeResource(PassParameters->ReactiveMaskTexture->GetRHI(), FFX_RESOURCE_STATE_COMPUTE_READ);
				}
				if (PassParameters->CompositeMaskTexture)
				{
					DispatchParams.transparencyAndComposition = ApiAccess->GetNativeResource(PassParameters->CompositeMaskTexture->GetRHI(), FFX_RESOURCE_STATE_COMPUTE_READ);
				}

				DispatchParams.dilatedDepth = FSR3State->Fsr3Resources->dilatedDepth.Resource;
				DispatchParams.dilatedMotionVectors = FSR3State->Fsr3Resources->dilatedMotionVectors.Resource;
				DispatchParams.reconstructedPrevNearestDepth = FSR3State->Fsr3Resources->reconstructedPrevNearestDepth.Resource;

				PassParameters->ColorTexture->MarkResourceAsUsed();
				PassParameters->DepthTexture->MarkResourceAsUsed();
				PassParameters->VelocityTexture->MarkResourceAsUsed();
				if (PassParameters->ExposureTexture)
				{
					PassParameters->ExposureTexture->MarkResourceAsUsed();
				}
				PassParameters->OutputTexture->MarkResourceAsUsed();
				if (PassParameters->ReactiveMaskTexture)
				{
					PassParameters->ReactiveMaskTexture->MarkResourceAsUsed();
				}
				if (PassParameters->CompositeMaskTexture)
				{
					PassParameters->CompositeMaskTexture->MarkResourceAsUsed();
				}

				//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				// Push barriers
				//   Some resources are in the wrong state for FSR3 to execute.  Transition them.  On some platforms, this may involve a bit of tricking the RHI into doing what we want...
				//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				if (ApiAccess)
				{
					ApiAccess->ForceUAVTransition(RHICmdList, PassParameters->OutputTexture->GetRHI(), ERHIAccess::UAVMask);
				}

				{
					SCOPED_DRAW_EVENT(RHICmdList, FidelityFXFSR3Dispatch);
					SCOPED_GPU_STAT(RHICmdList, FidelityFXFSR3Dispatch);

					//-------------------------------------------------------------------------------------
					// Dispatch FSR3
					//   Push the FSR3 algorithm directly onto the underlying graphics APIs command list.
					//-------------------------------------------------------------------------------------
					RHICmdList.EnqueueLambda([FSR3State, DispatchParams, ApiAccess](FRHICommandListImmediate& cmd) mutable
					{
						DispatchParams.commandList = ApiAccess->GetNativeCommandBuffer(cmd);
						FfxErrorCode Code = ffxFsr3UpscalerContextDispatch(&FSR3State->Fsr3, &DispatchParams);
						check(Code == FFX_OK);
					});
				}

				//-----------------------------------------------------------------------------------------------------------------------------------------------
				// Flush instructions to the GPU
				//   The FSR3 Dispatch has tampered with the state of the current command list.  Flush it all the way to the GPU so that Unreal can start anew.
				//-----------------------------------------------------------------------------------------------------------------------------------------------
				RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
			});
		}

		//----------------------------------------------------------------------------------------------------------------------------------
		// Update History Data (Part 2)
		//   Extract the output produced by the FSR3 Dispatch into the history reference we prepared to receive that output during Part 1.
		//----------------------------------------------------------------------------------------------------------------------------------
		if (CanWritePrevViewInfo)
		{
			// Copy the new history into the history wrapper
			GraphBuilder.QueueTextureExtraction(OutputTexture, &View.ViewState->PrevFrameViewInfo.TemporalAAHistory.RT[0]);
		}

		DeferredCleanup();

		return Outputs;
	}
#if UE_VERSION_OLDER_THAN(5, 3, 0)
	else
	{
		return GetDefaultTemporalUpscaler()->AddPasses(
			GraphBuilder,
			View,
			PassInputs);
	}
#endif
}

IFFXFSR3TemporalUpscaler* FFXFSR3TemporalUpscaler::Fork_GameThread(const class FSceneViewFamily& InViewFamily) const
{
	Initialize();

	IFFXFSR3TemporalUpscalingModule& FSR3ModuleInterface = FModuleManager::GetModuleChecked<IFFXFSR3TemporalUpscalingModule>(TEXT("FFXFSR3TemporalUpscaling"));

	return new FFXFSR3TemporalUpscalerProxy(FSR3ModuleInterface.GetFSR3Upscaler());
}

float FFXFSR3TemporalUpscaler::GetMinUpsampleResolutionFraction() const
{
	if (IsApiSupported())
	{
		return FFXFSR3GetScreenResolutionFromScalingMode(LowestResolutionQualityMode);
	}
	else
	{
#if UE_VERSION_AT_LEAST(5, 3, 0)
		return 0;
#else
		return GetDefaultTemporalUpscaler()->GetMinUpsampleResolutionFraction();
#endif
	}
}

float FFXFSR3TemporalUpscaler::GetMaxUpsampleResolutionFraction() const
{
	if (IsApiSupported())
	{
		return FFXFSR3GetScreenResolutionFromScalingMode(HighestResolutionQualityMode);
	}
	else
	{
#if UE_VERSION_AT_LEAST(5, 3, 0)
		return 0;
#else
		return GetDefaultTemporalUpscaler()->GetMaxUpsampleResolutionFraction();
#endif
	}
}

//-------------------------------------------------------------------------------------
// The ScreenSpaceReflections shaders are specialized as to whether they expect to be denoised or not.
// When using the denoising plugin API to capture reflection data it is necessary to swap the shaders so that it appears as it would without denoising.
//-------------------------------------------------------------------------------------
void FFXFSR3TemporalUpscaler::SetSSRShader(FGlobalShaderMap* GlobalMap)
{
	static const FHashedName SSRSourceFile(TEXT("/Engine/Private/SSRT/SSRTReflections.usf"));
	static const FHashedName SSRPixelShader(TEXT("FScreenSpaceReflectionsPS"));

	static const auto CVarFSR3Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.Enabled"));
	const bool bShouldBeSwapped = (CVarFSR3Enabled && (CVarFSR3Enabled->GetValueOnAnyThread() != 0) && (CVarFSR3UseExperimentalSSRDenoiser.GetValueOnAnyThread() == 0));

	FGlobalShaderMapSection* Section = GlobalMap->FindSection(SSRSourceFile);
	if (Section)
	{
		// Accessing SSRShaderMapSwapState is not thread-safe
		check(IsInGameThread());

		FFXFSR3ShaderMapSwapState& ShaderMapSwapState = SSRShaderMapSwapState.FindOrAdd(GlobalMap, FFXFSR3ShaderMapSwapState::Default);
		if (ShaderMapSwapState.Content != Section->GetContent())
		{
			ShaderMapSwapState.Content = Section->GetContent();
			ShaderMapSwapState.bSwapped = false;
		}

		if (bShouldBeSwapped != ShaderMapSwapState.bSwapped)
		{	
#if WITH_EDITORONLY_DATA
			const bool WasFrozen = Section->GetFrozenContentSize() > 0u;
			FShaderMapContent* Content = (FShaderMapContent*)Section->GetMutableContent();
#else
			FShaderMapContent* Content = (FShaderMapContent*)Section->GetContent();
#endif

			FFXFSR3ShaderMapContent* PublicContent = (FFXFSR3ShaderMapContent*)Content;

			for (uint32 i = 0; i < (uint32)ESSRQuality::MAX; i++)
			{
				FFXFSR3ScreenSpaceReflectionsPS::FPermutationDomain DefaultPermutationVector;
				DefaultPermutationVector.Set<FSSRQualityDim>((ESSRQuality)i);
				DefaultPermutationVector.Set<FSSROutputForDenoiser>(false);

				FFXFSR3ScreenSpaceReflectionsPS::FPermutationDomain DenoisePermutationVector;
				DenoisePermutationVector.Set<FSSRQualityDim>((ESSRQuality)i);
				DenoisePermutationVector.Set<FSSROutputForDenoiser>(true);

				// for this very small and simple shader map, index == permutation id
				const uint32 CurrentDefaultIndex = DefaultPermutationVector.ToDimensionValueId(), CurrentDenoiseIndex = DenoisePermutationVector.ToDimensionValueId();
				checkSlow(PublicContent->Shaders[CurrentDefaultIndex].GetChecked() == Content->GetShader(SSRPixelShader, DefaultPermutationVector.ToDimensionValueId())
					   && PublicContent->Shaders[CurrentDenoiseIndex].GetChecked() == Content->GetShader(SSRPixelShader, DenoisePermutationVector.ToDimensionValueId()));
				
				FShader* CurrentDefaultShader = PublicContent->Shaders[CurrentDefaultIndex];
				PublicContent->Shaders[CurrentDefaultIndex] = PublicContent->Shaders[CurrentDenoiseIndex];
				PublicContent->Shaders[CurrentDenoiseIndex] = CurrentDefaultShader;
			}

#if WITH_EDITORONLY_DATA
			// Calling FinalizeContent() is only correct in editor, and if the section was already frozen when we started.
			// if the section wasn't frozen, it hadn't finished loading yet... so how did we get here?
			if (ensure(WasFrozen))
			{
				Section->FinalizeContent();
				ShaderMapSwapState.Content = Section->GetContent();
			}
#endif

			ShaderMapSwapState.bSwapped = bShouldBeSwapped;
		}
	}
}

//-------------------------------------------------------------------------------------
// The FXSystem override lets us copy the scene color after all opaque rendering but before translucency.
// This can be used to compare and pick out translucency data that isn't captured in Separate Translucency.
//-------------------------------------------------------------------------------------
void FFXFSR3TemporalUpscaler::CopyOpaqueSceneColor(FRHICommandListImmediate& RHICmdList, FRHIUniformBuffer* ViewUniformBuffer, const class FShaderParametersMetadata* SceneTexturesUniformBufferStruct, FRHIUniformBuffer* SceneTexturesUniformBuffer)
{
	static const auto CVarFSR3Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.Enabled"));
	FTextureRHIRef SceneColor;
	if (PreAlpha.Target)
	{
		SceneColor = PreAlpha.Target->GetRHI();
	}
	if (IsApiSupported() && (CVarFSR3Enabled && CVarFSR3Enabled->GetValueOnRenderThread()) && SceneColorPreAlpha.GetReference() && SceneColor.GetReference() && SceneColorPreAlpha->GetFormat() == SceneColor->GetFormat())
	{
		SCOPED_DRAW_EVENTF(RHICmdList, FFXFSR3TemporalUpscaler_CopyOpaqueSceneColor, TEXT("FFXFSR3TemporalUpscaler CopyOpaqueSceneColor"));

		FRHICopyTextureInfo Info;
		Info.Size.X = FMath::Min(SceneColorPreAlpha->GetSizeX(), (uint32)SceneColor->GetSizeXYZ().X);
		Info.Size.Y = FMath::Min(SceneColorPreAlpha->GetSizeY(), (uint32)SceneColor->GetSizeXYZ().Y);
		RHICmdList.CopyTexture(SceneColor, SceneColorPreAlpha, Info);
	}
}

//-------------------------------------------------------------------------------------
// Binds the Lumen reflection data & previous depth buffer so we can reproject last frame's Lumen reflections into the reactive mask.
//-------------------------------------------------------------------------------------
void FFXFSR3TemporalUpscaler::SetLumenReflections(FSceneView& InView)
{
	if (InView.State)
	{
		FReflectionTemporalState& ReflectionTemporalState = ((FSceneViewState*)InView.State)->Lumen.ReflectionState;
		LumenReflections = ReflectionTemporalState.SpecularIndirectHistoryRT;
	}
}

//-------------------------------------------------------------------------------------
// Capture the post-processing inputs structure so that the separate translucency textures are available to the reactive mask.
//-------------------------------------------------------------------------------------
void FFXFSR3TemporalUpscaler::SetPostProcessingInputs(FPostProcessingInputs const& NewInputs)
{
	PostInputs = NewInputs;
}

//-------------------------------------------------------------------------------------
// As the upscaler retains some resources during the frame they must be released here to avoid leaking or accessing dangling pointers.
//-------------------------------------------------------------------------------------
void FFXFSR3TemporalUpscaler::EndOfFrame()
{
	PostInputs.SceneTextures = nullptr;
	ReflectionTexture = nullptr;
	LumenReflections.SafeRelease();
	PreAlpha.Target = nullptr;
	PreAlpha.Resolve = nullptr;
#if WITH_EDITOR
	bEnabledInEditor = true;
#endif
}

//-------------------------------------------------------------------------------------
// Updates the state of dynamic resolution for this frame.
//-------------------------------------------------------------------------------------
void FFXFSR3TemporalUpscaler::UpdateDynamicResolutionState()
{
	GEngine->GetDynamicResolutionCurrentStateInfos(DynamicResolutionStateInfos);
}

//-------------------------------------------------------------------------------------
// In the Editor it is necessary to disable the view extension via the upscaler API so it doesn't cause conflicts.
//-------------------------------------------------------------------------------------
#if WITH_EDITOR
bool FFXFSR3TemporalUpscaler::IsEnabledInEditor() const
{
	return bEnabledInEditor;
}

void FFXFSR3TemporalUpscaler::SetEnabledInEditor(bool bEnabled)
{
	bEnabledInEditor = bEnabled;
}
#endif

//-------------------------------------------------------------------------------------
// The interesting function in the denoiser API that lets us capture the reflections texture.
// This has to replicate the behavior of the engine, only we retain a reference to the output texture.
//-------------------------------------------------------------------------------------
IScreenSpaceDenoiser::FReflectionsOutputs FFXFSR3TemporalUpscaler::DenoiseReflections(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FPreviousViewInfo* PreviousViewInfos,
	const FSceneTextureParameters& SceneTextures,
	const FReflectionsInputs& ReflectionInputs,
	const FReflectionsRayTracingConfig RayTracingConfig) const
{
	IScreenSpaceDenoiser::FReflectionsOutputs Outputs;
	Outputs.Color = ReflectionInputs.Color;
	if (FFXFSR3ShouldRenderRayTracingReflections(View) || CVarFSR3UseExperimentalSSRDenoiser.GetValueOnRenderThread())
	{
		Outputs = WrappedDenoiser->DenoiseReflections(GraphBuilder, View, PreviousViewInfos, SceneTextures, ReflectionInputs, RayTracingConfig);
	}
	else if (IsFFXFSR3SSRTemporalPassRequired(View))
	{
		const bool bComposePlanarReflections = FFXFSR3HasDeferredPlanarReflections(View);

		check(View.ViewState);
		FTAAPassParameters TAASettings(View);
		TAASettings.Pass = ETAAPassConfig::ScreenSpaceReflections;
		TAASettings.SceneDepthTexture = SceneTextures.SceneDepthTexture;
		TAASettings.SceneVelocityTexture = SceneTextures.GBufferVelocityTexture;
		TAASettings.SceneColorInput = ReflectionInputs.Color;
		TAASettings.bOutputRenderTargetable = bComposePlanarReflections;

		FTAAOutputs TAAOutputs = AddTemporalAAPass(
			GraphBuilder,
			View,
			TAASettings,
			View.PrevViewInfo.SSRHistory,
			&View.ViewState->PrevFrameViewInfo.SSRHistory);

		Outputs.Color = TAAOutputs.SceneColor;
	}
	ReflectionTexture = Outputs.Color;
	return Outputs;
}

//-------------------------------------------------------------------------------------
// The remaining denoiser API simply passes through to the wrapped denoiser.
//-------------------------------------------------------------------------------------
IScreenSpaceDenoiser::EShadowRequirements FFXFSR3TemporalUpscaler::GetShadowRequirements(
	const FViewInfo& View,
	const FLightSceneInfo& LightSceneInfo,
	const FShadowRayTracingConfig& RayTracingConfig) const
{
	return WrappedDenoiser->GetShadowRequirements(View, LightSceneInfo, RayTracingConfig);
}

void FFXFSR3TemporalUpscaler::DenoiseShadowVisibilityMasks(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FPreviousViewInfo* PreviousViewInfos,
	const FSceneTextureParameters& SceneTextures,
	const TStaticArray<FShadowVisibilityParameters, IScreenSpaceDenoiser::kMaxBatchSize>& InputParameters,
	const int32 InputParameterCount,
	TStaticArray<FShadowVisibilityOutputs, IScreenSpaceDenoiser::kMaxBatchSize>& Outputs) const
{

	return WrappedDenoiser->DenoiseShadowVisibilityMasks(GraphBuilder, View, PreviousViewInfos, SceneTextures, InputParameters, InputParameterCount, Outputs);
}

IScreenSpaceDenoiser::FPolychromaticPenumbraOutputs FFXFSR3TemporalUpscaler::DenoisePolychromaticPenumbraHarmonics(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FPreviousViewInfo* PreviousViewInfos,
	const FSceneTextureParameters& SceneTextures,
	const FPolychromaticPenumbraHarmonics& Inputs) const
{
	return WrappedDenoiser->DenoisePolychromaticPenumbraHarmonics(GraphBuilder, View, PreviousViewInfos, SceneTextures, Inputs);
}

IScreenSpaceDenoiser::FReflectionsOutputs FFXFSR3TemporalUpscaler::DenoiseWaterReflections(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FPreviousViewInfo* PreviousViewInfos,
	const FSceneTextureParameters& SceneTextures,
	const FReflectionsInputs& ReflectionInputs,
	const FReflectionsRayTracingConfig RayTracingConfig) const
{
	return WrappedDenoiser->DenoiseWaterReflections(GraphBuilder, View, PreviousViewInfos, SceneTextures, ReflectionInputs, RayTracingConfig);
}

IScreenSpaceDenoiser::FAmbientOcclusionOutputs FFXFSR3TemporalUpscaler::DenoiseAmbientOcclusion(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FPreviousViewInfo* PreviousViewInfos,
	const FSceneTextureParameters& SceneTextures,
	const FAmbientOcclusionInputs& ReflectionInputs,
	const FAmbientOcclusionRayTracingConfig RayTracingConfig) const
{
	return WrappedDenoiser->DenoiseAmbientOcclusion(GraphBuilder, View, PreviousViewInfos, SceneTextures, ReflectionInputs, RayTracingConfig);
}

FSSDSignalTextures FFXFSR3TemporalUpscaler::DenoiseDiffuseIndirect(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FPreviousViewInfo* PreviousViewInfos,
	const FSceneTextureParameters& SceneTextures,
	const FDiffuseIndirectInputs& Inputs,
	const FAmbientOcclusionRayTracingConfig Config) const
{
	return WrappedDenoiser->DenoiseDiffuseIndirect(GraphBuilder, View, PreviousViewInfos, SceneTextures, Inputs, Config);
}

IScreenSpaceDenoiser::FDiffuseIndirectOutputs FFXFSR3TemporalUpscaler::DenoiseSkyLight(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FPreviousViewInfo* PreviousViewInfos,
	const FSceneTextureParameters& SceneTextures,
	const FDiffuseIndirectInputs& Inputs,
	const FAmbientOcclusionRayTracingConfig Config) const
{
	return WrappedDenoiser->DenoiseSkyLight(GraphBuilder, View, PreviousViewInfos, SceneTextures, Inputs, Config);
}

IScreenSpaceDenoiser::FDiffuseIndirectOutputs FFXFSR3TemporalUpscaler::DenoiseReflectedSkyLight(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FPreviousViewInfo* PreviousViewInfos,
	const FSceneTextureParameters& SceneTextures,
	const FDiffuseIndirectInputs& Inputs,
	const FAmbientOcclusionRayTracingConfig Config) const
{
	return WrappedDenoiser->DenoiseReflectedSkyLight(GraphBuilder, View, PreviousViewInfos, SceneTextures, Inputs, Config);
}

FSSDSignalTextures FFXFSR3TemporalUpscaler::DenoiseDiffuseIndirectHarmonic(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FPreviousViewInfo* PreviousViewInfos,
	const FSceneTextureParameters& SceneTextures,
	const FDiffuseIndirectHarmonic& Inputs,
	const HybridIndirectLighting::FCommonParameters& CommonDiffuseParameters) const
{
	return WrappedDenoiser->DenoiseDiffuseIndirectHarmonic(GraphBuilder, View, PreviousViewInfos, SceneTextures, Inputs, CommonDiffuseParameters);
}

bool FFXFSR3TemporalUpscaler::SupportsScreenSpaceDiffuseIndirectDenoiser(EShaderPlatform Platform) const
{
	return WrappedDenoiser->SupportsScreenSpaceDiffuseIndirectDenoiser(Platform);
}

FSSDSignalTextures FFXFSR3TemporalUpscaler::DenoiseScreenSpaceDiffuseIndirect(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FPreviousViewInfo* PreviousViewInfos,
	const FSceneTextureParameters& SceneTextures,
	const FDiffuseIndirectInputs& Inputs,
	const FAmbientOcclusionRayTracingConfig Config) const
{
	return WrappedDenoiser->DenoiseScreenSpaceDiffuseIndirect(GraphBuilder, View, PreviousViewInfos, SceneTextures, Inputs, Config);
}