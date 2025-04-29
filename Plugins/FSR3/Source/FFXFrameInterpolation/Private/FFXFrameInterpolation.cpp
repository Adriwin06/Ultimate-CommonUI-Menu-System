// This file is part of the FidelityFX Super Resolution 3.1 Unreal Engine Plugin.
//
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "FFXFrameInterpolation.h"
#include "FFXFrameInterpolationViewExtension.h"
#include "FFXSharedBackend.h"
#include "FFXFrameInterpolationSlate.h"
#include "FFXFrameInterpolationCustomPresent.h"
#include "FFXFSR3Settings.h"

#include "PostProcess/PostProcessing.h"
#include "PostProcess/TemporalAA.h"

#include "FFXFrameInterpolationApi.h"
#include "FFXOpticalFlowApi.h"

#include "ScenePrivate.h"
#include "RenderTargetPool.h"
#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "DataDrivenShaderPlatformInfo.h"
#endif
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"

#include "IAntiLag2.h"

#if UE_VERSION_AT_LEAST(5, 0, 0)
#define GET_RHI_TARGET_ARG 
#define GET_RHI_VIEW_ARG 
#define GET_TEXTURE 
#else
#define GET_RHI_TARGET_ARG ERenderTargetTexture::Targetable
#define GET_RHI_VIEW_ARG ERenderTargetTexture::ShaderResource
#define GET_TEXTURE .GetTexture()
#endif

//------------------------------------------------------------------------------------------------------
// In order to access the distortion data prior to our code executing it is necessary to gain access to FFXFIRDGBuilder internals.
//------------------------------------------------------------------------------------------------------
#if UE_VERSION_AT_LEAST(5, 3, 0)
struct FFXFIParallelPassSet : public FRHICommandListImmediate::FQueuedCommandList
{
	FFXFIParallelPassSet() = default;

	TArray<FRDGPass*, FRDGArrayAllocator> Passes;
#if UE_VERSION_OLDER_THAN(5, 5, 0)
	IF_RHI_WANT_BREADCRUMB_EVENTS(FRDGBreadcrumbState* BreadcrumbStateBegin{});
	IF_RHI_WANT_BREADCRUMB_EVENTS(FRDGBreadcrumbState* BreadcrumbStateEnd{});
	int8 bInitialized = 0;
#endif
	bool bDispatchAfterExecute = false;
#if UE_VERSION_AT_LEAST(5, 5, 0)
	bool bTaskModeAsync = false;
#else
	bool bParallelTranslate = false;
#endif
};
#endif


#if UE_VERSION_AT_LEAST(5, 1, 0)
#if UE_VERSION_AT_LEAST(5, 5, 0)
class FFXFIRDGBuilder : FRDGScopeState
#elif UE_VERSION_AT_LEAST(5, 4, 0)
class FFXFIRDGBuilder
#else
class FFXFIRDGBuilder : FRDGAllocatorScope
#endif
{
#if UE_VERSION_AT_LEAST(5, 4, 0)
	struct FAsyncDeleter
	{
		TUniqueFunction<void()> Function;
#if UE_VERSION_AT_LEAST(5, 5, 0)
		UE::Tasks::FTask Prerequisites;
#endif
		//static UE::Tasks::FTask LastTask;

		RENDERCORE_API ~FAsyncDeleter()
		{
		}
	} AsyncDeleter;

	struct
	{
		FRDGAllocator Root;
		FRDGAllocator Task;
		FRDGAllocator Transition;

		int32 GetByteCount() const
		{
			return Root.GetByteCount() + Task.GetByteCount() + Transition.GetByteCount();
		}

	} Allocators;

	FRDGAllocatorScope RootAllocatorScope;
#endif

public:
	FFXFIRDGBuilder(FRHICommandListImmediate& InRHICmdList, FRDGEventName InName = {}, ERDGBuilderFlags InFlags = ERDGBuilderFlags::None)
#if UE_VERSION_AT_LEAST(5, 5, 0)
		: FRDGScopeState(InRHICmdList, true, true)
		, RootAllocatorScope(Allocators.Transition)
#elif UE_VERSION_AT_LEAST(5, 4, 0)
		: RootAllocatorScope(Allocators.Transition)
		, RHICmdList(InRHICmdList)
#else
		: RHICmdList(InRHICmdList)
#endif
		, BuilderName(InName)
#if UE_VERSION_OLDER_THAN(5, 4, 0)
		, CompilePipe(TEXT("FFXFIRDGCompilePipe"))
#if RDG_CPU_SCOPES

		, CPUScopeStacks(Allocator)
#endif
		, GPUScopeStacks(Allocator)
#endif
#if UE_VERSION_AT_LEAST(5, 4, 0)
		, ExtendResourceLifetimeScope(InRHICmdList)
#endif
#if RDG_ENABLE_DEBUG
#if UE_VERSION_AT_LEAST(5, 5, 0)
		, UserValidation(Allocators.Transition)
#elif UE_VERSION_AT_LEAST(5, 4, 0)
		, UserValidation(Allocators.Transition, false)
#else
		, UserValidation(Allocator, bParallelExecuteEnabled)
#endif
		, BarrierValidation(&Passes, BuilderName)
#endif
#if UE_VERSION_AT_LEAST(5, 2, 0) && UE_VERSION_OLDER_THAN(5, 4, 0)
		, ExtendResourceLifetimeScope(InRHICmdList)
#endif
	{
	}

	FFXFIRDGBuilder(const FFXFIRDGBuilder&) = delete;
	~FFXFIRDGBuilder()
	{
	}

#if UE_VERSION_OLDER_THAN(5, 5, 0)
	FRHICommandListImmediate& RHICmdList;
#endif
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
#if UE_VERSION_AT_LEAST(5, 4, 0)
	FRDGPass* ProloguePass = nullptr;
	FRDGPass* EpiloguePass = nullptr;
	uint32 AsyncComputePassCount = 0;
	uint32 RasterPassCount = 0;
#if UE_VERSION_AT_LEAST(5, 5, 0)
	TArray<FRDGDispatchPass*, FRDGArrayAllocator> DispatchPasses;
#else
	EAsyncComputeBudget AsyncComputeBudgetScope = EAsyncComputeBudget::EAll_4;
	EAsyncComputeBudget AsyncComputeBudgetState = EAsyncComputeBudget(~0u);
	IF_RDG_CMDLIST_STATS(TStatId CommandListStatScope);
	IF_RDG_CMDLIST_STATS(TStatId CommandListStatState);
	IF_RDG_CPU_SCOPES(FRDGCPUScopeStacks CPUScopeStacks);
	FRDGGPUScopeStacksByPipeline GPUScopeStacks;
	IF_RHI_WANT_BREADCRUMB_EVENTS(FRDGBreadcrumbState* BreadcrumbState{});
#endif
	FRDGPassRegistry Passes;
	FRDGTextureRegistry Textures;
	FRDGBufferRegistry Buffers;
	FRDGViewRegistry Views;
	FRDGUniformBufferRegistry UniformBuffers;

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

	Experimental::TRobinHoodHashMap<FRHITexture*, FRDGTexture*, DefaultKeyFuncs<FRHITexture*>, FRDGArrayAllocator> ExternalTextures;
	Experimental::TRobinHoodHashMap<FRHIBuffer*, FRDGBuffer*, DefaultKeyFuncs<FRHIBuffer*>, FRDGArrayAllocator> ExternalBuffers;

	TArray<FRDGBuffer*, FRDGArrayAllocator> NumElementsCallbackBuffers;

	IRHITransientResourceAllocator* TransientResourceAllocator = nullptr;
	bool bSupportsTransientTextures = false;
	bool bSupportsTransientBuffers = false;

	Experimental::TRobinHoodHashMap<FRDGPooledTexture*, FRDGTexture*, DefaultKeyFuncs<FRDGPooledTexture*>, FConcurrentLinearArrayAllocator> PooledTextureOwnershipMap;
	Experimental::TRobinHoodHashMap<FRDGPooledBuffer*, FRDGBuffer*, DefaultKeyFuncs<FRDGPooledBuffer*>, FConcurrentLinearArrayAllocator> PooledBufferOwnershipMap;

	TMap<FRDGBarrierBatchBeginId, FRDGBarrierBatchBegin*, FRDGSetAllocator> BarrierBatchMap;
	TArray<FRHITrackedAccessInfo, FRDGArrayAllocator> EpilogueResourceAccesses;
	TArray<TRefCountPtr<IPooledRenderTarget>, FRDGArrayAllocator> ActivePooledTextures;
	TArray<TRefCountPtr<FRDGPooledBuffer>, FRDGArrayAllocator> ActivePooledBuffers;
	FRDGTransitionCreateQueue TransitionCreateQueue;
	FRDGTextureSubresourceState ScratchTextureState;
	FRDGSubresourceState PrologueSubresourceState;

	struct FAsyncSetupOp
	{
#if UE_VERSION_AT_LEAST(5, 5, 0)
		enum class EType : uint8
		{
			SetupPassResources,
			CullRootBuffer,
			CullRootTexture,
			ReservedBufferCommit
		};
#else
		enum class EType
		{
			SetupPassResources,
			CullRootBuffer,
			CullRootTexture
		};
#endif

#if UE_VERSION_AT_LEAST(5, 5, 0)
		uint64 Type : 8;
		uint64 Payload : 48;
#else
		EType Type;
#endif

		union
		{
			FRDGPass* Pass;
			FRDGBuffer* Buffer;
			FRDGTexture* Texture;
		};
	};

	struct FAsyncSetupQueue
	{
		UE::FMutex Mutex;
		TArray<FAsyncSetupOp, FRDGArrayAllocator> Ops;
#if UE_VERSION_OLDER_THAN(5, 5, 0)
		UE::Tasks::FTask LastTask;
#endif
		UE::Tasks::FPipe Pipe{ TEXT("FRDGBuilder::AsyncSetupQueue") };

	} AsyncSetupQueue;

#if UE_VERSION_AT_LEAST(5, 5, 0)
	TArray<uint64, FRDGArrayAllocator> ReservedBufferCommitSizes;
#endif

	TArray<FRDGPass*, FRDGArrayAllocator> CullPassStack;

	struct
	{
#if UE_VERSION_AT_LEAST(5, 5, 0)
		TStaticArray<TArray<UE::Tasks::FTask, FRDGArrayAllocator>, (int32)ERDGSetupTaskWaitPoint::MAX> Tasks;
#else
		TArray<UE::Tasks::FTask, FRDGArrayAllocator> Tasks;
		TArray<FRHICommandListImmediate::FQueuedCommandList, FConcurrentLinearArrayAllocator> CommandLists;
#endif
		bool bEnabled = false;

	} ParallelSetup;

#if UE_VERSION_AT_LEAST(5, 5, 0)
	bool bParallelCompileEnabled = false;
#endif

	struct
	{
		TArray<FFXFIParallelPassSet, FRDGArrayAllocator> ParallelPassSets;
#if UE_VERSION_AT_LEAST(5, 5, 0)
		TOptional<UE::Tasks::FTaskEvent> TasksAwait;
		TOptional<UE::Tasks::FTaskEvent> TasksAsync;
		TOptional<UE::Tasks::FTaskEvent> DispatchTaskEventAwait;
		TOptional<UE::Tasks::FTaskEvent> DispatchTaskEventAsync;
		ERDGPassTaskMode TaskMode = ERDGPassTaskMode::Inline;
#else
		TArray<UE::Tasks::FTask, FRDGArrayAllocator> Tasks;
		TOptional<UE::Tasks::FTaskEvent> DispatchTaskEvent;
		bool bEnabled = false;
#endif
	} ParallelExecute;

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
		FRDGBufferInitialDataFillCallback DataFillCallback;
	};

	TArray<FUploadedBuffer, FRDGArrayAllocator> UploadedBuffers;

	TArray<FRDGViewableResource*, FRDGArrayAllocator> AccessModeQueue;
	TSet<FRDGViewableResource*, DefaultKeyFuncs<FRDGViewableResource*>, FRDGSetAllocator> ExternalAccessResources;

#if UE_VERSION_AT_LEAST(5, 5, 0)
	TArray<TUniqueFunction<void()>, FRDGArrayAllocator> PostExecuteCallbacks;

	FGraphEventArray WaitOutstandingTasks;
#endif
	bool bFlushResourcesRHI = false;
	FRHICommandListScopedExtendResourceLifetime ExtendResourceLifetimeScope;

	struct FAuxiliaryPass
	{
		uint8 Clobber = 0;
		uint8 Visualize = 0;
		uint8 Dump = 0;
		uint8 FlushAccessModeQueue = 0;
	} AuxiliaryPasses;

#if UE_VERSION_AT_LEAST(5, 5, 0) && WITH_MGPU
	bool bForceCopyCrossGPU = false;
#endif

	IF_RDG_ENABLE_TRACE(FRDGTrace Trace);

#if RDG_ENABLE_DEBUG
	FRDGUserValidation UserValidation;
	FRDGBarrierValidation BarrierValidation;
#endif

#else // 5.0.3 or older
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
	TArray<FFXFIParallelPassSet, FRDGArrayAllocator> ParallelPassSets;
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

#endif
};
static_assert(sizeof(FRDGBuilder) == sizeof(FFXFIRDGBuilder), "FFXFIRDGBuilder must match the layout of FRDGBuilder so we can access the Lumen reflection texture!");

#if UE_VERSION_AT_LEAST(5, 6, 0)
#error "Unsupported Unreal Engine 5 version - update the definition for FFXFIRDGBuilder."
#endif

#endif

//------------------------------------------------------------------------------------------------------
// Helper variable declarations.
//------------------------------------------------------------------------------------------------------
static uint32_t s_opticalFlowBlockSize = 8;
static uint32_t s_opticalFlowSearchRadius = 8;

extern ENGINE_API float GAverageFPS;
extern ENGINE_API float GAverageMS;

#if UE_VERSION_OLDER_THAN(4, 27, 0)
#define GFrameCounterRenderThread GFrameNumberRenderThread
#endif

//------------------------------------------------------------------------------------------------------
// Input declaration for the frame interpolation pass.
//------------------------------------------------------------------------------------------------------
struct FFXFrameInterpolationPass
{
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		RDG_TEXTURE_ACCESS(ColorTexture, ERHIAccess::SRVCompute)
		RDG_TEXTURE_ACCESS(BackBufferTexture, ERHIAccess::SRVCompute)
		RDG_TEXTURE_ACCESS(SceneDepth, ERHIAccess::SRVCompute)
		RDG_TEXTURE_ACCESS(MotionVectors, ERHIAccess::SRVCompute)
		RDG_TEXTURE_ACCESS(DistortionTexture, ERHIAccess::SRVCompute)
		RDG_TEXTURE_ACCESS(HudTexture, ERHIAccess::CopyDest)
		RDG_TEXTURE_ACCESS(InterpolatedRT, ERHIAccess::CopyDest)
		RDG_TEXTURE_ACCESS(Interpolated, ERHIAccess::CopyDest)
	END_SHADER_PARAMETER_STRUCT()
};

//------------------------------------------------------------------------------------------------------
// Unreal shader to convert from the Velocity texture format to the Motion Vectors used by FFX.
//------------------------------------------------------------------------------------------------------
class FFXFIConvertVelocityCS : public FGlobalShader
{
public:
	static const int ThreadgroupSizeX = 8;
	static const int ThreadgroupSizeY = 8;
	static const int ThreadgroupSizeZ = 1;

	DECLARE_GLOBAL_SHADER(FFXFIConvertVelocityCS);
	SHADER_USE_PARAMETER_STRUCT(FFXFIConvertVelocityCS, FGlobalShader);

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
		OutEnvironment.SetDefine(TEXT("UNREAL_ENGINE_MINOR_VERSION"), ENGINE_MINOR_VERSION);
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXFIConvertVelocityCS, "/Plugin/FSR3/Private/PostProcessFFX_FSR3ConvertVelocity.usf", "MainCS", SF_Compute);

//------------------------------------------------------------------------------------------------------
// Unreal shader to convert from the Distortion texture format for use by FFX.
//------------------------------------------------------------------------------------------------------
class FFXFIConvertDistortionCS : public FGlobalShader
{
public:
	static const int ThreadgroupSizeX = 8;
	static const int ThreadgroupSizeY = 8;
	static const int ThreadgroupSizeZ = 1;

	DECLARE_GLOBAL_SHADER(FFXFIConvertDistortionCS);
	SHADER_USE_PARAMETER_STRUCT(FFXFIConvertDistortionCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, UEDistortion)
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
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXFIConvertDistortionCS, "/Plugin/FSR3/Private/PostProcessFFX_FIConvertDistortion.usf", "MainCS", SF_Compute);

#if UE_VERSION_OLDER_THAN(5, 1, 0)
inline void TransitionAndCopyTexture(FRHICommandList& RHICmdList, FRHITexture* SrcTexture, FRHITexture* DstTexture, const FRHICopyTextureInfo& Info)
{
	check(SrcTexture && DstTexture);
	check(SrcTexture->GetNumSamples() == DstTexture->GetNumSamples());

	if (SrcTexture == DstTexture)
	{
		RHICmdList.Transition({
			FRHITransitionInfo(SrcTexture, ERHIAccess::Unknown, ERHIAccess::SRVMask)
			});
		return;
	}

	RHICmdList.Transition({
		FRHITransitionInfo(SrcTexture, ERHIAccess::Unknown, ERHIAccess::CopySrc),
		FRHITransitionInfo(DstTexture, ERHIAccess::Unknown, ERHIAccess::CopyDest)
		});

	RHICmdList.CopyTexture(SrcTexture, DstTexture, Info);

	RHICmdList.Transition({
		FRHITransitionInfo(SrcTexture, ERHIAccess::CopySrc,  ERHIAccess::SRVMask),
		FRHITransitionInfo(DstTexture, ERHIAccess::CopyDest, ERHIAccess::SRVMask)
		});
}
#endif

//------------------------------------------------------------------------------------------------------
// Implementation for the Frame Interpolation.
//------------------------------------------------------------------------------------------------------
FFXFrameInterpolation::FFXFrameInterpolation()
: GameDeltaTime(0.0)
, LastTime(FPlatformTime::Seconds())
, AverageTime(0.f)
, AverageFPS(0.f)
, InterpolationCount(0llu)
, PresentCount(0llu)
, Index(0u)
, ResetState(0u)
, bInterpolatedFrame(false)
{
	UGameViewportClient::OnViewportCreated().AddRaw(this, &FFXFrameInterpolation::OnViewportCreatedHandler_SetCustomPresent);
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FFXFrameInterpolation::OnPostEngineInit);
}

FFXFrameInterpolation::~FFXFrameInterpolation()
{
	ViewExtension = nullptr;
}

IFFXFrameInterpolationCustomPresent* FFXFrameInterpolation::CreateCustomPresent(IFFXSharedBackend* Backend, uint32_t Flags, FIntPoint RenderSize, FIntPoint DisplaySize, FfxSwapchain RawSwapChain, FfxCommandQueue Queue, FfxApiSurfaceFormat Format, EFFXBackendAPI Api)
{
	FFXFrameInterpolationCustomPresent* Result = new FFXFrameInterpolationCustomPresent;
	if (Result)
	{
		if (Result->InitSwapChain(Backend, Flags, RenderSize, DisplaySize, RawSwapChain, Queue, Format, Api))
		{
			SwapChains.Add(RawSwapChain, Result);
		}
	}
	return Result;
}

bool FFXFrameInterpolation::GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS)
{
	bool bOK = false;
	AvgTimeMs = GAverageMS;
	AvgFPS = GAverageFPS;
	auto* Engine = GEngine;
	auto GameViewport = Engine ? Engine->GameViewport : nullptr;
	auto Viewport = GameViewport ? GameViewport->Viewport : nullptr;
	auto ViewportRHI = Viewport ? Viewport->GetViewportRHI() : nullptr;
	FFXFrameInterpolationCustomPresent* Presenter = ViewportRHI.IsValid() ? (FFXFrameInterpolationCustomPresent*)ViewportRHI->GetCustomPresent() : nullptr;
	if (Presenter)
	{
		if ((Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative) || Presenter->GetUseFFXSwapchain())
		{
			bOK = Presenter->GetBackend()->GetAverageFrameTimes(AvgTimeMs, AvgFPS);
		}
		else if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeRHI)
		{
			AvgTimeMs = AverageTime;
			AvgFPS = AverageFPS;
			bOK = true;
		}
	}
	return bOK;
}

void FFXFrameInterpolation::OnViewportCreatedHandler_SetCustomPresent()
{
    if (GEngine && GEngine->GameViewport)
	{
		if (!GEngine->GameViewport->Viewport->GetViewportRHI().IsValid())
		{
			GEngine->GameViewport->OnBeginDraw().AddRaw(this, &FFXFrameInterpolation::OnBeginDrawHandler);
		}
	}
}

void FFXFrameInterpolation::OnBeginDrawHandler()
{
	if (GEngine->GameViewport->Viewport->GetViewportRHI().IsValid() && (GEngine->GameViewport->Viewport->GetViewportRHI()->GetCustomPresent() == nullptr))
	{
		auto ViewportRHI = GEngine->GameViewport->Viewport->GetViewportRHI();
		void* NativeSwapChain = ViewportRHI->GetNativeSwapChain();
		FFXFrameInterpolationCustomPresent** PresentHandler = SwapChains.Find(NativeSwapChain);
		if (PresentHandler)
		{
			(*PresentHandler)->InitViewport(GEngine->GameViewport->Viewport, GEngine->GameViewport->Viewport->GetViewportRHI());
		}
		else if ((CVarFSR3UseRHI.GetValueOnAnyThread() != 0) || FParse::Param(FCommandLine::Get(), TEXT("fsr3rhi")))
		{
			IFFXSharedBackendModule* RHIBackendModule = FModuleManager::GetModulePtr<IFFXSharedBackendModule>(TEXT("FFXRHIBackend"));
			check(RHIBackendModule);

			auto* RHIBackend = RHIBackendModule->GetBackend();
			RHIBackend->Init();
		}
	}
}

void FFXFrameInterpolation::CalculateFPSTimings()
{
	auto* Engine = GEngine;
	auto GameViewport = Engine ? Engine->GameViewport : nullptr;
	auto Viewport = GameViewport ? GameViewport->Viewport : nullptr;
	auto ViewportRHI = Viewport ? Viewport->GetViewportRHI() : nullptr;
	FFXFrameInterpolationCustomPresent* Presenter = ViewportRHI.IsValid() ? (FFXFrameInterpolationCustomPresent*)ViewportRHI->GetCustomPresent() : nullptr;
	if (CVarEnableFFXFI.GetValueOnAnyThread() != 0 && Presenter && Presenter->GetMode() == EFFXFrameInterpolationPresentModeRHI)
	{
		double CurrentTime = FPlatformTime::Seconds();
		float FrameTimeMS = (float)((CurrentTime - LastTime) * 1000.0);
		AverageTime = AverageTime * 0.75f + FrameTimeMS * 0.25f;
		LastTime = CurrentTime;
		AverageFPS = 1000.f / AverageTime;

		if (CVarFFXFIUpdateGlobalFrameTime.GetValueOnAnyThread() != 0)
		{
			GAverageMS = AverageTime;
			GAverageFPS = AverageFPS;
		}
	}
}

void FFXFrameInterpolation::OnPostEngineInit()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication& App = FSlateApplication::Get();

		// Has to be used by all backends as otherwise we end up waiting on DrawBuffers.
		{
			FSlateApplicationBase& BaseApp = static_cast<FSlateApplicationBase&>(App);
			FFXFISlateApplicationAccessor& Accessor = (FFXFISlateApplicationAccessor&)BaseApp;
			TSharedPtr<FSlateRenderer>* Ptr = &Accessor.Renderer;
			auto SharedRef = Ptr->ToSharedRef();
			TSharedRef<FFXFrameInterpolationSlateRenderer> RendererWrapper = MakeShared<FFXFrameInterpolationSlateRenderer>(SharedRef);
			App.InitializeRenderer(RendererWrapper, true);
		}
		
		FSlateRenderer* SlateRenderer = App.GetRenderer();
		SlateRenderer->OnSlateWindowRendered().AddRaw(const_cast<FFXFrameInterpolation*>(this), &FFXFrameInterpolation::OnSlateWindowRendered);
		SlateRenderer->OnBackBufferReadyToPresent().AddRaw(const_cast<FFXFrameInterpolation*>(this), &FFXFrameInterpolation::OnBackBufferReadyToPresentCallback);
		GEngine->GetPostRenderDelegateEx().AddRaw(const_cast<FFXFrameInterpolation*>(this), &FFXFrameInterpolation::InterpolateFrame);

		FFXFrameInterpolation* Self = this;
		FCoreDelegates::OnBeginFrame.AddLambda([Self]()
		{
			ENQUEUE_RENDER_COMMAND(BeginFrameRT)([Self](FRHICommandListImmediate& RHICmdList)
			{
				Self->CalculateFPSTimings();
			});
		});

		ViewExtension = FSceneViewExtensions::NewExtension<FFXFrameInterpolationViewExtension>(this);
	}
}

void FFXFrameInterpolation::SetupView(const FSceneView& InView, const FPostProcessingInputs& Inputs)
{
	if (InView.bIsViewInfo)
	{
		FFXFrameInterpolationView View;
		View.ViewFamilyTexture = Inputs.ViewFamilyTexture;
#if UE_VERSION_AT_LEAST(5, 0, 0)
		View.SceneDepth = Inputs.SceneTextures->GetContents()->SceneDepthTexture;
		View.SceneVelocity = Inputs.SceneTextures->GetContents()->GBufferVelocityTexture;
		View.CameraNear = InView.ViewMatrices.ComputeNearPlane();
#else
		View.SceneDepth = (*Inputs.SceneTextures)->SceneDepthTexture;
		View.SceneVelocity = (*Inputs.SceneTextures)->GBufferVelocityTexture;
		View.CameraNear = GNearClippingPlane;
#endif
		View.ViewRect = ((FViewInfo const&)InView).ViewRect;
		View.InputExtentsQuantized = View.ViewRect.Size();
		QuantizeSceneBufferSize(((FViewInfo const&)InView).GetSecondaryViewRectSize(), View.OutputExtents);
		View.OutputExtents = FIntPoint(FMath::Max(View.InputExtentsQuantized.X, View.OutputExtents.X), FMath::Max(View.InputExtentsQuantized.Y, View.OutputExtents.Y));
		View.bReset = InView.bCameraCut;
		View.CameraFOV = InView.ViewMatrices.ComputeHalfFieldOfViewPerAxis().Y * 2.0f;
#if UE_VERSION_AT_LEAST(5, 1, 0)
		View.bEnabled = InView.bIsGameView && !InView.bIsSceneCapture && !InView.bIsSceneCaptureCube && !InView.bIsReflectionCapture && !InView.bIsPlanarReflection;
#else
		View.bEnabled = InView.bIsGameView && !InView.bIsSceneCapture && !InView.bIsReflectionCapture && !InView.bIsPlanarReflection;
#endif
		View.TemporalJitterPixels = ((FViewInfo const&)InView).TemporalJitterPixels;
#if UE_VERSION_AT_LEAST(5, 0, 0)
		if (View.bEnabled && (InView.GetFeatureLevel() >= ERHIFeatureLevel::SM6))
		{
			View.GameTimeMs = InView.Family->Time.GetDeltaWorldTimeSeconds();
			GameDeltaTime = InView.Family->Time.GetDeltaWorldTimeSeconds();
#else
		if (View.bEnabled)
		{
			GameDeltaTime = InView.Family->DeltaWorldTime;
#endif
			Views.Add(&InView, View);
		}
	}
}

static FfxCommandList GCommandList = nullptr;

#if UE_VERSION_OLDER_THAN(5, 1, 0)
enum class EDisplayOutputFormat : uint8
{
	SDR_sRGB,
	SDR_Rec709,
	SDR_ExplicitGammaMapping,
	HDR_ACES_1000nit_ST2084,
	HDR_ACES_2000nit_ST2084,
	HDR_ACES_1000nit_ScRGB,
	HDR_ACES_2000nit_ScRGB,
};
#endif

static uint32_t GetFfxTransferFunction(EDisplayOutputFormat UEFormat)
{
	uint32_t Output = FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SRGB;
	switch (UEFormat)
	{
		// Gamma ST.2084
	case EDisplayOutputFormat::HDR_ACES_1000nit_ST2084:
	case EDisplayOutputFormat::HDR_ACES_2000nit_ST2084:
		Output = FFX_API_BACKBUFFER_TRANSFER_FUNCTION_PQ;
		break;

		// Gamma 1.0 (Linear)
	case EDisplayOutputFormat::HDR_ACES_1000nit_ScRGB:
	case EDisplayOutputFormat::HDR_ACES_2000nit_ScRGB:
		// Linear. Still supports expanded color space with values >1.0f and <0.0f.
		// The actual range is determined by the pixel format (e.g. a UNORM format can only ever have 0-1).
		Output = FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SCRGB;
		break;
	
		// Gamma 2.2
	case EDisplayOutputFormat::SDR_sRGB:
	case EDisplayOutputFormat::SDR_Rec709:
		Output = FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SRGB;
		break;

		// Unsupported types that require modifications to the FidelityFX code in order to support
	case EDisplayOutputFormat::SDR_ExplicitGammaMapping:
#if UE_VERSION_AT_LEAST(5, 1, 0)
	case EDisplayOutputFormat::HDR_LinearEXR:
	case EDisplayOutputFormat::HDR_LinearNoToneCurve:
	case EDisplayOutputFormat::HDR_LinearWithToneCurve:
#endif
	default:
		check(false);
		Output = FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SRGB;
		break;
	}

	return Output;
}

bool FFXFrameInterpolation::InterpolateView(FRDGBuilder& GraphBuilder, FFXFrameInterpolationCustomPresent* Presenter, const FSceneView* View, FFXFrameInterpolationView const& ViewDesc, FRDGTextureRef FinalBuffer, FRDGTextureRef InterpolatedRDG, FRDGTextureRef BackBufferRDG, uint32 InterpolateIndex)
{
	bool bInterpolated = false;
	auto* Engine = GEngine;
	auto GameViewport = Engine ? Engine->GameViewport : nullptr;
	auto Viewport = GameViewport ? GameViewport->Viewport : nullptr;
	auto ViewportRHI = Viewport ? Viewport->GetViewportRHI() : nullptr;
	FIntPoint ViewportSizeXY = Viewport ? Viewport->GetSizeXY() : FIntPoint::ZeroValue;

	FRDGTextureRef ViewFamilyTexture = ViewDesc.ViewFamilyTexture;
	FIntRect ViewRect = ViewDesc.ViewRect;
	FIntPoint InputExtents = ViewDesc.ViewRect.Size();
	FIntPoint InputExtentsQuantized = ViewDesc.InputExtentsQuantized;
	FIntPoint InputTextureExtents = CVarFSR3QuantizeInternalTextures.GetValueOnRenderThread() ? InputExtentsQuantized : InputExtents;
	FIntPoint OutputExtents = ((FViewInfo*)View)->UnscaledViewRect.Size();
	FIntPoint OutputPoint = ((FViewInfo*)View)->UnscaledViewRect.Min;
	float CameraNear = ViewDesc.CameraNear;
	float CameraFOV = ViewDesc.CameraFOV;
	bool bEnabled = ViewDesc.bEnabled;
	bool bReset = ViewDesc.bReset || (ResetState == 0);
	bool const bResized = Presenter->Resized();
	float DeltaTimeMs = GameDeltaTime * 1000.f;
	FRHICopyTextureInfo Info;

	ffxDispatchDescFrameGenerationPrepare UpscalerDesc;
	FMemory::Memzero(UpscalerDesc);
	UpscalerDesc.header.type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE;
#if UE_VERSION_AT_LEAST(5, 3, 0)
	UpscalerDesc.frameID = View->Family->FrameCounter;
#else
	UpscalerDesc.frameID = GFrameCounterRenderThread;
#endif
#if UE_VERSION_AT_LEAST(5, 0, 0)
	UpscalerDesc.frameTimeDelta = View->Family->Time.GetDeltaWorldTimeSeconds() * 1000.f;
#else
	UpscalerDesc.frameTimeDelta = DeltaTimeMs;
#endif
	if (bool(ERHIZBuffer::IsInverted))
	{
		UpscalerDesc.cameraNear = FLT_MAX;
		UpscalerDesc.cameraFar = CameraNear;
	}
	else
	{
		UpscalerDesc.cameraNear = CameraNear;
		UpscalerDesc.cameraFar = FLT_MAX;
	}
	UpscalerDesc.cameraFovAngleVertical = CameraFOV;
	UpscalerDesc.viewSpaceToMetersFactor = 1.f / View->WorldToMetersScale;

	UpscalerDesc.jitterOffset.x = ((FViewInfo*)View)->TemporalJitterPixels.X;
	UpscalerDesc.jitterOffset.y = ((FViewInfo*)View)->TemporalJitterPixels.Y;

	UpscalerDesc.renderSize.width = InputExtents.X;
	UpscalerDesc.renderSize.height = InputExtents.Y;
	UpscalerDesc.motionVectorScale.x = InputExtents.X;
	UpscalerDesc.motionVectorScale.y = InputExtents.Y;

	FIntPoint MaxRenderSize = ((FViewInfo*)View)->GetSecondaryViewRectSize();

	ffxCreateContextDescFrameGeneration FgDesc;
	FMemory::Memzero(FgDesc);
	FgDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;
	FgDesc.backBufferFormat = GetFFXApiFormat(BackBufferRDG->Desc.Format, false);
	FgDesc.displaySize.width = FMath::Max((uint32)MaxRenderSize.X, (uint32)OutputExtents.X);
	FgDesc.displaySize.height = FMath::Max((uint32)MaxRenderSize.Y, (uint32)OutputExtents.Y);
	FgDesc.maxRenderSize.width = MaxRenderSize.X;
	FgDesc.maxRenderSize.height = MaxRenderSize.Y;
	FgDesc.flags |= (bool(ERHIZBuffer::IsInverted)) ? FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED : 0;
	FgDesc.flags |= FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE;
	FgDesc.flags |= CVarFSR3AllowAsyncWorkloads.GetValueOnAnyThread() ? FFX_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT : 0;

	FRDGTextureRef ColorBuffer = FinalBuffer;
	FRDGTextureRef InterBuffer = InterpolatedRDG;
	FRDGTextureRef HudBuffer = nullptr;
	FFXFIResourceRef Context = Presenter->UpdateContexts(GraphBuilder, ((FSceneViewState*)View->State)->UniqueID, UpscalerDesc, FgDesc);

	//------------------------------------------------------------------------------------------------------
	// Consolidate Motion Vectors
	//   UE motion vectors are in sparse format by default.  Convert them to a format consumable by FFX.
	//------------------------------------------------------------------------------------------------------
	if (!IsValidRef(Context->MotionVectorRT) || Context->MotionVectorRT->GetDesc().Extent.X != InputTextureExtents.X || Context->MotionVectorRT->GetDesc().Extent.Y != InputTextureExtents.Y)
	{
#if UE_VERSION_AT_LEAST(5, 0, 0)
		ETextureCreateFlags DescFlags = TexCreate_ShaderResource | TexCreate_UAV;
#else
		ETextureCreateFlags DescFlags = TexCreate_ShaderResource;
#endif
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(InputTextureExtents,
			PF_G16R16F,
			FClearValueBinding::Transparent,
			DescFlags,
			TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable,
			false));
		GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, Context->MotionVectorRT, TEXT("FFXFIMotionVectorTexture"));
	}

	FRDGTextureRef MotionVectorTexture = GraphBuilder.RegisterExternalTexture(Context->MotionVectorRT);
	{
		FFXFIConvertVelocityCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FFXFIConvertVelocityCS::FParameters>();
		FRDGTextureUAVDesc OutputDesc(MotionVectorTexture);

		FRDGTextureSRVDesc DepthDesc = FRDGTextureSRVDesc::Create(ViewDesc.SceneDepth);
		FRDGTextureSRVDesc VelocityDesc = FRDGTextureSRVDesc::Create(ViewDesc.SceneVelocity);

		PassParameters->DepthTexture = ViewDesc.SceneDepth;
		PassParameters->InputDepth = GraphBuilder.CreateSRV(DepthDesc);
		PassParameters->InputVelocity = GraphBuilder.CreateSRV(VelocityDesc);

		PassParameters->View = ((FViewInfo*)View)->ViewUniformBuffer;

		PassParameters->OutputTexture = GraphBuilder.CreateUAV(OutputDesc);

		TShaderMapRef<FFXFIConvertVelocityCS> ComputeShaderFSR(((FViewInfo*)View)->ShaderMap);
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("FidelityFX-FI/ConvertVelocity (CS)"),
			ComputeShaderFSR,
			PassParameters,
			FComputeShaderUtils::GetGroupCount(FIntVector(ViewDesc.SceneDepth->Desc.Extent.X, ViewDesc.SceneDepth->Desc.Extent.Y, 1),
				FIntVector(FFXFIConvertVelocityCS::ThreadgroupSizeX, FFXFIConvertVelocityCS::ThreadgroupSizeY, FFXFIConvertVelocityCS::ThreadgroupSizeZ))
		);
	}



	FRDGTextureRef DistortionTexture = nullptr;
#if UE_VERSION_AT_LEAST(5, 1, 0)
	if (CVarFFXFIUseDistortionTexture.GetValueOnRenderThread()) {
		FFXFIRDGBuilder& GraphBulderAccessor = (FFXFIRDGBuilder&)GraphBuilder;
		FRDGTextureRef UEDistortionTexture = GraphBulderAccessor.FindTexture(TEXT("Distortion"));

		if (HasBeenProduced(UEDistortionTexture)) {
			FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(InputTextureExtents, PF_G16R16F, FClearValueBinding::Transparent, TexCreate_ShaderResource | TexCreate_UAV);

			DistortionTexture = GraphBuilder.CreateTexture(Desc, TEXT("FFXFIDistortionTexture"));

			FFXFIConvertDistortionCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FFXFIConvertDistortionCS::FParameters>();

			PassParameters->View = ((FViewInfo*)View)->ViewUniformBuffer;

			PassParameters->UEDistortion = GraphBuilder.CreateSRV(UEDistortionTexture);
			PassParameters->OutputTexture = GraphBuilder.CreateUAV(DistortionTexture);

			TShaderMapRef<FFXFIConvertDistortionCS> ComputeShaderFSR(((FViewInfo*)View)->ShaderMap);
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("FidelityFX-FI/ConvertDistortion (CS)"),
				ComputeShaderFSR,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(FIntVector(ViewDesc.SceneDepth->Desc.Extent.X, ViewDesc.SceneDepth->Desc.Extent.Y, 1), 
					FIntVector(FFXFIConvertDistortionCS::ThreadgroupSizeX, FFXFIConvertDistortionCS::ThreadgroupSizeY, FFXFIConvertDistortionCS::ThreadgroupSizeZ))
			);
		}
	}
#endif

	if (Context->Desc.displaySize.width != ViewportSizeXY.X || Context->Desc.displaySize.height != ViewportSizeXY.Y)
	{
		if (!IsValidRef(Context->Color) || Context->Color->GetDesc().Extent.X != Context->Desc.displaySize.width || Context->Color->GetDesc().Extent.Y != Context->Desc.displaySize.height || Context->Color->GetDesc().Format != BackBufferRDG->Desc.Format)
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(Context->Desc.displaySize.width, Context->Desc.displaySize.height),
				BackBufferRDG->Desc.Format,
				FClearValueBinding::Transparent,
				TexCreate_UAV | TexCreate_ShaderResource,
				TexCreate_UAV | TexCreate_ShaderResource,
				false,
				1,
				true,
				true));
			GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, Context->Color, TEXT("FIColor"));
			GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, Context->Inter, TEXT("FIInter"));

			if ((Presenter->GetBackend()->GetAPI() != EFFXBackendAPI::Unreal) && (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative))
			{
				GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, Context->Hud, TEXT("FIHud"));
			}
		}

		FRHICopyTextureInfo CopyInfo;
		ColorBuffer = GraphBuilder.RegisterExternalTexture(Context->Color);
		CopyInfo.SourcePosition.X = OutputPoint.X;
		CopyInfo.SourcePosition.Y = OutputPoint.Y;
		CopyInfo.Size.X = FMath::Min((uint32)Context->Desc.displaySize.width, (uint32)FinalBuffer->Desc.Extent.X);
		CopyInfo.Size.Y = FMath::Min((uint32)Context->Desc.displaySize.height, (uint32)FinalBuffer->Desc.Extent.Y);
		AddCopyTexturePass(GraphBuilder, FinalBuffer, ColorBuffer, CopyInfo);

		if ((Presenter->GetBackend()->GetAPI() != EFFXBackendAPI::Unreal) && (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative))
		{
			HudBuffer = GraphBuilder.RegisterExternalTexture(Context->Hud);
			AddCopyTexturePass(GraphBuilder, BackBufferRDG, HudBuffer, CopyInfo);
		}

		InterBuffer = GraphBuilder.RegisterExternalTexture(Context->Inter);

		FRDGTextureUAVDesc Interpolatedesc(InterBuffer);
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Interpolatedesc), FVector::ZeroVector);
	}

	FFXFrameInterpolationPass::FParameters* PassParameters = GraphBuilder.AllocParameters<FFXFrameInterpolationPass::FParameters>();
	PassParameters->ColorTexture = ColorBuffer;
	PassParameters->BackBufferTexture = BackBufferRDG;
	PassParameters->HudTexture = HudBuffer;
	PassParameters->InterpolatedRT = InterBuffer;
	PassParameters->Interpolated = InterpolatedRDG;
	PassParameters->SceneDepth = ViewDesc.SceneDepth;
	PassParameters->MotionVectors = MotionVectorTexture;

	static const auto CVarHDRMinLuminanceLog10 = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.HDR.Display.MinLuminanceLog10"));
	static const auto CVarHDRMaxLuminance = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.HDR.Display.MaxLuminance"));

	float GHDRMinLuminnanceLog10 = CVarHDRMinLuminanceLog10 ? CVarHDRMinLuminanceLog10->GetValueOnAnyThread() : 0.f;
	int32 GHDRMaxLuminnance = CVarHDRMaxLuminance ? CVarHDRMaxLuminance->GetValueOnAnyThread() : 1;
#if UE_VERSION_AT_LEAST(5, 1, 0)
	EDisplayOutputFormat ViewportOutputFormat = Viewport->GetDisplayOutputFormat();
#else
	static const auto CVarHDROutputDevice = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.HDR.Display.OutputDevice"));
	static const auto CVarHDROutputEnabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.HDR.EnableHDROutput"));
	const EDisplayOutputFormat ViewportOutputFormat = (CVarHDROutputDevice && GRHISupportsHDROutput && (CVarHDROutputEnabled && CVarHDROutputEnabled->GetValueOnAnyThread() != 0)) ? EDisplayOutputFormat(CVarHDROutputDevice->GetValueOnAnyThread()) : EDisplayOutputFormat::SDR_sRGB;
	if ((GHDRMaxLuminnance == 0) && (ViewportOutputFormat == EDisplayOutputFormat::HDR_ACES_1000nit_ST2084 || ViewportOutputFormat == EDisplayOutputFormat::HDR_ACES_2000nit_ST2084 || ViewportOutputFormat == EDisplayOutputFormat::HDR_ACES_1000nit_ScRGB || ViewportOutputFormat == EDisplayOutputFormat::HDR_ACES_2000nit_ScRGB))
	{
		if (ViewportOutputFormat == EDisplayOutputFormat::HDR_ACES_1000nit_ST2084 || ViewportOutputFormat == EDisplayOutputFormat::HDR_ACES_1000nit_ScRGB)
		{
			GHDRMaxLuminnance = 1000;
		}
		else
		{
			GHDRMaxLuminnance = 2000;
		}
	}
#endif

	// compute how many VSync intervals interpolated and real frame should be displayed
	ffxDispatchDescFrameGeneration* interpolateParams = new ffxDispatchDescFrameGeneration;
	{
		interpolateParams->header.type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION;
		interpolateParams->header.pNext = nullptr;
		interpolateParams->numGeneratedFrames = 1;
#if UE_VERSION_AT_LEAST(5, 3, 0)
		interpolateParams->frameID = View->Family->FrameCounter;
#else
		interpolateParams->frameID = GFrameCounterRenderThread;
#endif
    	interpolateParams->backbufferTransferFunction = GetFfxTransferFunction(ViewportOutputFormat);
		interpolateParams->generationRect = {0 ,0, (int32_t)Context->Desc.displaySize.width, (int32_t)Context->Desc.displaySize.height};
		interpolateParams->reset = bReset;

		interpolateParams->minMaxLuminance[0] = interpolateParams->backbufferTransferFunction != FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SRGB ? FMath::Pow(10, GHDRMinLuminnanceLog10) : 0.f;
		interpolateParams->minMaxLuminance[1] = interpolateParams->backbufferTransferFunction != FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SRGB ? GHDRMaxLuminnance : 1.f;
	}

	auto DisplaySize = ColorBuffer->Desc.Extent;
	bool const bOverrideSwapChain = ((CVarFSR3OverrideSwapChainDX12.GetValueOnAnyThread() != 0) || FParse::Param(FCommandLine::Get(), TEXT("fsr3swapchain")));

	ffxConfigureDescFrameGeneration ConfigDesc;
	FMemory::Memzero(ConfigDesc);
	ConfigDesc.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
	ConfigDesc.swapChain = Presenter->GetBackend()->GetSwapchain(ViewportRHI->GetNativeSwapChain());
	ConfigDesc.frameGenerationEnabled = true;
	ConfigDesc.allowAsyncWorkloads = (CVarFSR3AllowAsyncWorkloads.GetValueOnAnyThread() != 0);
	ConfigDesc.generationRect = interpolateParams->generationRect;
	ConfigDesc.frameID = interpolateParams->frameID;
	ConfigDesc.flags |= bOverrideSwapChain ? 0 : FFX_FRAMEGENERATION_FLAG_NO_SWAPCHAIN_CONTEXT_NOTIFY;
#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT || UE_BUILD_TEST)
	ConfigDesc.flags |= CVarFFXFIShowDebugTearLines.GetValueOnAnyThread() ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES : 0;
	ConfigDesc.flags |= CVarFFXFIShowDebugView.GetValueOnAnyThread() ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW : 0;
#endif

	if (Presenter->GetBackend()->GetAPI() == EFFXBackendAPI::Unreal)
	{
		bInterpolated = true;
		interpolateParams->presentColor = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture GET_TEXTURE, FFX_API_RESOURCE_STATE_COPY_DEST);

		Presenter->GetBackend()->SetFeatureLevel(&Context->Context, View->GetFeatureLevel());


		if (!HasBeenProduced(DistortionTexture)) {
			DistortionTexture = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
		}
		PassParameters->DistortionTexture = DistortionTexture;

		GraphBuilder.AddPass(RDG_EVENT_NAME("FidelityFX-FrameInterpolation"), PassParameters, ERDGPassFlags::Compute | ERDGPassFlags::NeverCull | ERDGPassFlags::Copy, [Presenter, PassParameters, Context, ConfigDesc](FRHICommandListImmediate& RHICmdList)
		{
			PassParameters->ColorTexture->MarkResourceAsUsed();
			PassParameters->InterpolatedRT->MarkResourceAsUsed();
			PassParameters->SceneDepth->MarkResourceAsUsed();
			PassParameters->MotionVectors->MarkResourceAsUsed();
			PassParameters->DistortionTexture->MarkResourceAsUsed();
			Context->Distortion = PassParameters->DistortionTexture->GetRHI();

			Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::InterpolateRT);
			RHICmdList.EnqueueLambda([Presenter, Context, ConfigDesc](FRHICommandListImmediate& cmd) mutable
			{
				if (Context->Distortion.IsValid())
				{
					ffxConfigureDescFrameGenerationRegisterDistortionFieldResource DistortionDesc;
					DistortionDesc.header.pNext = nullptr;
					DistortionDesc.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_REGISTERDISTORTIONRESOURCE;
					DistortionDesc.distortionField = Presenter->GetBackend()->GetNativeResource(Context->Distortion, FFX_API_RESOURCE_STATE_PIXEL_READ);
					Presenter->GetBackend()->UpdateSwapChain(&Context->Context, ConfigDesc, DistortionDesc);
				}
				else
				{
					Presenter->GetBackend()->UpdateSwapChain(&Context->Context, ConfigDesc);
				}

				Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::InterpolateRHI);
			});
		});


		// Interpolate the frame
		{
			interpolateParams->commandList = (void*)&GraphBuilder;
			interpolateParams->outputs[0] = Presenter->GetBackend()->GetNativeResource(PassParameters->InterpolatedRT GET_TEXTURE, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

			UpscalerDesc.commandList = interpolateParams->commandList;
			UpscalerDesc.depth = Presenter->GetBackend()->GetNativeResource(PassParameters->SceneDepth GET_TEXTURE, FFX_API_RESOURCE_STATE_COMPUTE_READ);
			UpscalerDesc.motionVectors = Presenter->GetBackend()->GetNativeResource(PassParameters->MotionVectors GET_TEXTURE, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

			auto Code = Presenter->GetBackend()->ffxDispatch(&Context->Context, &UpscalerDesc.header);
			check(Code == FFX_API_RETURN_OK);

			Code = Presenter->GetBackend()->ffxDispatch(&Context->Context, &interpolateParams->header);
			check(Code == FFX_API_RETURN_OK);

			Info.Size.X = DisplaySize.X;
			Info.Size.Y = DisplaySize.Y;
			if (PassParameters->Interpolated != PassParameters->InterpolatedRT)
			{
				Info.DestPosition.X = OutputPoint.X;
				Info.DestPosition.Y = OutputPoint.Y;
				Info.Size.X = FMath::Min((uint32)DisplaySize.X, (uint32)PassParameters->Interpolated->Desc.Extent.X);
				Info.Size.Y = FMath::Min((uint32)DisplaySize.Y, (uint32)PassParameters->Interpolated->Desc.Extent.Y);
#if UE_VERSION_AT_LEAST(5, 0, 0)
				AddCopyTexturePass(GraphBuilder, PassParameters->InterpolatedRT, PassParameters->Interpolated, Info);
				AddCopyTexturePass(GraphBuilder, PassParameters->InterpolatedRT, BackBufferRDG, Info);
#else
				AddCopyTexturePass(GraphBuilder, PassParameters->InterpolatedRT.GetTexture(), PassParameters->Interpolated.GetTexture(), Info);
				AddCopyTexturePass(GraphBuilder, PassParameters->InterpolatedRT.GetTexture(), BackBufferRDG, Info);
#endif
			}
			else
			{
				check(Info.Size.X == BackBufferRDG->Desc.Extent.X && Info.Size.Y == BackBufferRDG->Desc.Extent.Y);
				check(Info.Size.X == PassParameters->InterpolatedRT->Desc.Extent.X && Info.Size.Y == PassParameters->InterpolatedRT->Desc.Extent.Y);
#if UE_VERSION_AT_LEAST(5, 0, 0)
				AddCopyTexturePass(GraphBuilder, PassParameters->InterpolatedRT, BackBufferRDG, Info);
#else
				AddCopyTexturePass(GraphBuilder, PassParameters->InterpolatedRT.GetTexture(), BackBufferRDG, Info);
#endif
			}

			delete interpolateParams;
		}
	}
	else if (!bResized)
	{
		bInterpolated = true;

		if (HasBeenProduced(DistortionTexture)) {
			PassParameters->DistortionTexture = DistortionTexture;
		}

		GraphBuilder.AddPass(RDG_EVENT_NAME("FidelityFX-FrameInterpolation"), PassParameters, ERDGPassFlags::Compute | ERDGPassFlags::NeverCull | ERDGPassFlags::Copy, [InterpolateIndex, UpscalerDesc, ConfigDesc, OutputExtents, OutputPoint, ViewportRHI, Presenter, Context, PassParameters, interpolateParams, DeltaTimeMs, Engine, ViewportOutputFormat, GHDRMinLuminnanceLog10, GHDRMaxLuminnance](FRHICommandListImmediate& RHICmdList)
		{
			PassParameters->ColorTexture->MarkResourceAsUsed();
			PassParameters->InterpolatedRT->MarkResourceAsUsed();
			if (PassParameters->HudTexture)
			{
				PassParameters->HudTexture->MarkResourceAsUsed();
			}
			PassParameters->SceneDepth->MarkResourceAsUsed();
			PassParameters->MotionVectors->MarkResourceAsUsed();
			if (PassParameters->DistortionTexture)
			{
				PassParameters->DistortionTexture->MarkResourceAsUsed();
				Context->Distortion = PassParameters->DistortionTexture->GetRHI();
			}
			else
			{
				Context->Distortion.SafeRelease();
			}

			bool const bWholeScreen = (PassParameters->Interpolated.GetTexture() == PassParameters->InterpolatedRT.GetTexture());
			ffxConfigureDescFrameGeneration ConfigureDesc = ConfigDesc;

			if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
			{
#if UE_VERSION_AT_LEAST(5, 0, 0)
				ConfigureDesc.HUDLessColor = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture, FFX_API_RESOURCE_STATE_COPY_DEST);
#else
				ConfigureDesc.HUDLessColor = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture.GetTexture(), FFX_API_RESOURCE_STATE_COPY_DEST);
#endif
				interpolateParams->presentColor = Presenter->GetBackend()->GetNativeResource(bWholeScreen ? PassParameters->BackBufferTexture.GetTexture() : PassParameters->HudTexture.GetTexture(), bWholeScreen ? FFX_API_RESOURCE_STATE_PRESENT : FFX_API_RESOURCE_STATE_COPY_DEST);
			}
			else
			{
#if UE_VERSION_AT_LEAST(5, 0, 0)
				interpolateParams->presentColor = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture, FFX_API_RESOURCE_STATE_COPY_DEST);
#else
				interpolateParams->presentColor = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture.GetTexture(), FFX_API_RESOURCE_STATE_COPY_DEST);
#endif
			}

			if (InterpolateIndex != 0)
			{
				ConfigureDesc.swapChain = nullptr;
				ConfigureDesc.presentCallback = nullptr;
				ConfigureDesc.presentCallbackUserContext = nullptr;
				ConfigureDesc.frameGenerationCallback = nullptr;
				ConfigureDesc.frameGenerationCallbackUserContext = nullptr;
				ConfigureDesc.flags |= FFX_FRAMEGENERATION_FLAG_NO_SWAPCHAIN_CONTEXT_NOTIFY;
			}

			ffxDispatchDescFrameGenerationPrepare PrepareDesc = UpscalerDesc;
#if UE_VERSION_AT_LEAST(5, 0, 0)
			PrepareDesc.depth = Presenter->GetBackend()->GetNativeResource(PassParameters->SceneDepth, FFX_API_RESOURCE_STATE_COMPUTE_READ);
			PrepareDesc.motionVectors = Presenter->GetBackend()->GetNativeResource(PassParameters->MotionVectors, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

			auto InterpolatedRes = Presenter->GetBackend()->GetNativeResource(PassParameters->InterpolatedRT, Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative || !bWholeScreen || InterpolateIndex == 0 ? FFX_API_RESOURCE_STATE_UNORDERED_ACCESS : FFX_API_RESOURCE_STATE_COPY_SRC);
#else
			PrepareDesc.depth = Presenter->GetBackend()->GetNativeResource(PassParameters->SceneDepth.GetTexture(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
			PrepareDesc.motionVectors = Presenter->GetBackend()->GetNativeResource(PassParameters->MotionVectors.GetTexture(), FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

			auto InterpolatedRes = Presenter->GetBackend()->GetNativeResource(PassParameters->InterpolatedRT.GetTexture(), Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative || !bWholeScreen || InterpolateIndex == 0 ? FFX_API_RESOURCE_STATE_UNORDERED_ACCESS : FFX_API_RESOURCE_STATE_COPY_SRC);
#endif
			Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::InterpolateRT);
			RHICmdList.EnqueueLambda([PrepareDesc, ConfigureDesc, ViewportRHI, Presenter, Context, InterpolatedRes, interpolateParams, OutputExtents, OutputPoint, bWholeScreen, ViewportOutputFormat, GHDRMinLuminnanceLog10, GHDRMaxLuminnance](FRHICommandListImmediate& cmd) mutable
			{
				if (Context->Distortion.IsValid())
				{
					ffxConfigureDescFrameGenerationRegisterDistortionFieldResource DistortionDesc;
					DistortionDesc.header.pNext = nullptr;
					DistortionDesc.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_REGISTERDISTORTIONRESOURCE;
					DistortionDesc.distortionField = Presenter->GetBackend()->GetNativeResource(Context->Distortion, FFX_API_RESOURCE_STATE_PIXEL_READ);
					Presenter->GetBackend()->UpdateSwapChain(&Context->Context, ConfigureDesc, DistortionDesc);
				}
				else
				{
					Presenter->GetBackend()->UpdateSwapChain(&Context->Context, ConfigureDesc);
				}

				Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::InterpolateRHI);
				if ((Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative) || Presenter->GetUseFFXSwapchain())
				{
					Presenter->GetBackend()->RegisterFrameResources(Context.GetReference(), ConfigureDesc.frameID);
				}
				
				FfxCommandList CmdBuffer = nullptr;
				if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
				{
					if (!GCommandList)
					{
						GCommandList = Presenter->GetBackend()->GetInterpolationCommandList(Presenter->GetBackend()->GetSwapchain(ViewportRHI->GetNativeSwapChain()));
					}
					CmdBuffer = GCommandList;
				}
				else
				{
					CmdBuffer = Presenter->GetBackend()->GetNativeCommandBuffer(cmd, Context->MotionVectorRT->GetRHI(GET_RHI_VIEW_ARG));
				}
				if (CmdBuffer)
				{
					// Prepare the interpolation context on the current RHI command list
					{
						ffxDispatchDescFrameGenerationPrepare UpscalerDesc = PrepareDesc;
						UpscalerDesc.commandList = Presenter->GetBackend()->GetNativeCommandBuffer(cmd, Context->MotionVectorRT->GetRHI(GET_RHI_VIEW_ARG));

						auto Code = Presenter->GetBackend()->ffxDispatch(&Context->Context, &UpscalerDesc.header);
						check(Code == FFX_API_RETURN_OK);
					}

					// Interpolate the frame
					{
						FfxApiResource OutputRes = Presenter->GetBackend()->GetInterpolationOutput(Presenter->GetBackend()->GetSwapchain(ViewportRHI->GetNativeSwapChain()));
						interpolateParams->outputs[0] = (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative && bWholeScreen) ? OutputRes : InterpolatedRes;
						interpolateParams->commandList = CmdBuffer;

						auto Code = Presenter->GetBackend()->ffxDispatch(&Context->Context, &interpolateParams->header);
						check(Code == FFX_API_RETURN_OK);

                        if (!bWholeScreen && (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative))
                        {
							Presenter->GetBackend()->CopySubRect(CmdBuffer, InterpolatedRes, OutputRes, OutputExtents, OutputPoint);
                        }
					}
				}
				delete interpolateParams;
			});

			Presenter->GetBackend()->Flush(Context->MotionVectorRT->GetRHI(GET_RHI_VIEW_ARG), RHICmdList);

			if (Presenter->GetMode() != EFFXFrameInterpolationPresentModeNative)
			{
#if UE_VERSION_AT_LEAST(5, 2, 0)
				FTextureRHIRef BackBuffer = RHIGetViewportBackBuffer(ViewportRHI);
#else
				FTextureRHIRef BackBuffer = RHICmdList.GetViewportBackBuffer(ViewportRHI);
#endif

				if (PassParameters->Interpolated != PassParameters->InterpolatedRT)
				{
					FRHICopyTextureInfo CopyInfo;
					CopyInfo.DestPosition.X = OutputPoint.X;
					CopyInfo.DestPosition.Y = OutputPoint.Y;
					CopyInfo.Size.X = OutputExtents.X;
					CopyInfo.Size.Y = OutputExtents.Y;
					FTextureRHIRef InterpolatedFrame = PassParameters->InterpolatedRT->GetRHI();
					TransitionAndCopyTexture(RHICmdList, InterpolatedFrame, PassParameters->Interpolated->GetRHI(), CopyInfo);
					if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeRHI)
					{
						check(PassParameters->Interpolated->Desc.Extent == FIntPoint(BackBuffer->GetSizeXYZ().X, BackBuffer->GetSizeXYZ().Y));
						TransitionAndCopyTexture(RHICmdList, InterpolatedFrame, BackBuffer, CopyInfo);
					}
				}
				else
				{
					FTextureRHIRef InterpolatedFrame = PassParameters->InterpolatedRT->GetRHI();
					check(FIntPoint(InterpolatedFrame->GetSizeXYZ().X, InterpolatedFrame->GetSizeXYZ().Y) == FIntPoint(BackBuffer->GetSizeXYZ().X, BackBuffer->GetSizeXYZ().Y));
					TransitionAndCopyTexture(RHICmdList, InterpolatedFrame, BackBuffer, {});
				}
			}
		});
	}

	if (bInterpolated)
	{
		if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
		{
			FFX_RENDER_TEST_CAPTURE_PASS_BEGIN(TEXT("FFXFrameInterpolation-Native"), GraphBuilder, 0.05f);
				FFX_RENDER_TEST_CAPTURE_PASS_PARAM(TEXT("ColorTexture"), PassParameters->ColorTexture.GetTexture(), GraphBuilder, 0.05f);
				FFX_RENDER_TEST_CAPTURE_PASS_PARAM(TEXT("BackBufferTexture"), PassParameters->BackBufferTexture.GetTexture(), GraphBuilder, 0.05f);
				FFX_RENDER_TEST_CAPTURE_PASS_PARAM(TEXT("HudTexture"), PassParameters->HudTexture.GetTexture(), GraphBuilder, 0.05f);
				FFX_RENDER_TEST_CAPTURE_PASS_PARAM(TEXT("SceneDepth"), PassParameters->SceneDepth.GetTexture(), GraphBuilder, 0.05f);
				FFX_RENDER_TEST_CAPTURE_PASS_PARAM(TEXT("MotionVectors"), PassParameters->MotionVectors.GetTexture(), GraphBuilder, 0.05f);
			FFX_RENDER_TEST_CAPTURE_PASS_END(GraphBuilder)
		}
		else
		{
			FFX_RENDER_TEST_CAPTURE_PASS_BEGIN(TEXT("FFXFrameInterpolation-RHI"), GraphBuilder, 0.05f);
				FFX_RENDER_TEST_CAPTURE_PASS_PARAMS(FFXFrameInterpolationPass::FParameters, PassParameters, GraphBuilder, 0.05f);
			FFX_RENDER_TEST_CAPTURE_PASS_END(GraphBuilder)
		}
	}

	return bInterpolated;
}

#if UE_VERSION_OLDER_THAN(5, 0, 0)
inline static FRDGTextureRef RegisterExternalTexture(FRDGBuilder& GraphBuilder, FRHITexture* Texture, const TCHAR* NameIfUnregistered = nullptr)
{
	if (FRDGTextureRef FoundTexture = GraphBuilder.FindExternalTexture(Texture))
	{
		return FoundTexture;
	}

	return GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Texture, NameIfUnregistered));
}
#endif

void FFXFrameInterpolation::InterpolateFrame(FRDGBuilder& InGraphBuilder)
{
	auto* Engine = GEngine;
	auto GameViewport = Engine ? Engine->GameViewport : nullptr;
	auto Viewport = GameViewport ? GameViewport->Viewport : nullptr;
	auto ViewportRHI = Viewport ? Viewport->GetViewportRHI() : nullptr;
	FIntPoint ViewportSizeXY = Viewport ? Viewport->GetSizeXY() : FIntPoint::ZeroValue;
	FFXFrameInterpolationCustomPresent* Presenter = ViewportRHI.IsValid() ? (FFXFrameInterpolationCustomPresent*)ViewportRHI->GetCustomPresent() : nullptr;
	bool bAllowed = CVarEnableFFXFI.GetValueOnAnyThread() != 0 && Presenter && (Views.Num() > 0);
#if WITH_EDITORONLY_DATA
	bAllowed &= !GIsEditor;
#endif
#if UE_VERSION_AT_LEAST(5, 0, 0)		
	FRDGBuilder& GraphBuilder = InGraphBuilder;
#else
	if (IFFXSharedBackend::GetGraphBuilder() == nullptr)
	{
		bAllowed = false;
	}
#endif

	if (bAllowed)
	{
		FTextureRHIRef BackBuffer = RHIGetViewportBackBuffer(ViewportRHI);
#if UE_VERSION_AT_LEAST(5, 0, 0)		
		FRDGTextureRef BackBufferRDG = RegisterExternalTexture(GraphBuilder, BackBuffer, nullptr);
#else
		FRDGBuilder& GraphBuilder = *IFFXSharedBackend::GetGraphBuilder();
		FRDGTextureRef BackBufferRDG = RegisterExternalTexture(GraphBuilder, BackBuffer);
#endif

#if UE_VERSION_AT_LEAST(5, 0, 0)
		ETextureCreateFlags DescFlags = TexCreate_UAV;
#else
		ETextureCreateFlags DescFlags = TexCreate_None;
#endif

		if (!IsValidRef(BackBufferRT) || BackBufferRT->GetDesc().Extent.X != BackBufferRDG->Desc.Extent.X || BackBufferRT->GetDesc().Extent.Y != BackBufferRDG->Desc.Extent.Y || BackBufferRT->GetDesc().Format != BackBufferRDG->Desc.Format)
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(BackBufferRDG->Desc.Extent.X, BackBufferRDG->Desc.Extent.Y),
				BackBuffer->GetFormat(),
				FClearValueBinding::Transparent,
				DescFlags,
				TexCreate_UAV | TexCreate_ShaderResource,
				false,
				1,
				true,
				true));
			GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, BackBufferRT, TEXT("BackBufferRT"));
			GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, InterpolatedRT, TEXT("InterpolatedRT"));
		}

		if ((Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative) && (!IsValidRef(AsyncBufferRT[0]) || AsyncBufferRT[0]->GetDesc().Extent.X != BackBufferRDG->Desc.Extent.X || AsyncBufferRT[0]->GetDesc().Extent.Y != BackBufferRDG->Desc.Extent.Y || AsyncBufferRT[0]->GetDesc().Format != BackBufferRDG->Desc.Format))
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(BackBufferRDG->Desc.Extent.X, BackBufferRDG->Desc.Extent.Y),
				BackBuffer->GetFormat(),
				FClearValueBinding::Transparent,
				DescFlags,
				TexCreate_UAV | TexCreate_ShaderResource,
				false,
				1,
				true,
				true));
			GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, AsyncBufferRT[0], TEXT("AsyncBufferRT0"));
			GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, AsyncBufferRT[1], TEXT("AsyncBufferRT1"));
		}

		Presenter->BeginFrame();
		Presenter->SetPreUITextures(BackBufferRT, InterpolatedRT);
		Presenter->SetEnabled(true);

		FRHICopyTextureInfo Info;
		FRDGTextureRef FinalBuffer = GraphBuilder.RegisterExternalTexture(BackBufferRT);
		FRDGTextureRef AsyncBuffer = nullptr;
		FRDGTextureRef InterpolatedRDG = GraphBuilder.RegisterExternalTexture(InterpolatedRT);
		check(BackBufferRDG->Desc.Extent == FinalBuffer->Desc.Extent);
		AddCopyTexturePass(GraphBuilder, BackBufferRDG, FinalBuffer, Info);
		
		if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
		{
			AsyncBuffer = GraphBuilder.RegisterExternalTexture(AsyncBufferRT[Index]);
			AddCopyTexturePass(GraphBuilder, BackBufferRDG, AsyncBuffer, Info);
			FinalBuffer = AsyncBuffer;
			Index = (Index + 1) % 2;

			// Reset the state if the present counter falls behind the interpolation, this ensures that textures will get cleared before first use
			ResetState = (PresentCount >= InterpolationCount) ? ResetState : 0u;
		}

		FRDGTextureUAVDesc Interpolatedesc(InterpolatedRDG);
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Interpolatedesc), FVector::ZeroVector);

		bAllowed = false;
		uint32 InterpolateIndex = 0;
		for (auto Pair : Views)
		{
			if (Pair.Key->State)
			{
				if (Pair.Value.bEnabled && Pair.Value.ViewFamilyTexture && (ViewportSizeXY.X == Pair.Value.ViewFamilyTexture->Desc.Extent.X) && (ViewportSizeXY.Y == Pair.Value.ViewFamilyTexture->Desc.Extent.Y))
				{
					bool const bInterpolated = InterpolateView(GraphBuilder, Presenter, Pair.Key, Pair.Value, FinalBuffer, InterpolatedRDG, BackBufferRDG, InterpolateIndex);
					InterpolateIndex = bInterpolated ? InterpolateIndex +1 : InterpolateIndex;
					bAllowed |= bInterpolated;
				}
			}
		}

		Presenter->EndFrame();
	}

#if UE_VERSION_OLDER_THAN(5, 0, 0)		
	if (IFFXSharedBackend::GetGraphBuilder() == nullptr)
#endif
	{
#if UE_VERSION_OLDER_THAN(5, 0, 0)		
		FRDGBuilder& GraphBuilder = *IFFXSharedBackend::GetGraphBuilder();
#endif
		GraphBuilder.AddPass(
			RDG_EVENT_NAME("FidelityFX-FrameInterpolation Unset CommandList"),
			ERDGPassFlags::None | ERDGPassFlags::NeverCull,
			[](FRHICommandListImmediate& RHICmdList)
			{
				RHICmdList.EnqueueLambda([](FRHICommandListImmediate& cmd)
				{
					GCommandList = nullptr;
				});
			});
	}

	Views.Empty();

	bInterpolatedFrame |= bAllowed;
	if (Presenter && Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
	{
		// If the present count fell behind reset it - otherwise it will persist indefinitely
		PresentCount = (PresentCount >= InterpolationCount) ? PresentCount : (InterpolationCount + (bInterpolatedFrame ? 1llu : 0llu));
		InterpolationCount += bInterpolatedFrame ? 1llu : 0llu;
		ResetState = bInterpolatedFrame ? 2u : 0u;
	}
	else
	{
		ResetState = bInterpolatedFrame ? 3u : 0u;
	}

	IAntiLag2Module* AntiLag2Interface = (IAntiLag2Module*)FModuleManager::Get().GetModule(TEXT("AntiLag2"));
	if (AntiLag2Interface)
	{
		InGraphBuilder.AddPass(
			RDG_EVENT_NAME("FidelityFX-FrameInterpolation Set AntLag2 FrameGen Mode"),
			ERDGPassFlags::None | ERDGPassFlags::NeverCull,
			[AntiLag2Interface](FRHICommandListImmediate& RHICmdList)
			{
				RHICmdList.EnqueueLambda([AntiLag2Interface](FRHICommandListImmediate& cmd)
					{
						AntiLag2Interface->MarkEndOfFrameRendering();
					});
			});
	}
}

void FFXFrameInterpolation::OnSlateWindowRendered(SWindow& SlateWindow, void* ViewportRHIPtr)
{
    static bool bProcessing = false;

	FViewportRHIRef Viewport = *((FViewportRHIRef*)ViewportRHIPtr);
	FFXFrameInterpolationCustomPresent* PresentHandler = (FFXFrameInterpolationCustomPresent*)Viewport->GetCustomPresent();

	if (IsInGameThread() && PresentHandler && PresentHandler->Enabled() && CVarEnableFFXFI.GetValueOnAnyThread())
	{
		if (!bProcessing)
		{
			bProcessing = true;
			FSlateApplication& App = FSlateApplication::Get();
			TSharedPtr<SWindow> WindowPtr;
			TSharedPtr<SWidget> TestWidget = SlateWindow.AsShared();
			while (TestWidget && !WindowPtr.IsValid())
			{
				if (TestWidget->Advanced_IsWindow())
				{
					WindowPtr = StaticCastSharedPtr<SWindow>(TestWidget);
				}

				TestWidget = TestWidget->GetParentWidget();
			}

			Windows.Add(&SlateWindow, Viewport.GetReference());

			bool bDrawDebugView = false;
#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT || UE_BUILD_TEST)
			bDrawDebugView = CVarFFXFIShowDebugView.GetValueOnAnyThread() != 0;
#endif

			if (PresentHandler->GetMode() == EFFXFrameInterpolationPresentModeRHI && !bDrawDebugView)
			{
				ENQUEUE_RENDER_COMMAND(UpdateWindowBackBufferCommand)(
					[this, Viewport](FRHICommandListImmediate& RHICmdList)
					{
#if UE_VERSION_AT_LEAST(5, 2, 0)
						FTextureRHIRef BackBuffer = RHIGetViewportBackBuffer(Viewport);
#else
						FTextureRHIRef BackBuffer = RHICmdList.GetViewportBackBuffer(Viewport);
#endif
						FFXFrameInterpolationCustomPresent* Presenter = (FFXFrameInterpolationCustomPresent*)Viewport->GetCustomPresent();
						if (Presenter && BackBufferRT.IsValid())
						{
							this->CalculateFPSTimings();
							FTextureRHIRef InterpolatedFrame = BackBufferRT->GetRHI(GET_RHI_TARGET_ARG);
							{
#if UE_VERSION_AT_LEAST(5, 5, 0)
								SCOPED_DRAW_EVENT(RHICmdList, FFXFrameInterpolation::OnSlateWindowRendered)
#else
								RHICmdList.PushEvent(TEXT("FFXFrameInterpolation::OnSlateWindowRendered"), FColor::White);
#endif
								check(FIntPoint(InterpolatedFrame->GetSizeXYZ().X, InterpolatedFrame->GetSizeXYZ().Y) == FIntPoint(BackBuffer->GetSizeXYZ().X, BackBuffer->GetSizeXYZ().Y));
								TransitionAndCopyTexture(RHICmdList, InterpolatedFrame, BackBuffer, {});
#if UE_VERSION_OLDER_THAN(5, 5, 0)
								RHICmdList.PopEvent();
#endif
							}

							Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::PresentRT);
							RHICmdList.EnqueueLambda([this, Presenter](FRHICommandListImmediate& cmd) mutable
							{
								Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::PresentRHI);
							});
						}
				});

				double OldLastTickTime = 0.0;
				bool const bModifySlateDeltaTime = (CVarFFXFIModifySlateDeltaTime.GetValueOnAnyThread() != 0);
				if (bModifySlateDeltaTime)
				{
					OldLastTickTime = ((FFXFISlateApplication&)App).LastTickTime;
					((FFXFISlateApplication&)App).LastTickTime = (((FFXFISlateApplication&)App).CurrentTime);
				}

				// If we hold on to this and the viewport resizes during redrawing then bad things will happen.
				Viewport.SafeRelease();

				App.ForceRedrawWindow(WindowPtr.ToSharedRef());
				
				if (bModifySlateDeltaTime)
				{
					((FFXFISlateApplication&)App).LastTickTime = OldLastTickTime;
				}
			}
			bProcessing = false;
		}
	}
	else
	{
		ENQUEUE_RENDER_COMMAND(UpdateWindowBackBufferCommand)(
			[Viewport](FRHICommandListImmediate& RHICmdList)
			{
				FFXFrameInterpolationCustomPresent* Presenter = (FFXFrameInterpolationCustomPresent*)Viewport->GetCustomPresent();
				if (Presenter)
				{
					Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::PresentRT);
					RHICmdList.EnqueueLambda([Presenter](FRHICommandListImmediate& cmd) mutable
						{
							Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::PresentRHI);
						});
				}
		});
	}
}

void FFXFrameInterpolation::OnBackBufferReadyToPresentCallback(class SWindow& SlateWindow, const FTextureRHIRef& BackBuffer)
{
    /** Callback for when a backbuffer is ready for reading (called on render thread) */
	FRHIViewport** ViewportPtr = Windows.Find(&SlateWindow);
	FViewportRHIRef Viewport = ViewportPtr ? *ViewportPtr : nullptr;
    FFXFrameInterpolationCustomPresent* Presenter = Viewport ? (FFXFrameInterpolationCustomPresent*)Viewport->GetCustomPresent() : nullptr;

	ResetState = (ResetState > 0) ? ResetState - 1 : ResetState;

	if (Presenter && CVarEnableFFXFI.GetValueOnAnyThread())
	{
		PresentCount += (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative) ? 1llu : 0llu;
		if (ResetState)
		{
			Presenter->CopyBackBufferRT(BackBuffer);
		}
	}

	bInterpolatedFrame = false;
}