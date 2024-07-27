// This file is part of the FidelityFX Super Resolution 3.1 Unreal Engine Plugin.
//
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
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

//------------------------------------------------------------------------------------------------------
// Helper variable declarations.
//------------------------------------------------------------------------------------------------------
static uint32_t s_opticalFlowBlockSize = 8;
static uint32_t s_opticalFlowSearchRadius = 8;

extern ENGINE_API float GAverageFPS;
extern ENGINE_API float GAverageMS;

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
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXFIConvertVelocityCS, "/Plugin/FSR3/Private/PostProcessFFX_FSR3ConvertVelocity.usf", "MainCS", SF_Compute);

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
		View.SceneDepth = Inputs.SceneTextures->GetContents()->SceneDepthTexture;
		View.SceneVelocity = Inputs.SceneTextures->GetContents()->GBufferVelocityTexture;
		View.ViewRect = ((FViewInfo const&)InView).ViewRect;
		View.InputExtentsQuantized = View.ViewRect.Size();
		QuantizeSceneBufferSize(((FViewInfo const&)InView).GetSecondaryViewRectSize(), View.OutputExtents);
		View.OutputExtents = FIntPoint(FMath::Max(View.InputExtentsQuantized.X, View.OutputExtents.X), FMath::Max(View.InputExtentsQuantized.Y, View.OutputExtents.Y));
		View.bReset = InView.bCameraCut;
		View.CameraNear = InView.ViewMatrices.ComputeNearPlane();
		View.CameraFOV = InView.ViewMatrices.ComputeHalfFieldOfViewPerAxis().Y * 2.0f;
#if UE_VERSION_AT_LEAST(5, 1, 0)
		View.bEnabled = InView.bIsGameView && !InView.bIsSceneCapture && !InView.bIsSceneCaptureCube && !InView.bIsReflectionCapture && !InView.bIsPlanarReflection;
#else
		View.bEnabled = InView.bIsGameView && !InView.bIsSceneCapture && !InView.bIsReflectionCapture && !InView.bIsPlanarReflection;
#endif
		View.TemporalJitterPixels = ((FViewInfo const&)InView).TemporalJitterPixels;
		if (View.bEnabled && (InView.GetFeatureLevel() >= ERHIFeatureLevel::SM6))
		{
			View.GameTimeMs = InView.Family->Time.GetDeltaWorldTimeSeconds();
			GameDeltaTime = InView.Family->Time.GetDeltaWorldTimeSeconds();
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
	UpscalerDesc.frameTimeDelta = View->Family->Time.GetDeltaWorldTimeSeconds() * 1000.f;
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
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(InputTextureExtents,
			PF_G16R16F,
			FClearValueBinding::Transparent,
			TexCreate_ShaderResource | TexCreate_UAV,
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
		interpolateParams->presentColor = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture, FFX_API_RESOURCE_STATE_COPY_DEST);

		Presenter->GetBackend()->SetFeatureLevel(&Context->Context, View->GetFeatureLevel());

		GraphBuilder.AddPass(RDG_EVENT_NAME("FidelityFX-FrameInterpolation"), PassParameters, ERDGPassFlags::Compute | ERDGPassFlags::NeverCull | ERDGPassFlags::Copy, [Presenter, PassParameters, Context, ConfigDesc](FRHICommandListImmediate& RHICmdList)
		{
			PassParameters->ColorTexture->MarkResourceAsUsed();
			PassParameters->InterpolatedRT->MarkResourceAsUsed();
			PassParameters->SceneDepth->MarkResourceAsUsed();
			PassParameters->MotionVectors->MarkResourceAsUsed();

			Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::InterpolateRT);
			RHICmdList.EnqueueLambda([Presenter, Context, ConfigDesc](FRHICommandListImmediate& cmd) mutable
			{
				Presenter->GetBackend()->UpdateSwapChain(&Context->Context, ConfigDesc);
				Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::InterpolateRHI);
			});
		});

		// Interpolate the frame
		{
			interpolateParams->commandList = (void*)&GraphBuilder;
			interpolateParams->outputs[0] = Presenter->GetBackend()->GetNativeResource(PassParameters->InterpolatedRT, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

			UpscalerDesc.commandList = interpolateParams->commandList;
			UpscalerDesc.depth = Presenter->GetBackend()->GetNativeResource(PassParameters->SceneDepth, FFX_API_RESOURCE_STATE_COMPUTE_READ);
			UpscalerDesc.motionVectors = Presenter->GetBackend()->GetNativeResource(PassParameters->MotionVectors, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

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
				AddCopyTexturePass(GraphBuilder, PassParameters->InterpolatedRT, PassParameters->Interpolated, Info);
				AddCopyTexturePass(GraphBuilder, PassParameters->InterpolatedRT, BackBufferRDG, Info);
			}
			else
			{
				check(Info.Size.X == BackBufferRDG->Desc.Extent.X && Info.Size.Y == BackBufferRDG->Desc.Extent.Y);
				check(Info.Size.X == PassParameters->InterpolatedRT->Desc.Extent.X && Info.Size.Y == PassParameters->InterpolatedRT->Desc.Extent.Y);
				AddCopyTexturePass(GraphBuilder, PassParameters->InterpolatedRT, BackBufferRDG, Info);
			}

			delete interpolateParams;
		}
	}
	else if (!bResized)
	{
		bInterpolated = true;
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

			bool const bWholeScreen = (PassParameters->Interpolated.GetTexture() == PassParameters->InterpolatedRT.GetTexture());
			ffxConfigureDescFrameGeneration ConfigureDesc = ConfigDesc;

			if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
			{
				ConfigureDesc.HUDLessColor = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture, FFX_API_RESOURCE_STATE_COPY_DEST);
				interpolateParams->presentColor = Presenter->GetBackend()->GetNativeResource(bWholeScreen ? PassParameters->BackBufferTexture.GetTexture() : PassParameters->HudTexture.GetTexture(), bWholeScreen ? FFX_API_RESOURCE_STATE_PRESENT : FFX_API_RESOURCE_STATE_COPY_DEST);
			}
			else
			{
				interpolateParams->presentColor = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture, FFX_API_RESOURCE_STATE_COPY_DEST);
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
			PrepareDesc.depth = Presenter->GetBackend()->GetNativeResource(PassParameters->SceneDepth, FFX_API_RESOURCE_STATE_COMPUTE_READ);
			PrepareDesc.motionVectors = Presenter->GetBackend()->GetNativeResource(PassParameters->MotionVectors, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

			auto InterpolatedRes = Presenter->GetBackend()->GetNativeResource(PassParameters->InterpolatedRT, Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative || !bWholeScreen || InterpolateIndex == 0 ? FFX_API_RESOURCE_STATE_UNORDERED_ACCESS : FFX_API_RESOURCE_STATE_COPY_SRC);
			Presenter->SetCustomPresentStatus(Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative ? FFXFrameInterpolationCustomPresentStatus::PresentRT : FFXFrameInterpolationCustomPresentStatus::InterpolateRT);
			RHICmdList.EnqueueLambda([PrepareDesc, ConfigureDesc, ViewportRHI, Presenter, Context, InterpolatedRes, interpolateParams, OutputExtents, OutputPoint, bWholeScreen, ViewportOutputFormat, GHDRMinLuminnanceLog10, GHDRMaxLuminnance](FRHICommandListImmediate& cmd) mutable
			{
				Presenter->GetBackend()->UpdateSwapChain(&Context->Context, ConfigureDesc);
				Presenter->SetCustomPresentStatus(Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative ? FFXFrameInterpolationCustomPresentStatus::PresentRHI : FFXFrameInterpolationCustomPresentStatus::InterpolateRHI);
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
					CmdBuffer = Presenter->GetBackend()->GetNativeCommandBuffer(cmd);
				}
				if (CmdBuffer)
				{
					// Prepare the interpolation context on the current RHI command list
					{
						ffxDispatchDescFrameGenerationPrepare UpscalerDesc = PrepareDesc;
						UpscalerDesc.commandList = Presenter->GetBackend()->GetNativeCommandBuffer(cmd);

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

			RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
#if UE_VERSION_OLDER_THAN(5, 1, 0)
			RHICmdList.SubmitCommandsHint();
#endif

			if (Presenter->GetMode() != EFFXFrameInterpolationPresentModeNative)
			{
#if UE_VERSION_AT_LEAST(5, 2, 0)
				FTexture2DRHIRef BackBuffer = RHIGetViewportBackBuffer(ViewportRHI);
#else
				FTexture2DRHIRef BackBuffer = RHICmdList.GetViewportBackBuffer(ViewportRHI);
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

	return bInterpolated;
}

void FFXFrameInterpolation::InterpolateFrame(FRDGBuilder& GraphBuilder)
{
	auto* Engine = GEngine;
	auto GameViewport = Engine ? Engine->GameViewport : nullptr;
	auto Viewport = GameViewport ? GameViewport->Viewport : nullptr;
	auto ViewportRHI = Viewport ? Viewport->GetViewportRHI() : nullptr;
	FIntPoint ViewportSizeXY = Viewport ? Viewport->GetSizeXY() : FIntPoint::ZeroValue;
	FFXFrameInterpolationCustomPresent* Presenter = ViewportRHI.IsValid() ? (FFXFrameInterpolationCustomPresent*)ViewportRHI->GetCustomPresent() : nullptr;
	bool bAllowed = CVarEnableFFXFI.GetValueOnAnyThread() != 0 && Presenter;
#if WITH_EDITORONLY_DATA
	bAllowed &= !GIsEditor;
#endif
	if (bAllowed && (Views.Num() > 0))
	{
		FTexture2DRHIRef BackBuffer = RHIGetViewportBackBuffer(ViewportRHI);
		FRDGTextureRef BackBufferRDG = RegisterExternalTexture(GraphBuilder, BackBuffer, nullptr);

		if (!IsValidRef(BackBufferRT) || BackBufferRT->GetDesc().Extent.X != BackBufferRDG->Desc.Extent.X || BackBufferRT->GetDesc().Extent.Y != BackBufferRDG->Desc.Extent.Y || BackBufferRT->GetDesc().Format != BackBufferRDG->Desc.Format)
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(BackBufferRDG->Desc.Extent.X, BackBufferRDG->Desc.Extent.Y),
				BackBuffer->GetFormat(),
				FClearValueBinding::Transparent,
				TexCreate_UAV,
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
				TexCreate_UAV,
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
				if ((Pair.Key->Family->GetTemporalUpscalerInterface() && Pair.Key->Family->GetTemporalUpscalerInterface()->GetDebugName() == FString(TEXT("FFXFSR3TemporalUpscaler"))) && Pair.Value.bEnabled && Pair.Value.ViewFamilyTexture && (ViewportSizeXY.X == Pair.Value.ViewFamilyTexture->Desc.Extent.X) && (ViewportSizeXY.Y == Pair.Value.ViewFamilyTexture->Desc.Extent.Y))
				{
					bool const bInterpolated = InterpolateView(GraphBuilder, Presenter, Pair.Key, Pair.Value, FinalBuffer, InterpolatedRDG, BackBufferRDG, InterpolateIndex);
					InterpolateIndex = bInterpolated ? InterpolateIndex +1 : InterpolateIndex;
					bAllowed |= bInterpolated;
				}
			}
		}

		if ((Presenter->GetBackend()->GetAPI() != EFFXBackendAPI::Unreal) && (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative))
		{
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

		Presenter->EndFrame();
	}

	Views.Empty();

	if (!bAllowed && ViewportRHI.IsValid())
	{
		if (Presenter)
		{
			Presenter->SetEnabled(false);
			if (Presenter->GetContext())
			{
				ffxConfigureDescFrameGeneration ConfigDesc;
				FMemory::Memzero(ConfigDesc);
				ConfigDesc.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
				ConfigDesc.swapChain = Presenter->GetBackend()->GetSwapchain(ViewportRHI->GetNativeSwapChain());
				ConfigDesc.frameGenerationEnabled = false;
				ConfigDesc.allowAsyncWorkloads = false;

				Presenter->GetBackend()->UpdateSwapChain(Presenter->GetContext(), ConfigDesc);
			}
		}
	}

	bInterpolatedFrame = bAllowed;
	if (Presenter && Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
	{
		// If the present count fell behind reset it - otherwise it will persist indefinitely
		PresentCount = (PresentCount >= InterpolationCount) ? PresentCount : (InterpolationCount + (bAllowed ? 1llu : 0llu));
		InterpolationCount += bAllowed ? 1llu : 0llu;
		ResetState = bAllowed ? 1u : 0u;
	}
	else
	{
		ResetState = bAllowed ? 2u : 0u;
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
						FTexture2DRHIRef BackBuffer = RHIGetViewportBackBuffer(Viewport);
#else
						FTexture2DRHIRef BackBuffer = RHICmdList.GetViewportBackBuffer(Viewport);
#endif
						FFXFrameInterpolationCustomPresent* Presenter = (FFXFrameInterpolationCustomPresent*)Viewport->GetCustomPresent();
						if (Presenter && BackBufferRT.IsValid())
						{
							this->CalculateFPSTimings();
							FTextureRHIRef InterpolatedFrame = BackBufferRT->GetRHI();
							RHICmdList.PushEvent(TEXT("FFXFrameInterpolation::OnSlateWindowRendered"), FColor::White);
							check(FIntPoint(InterpolatedFrame->GetSizeXYZ().X, InterpolatedFrame->GetSizeXYZ().Y) == FIntPoint(BackBuffer->GetSizeXYZ().X, BackBuffer->GetSizeXYZ().Y));
							TransitionAndCopyTexture(RHICmdList, InterpolatedFrame, BackBuffer, {});
							RHICmdList.PopEvent();

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

void FFXFrameInterpolation::OnBackBufferReadyToPresentCallback(class SWindow& SlateWindow, const FTexture2DRHIRef& BackBuffer)
{
    /** Callback for when a backbuffer is ready for reading (called on render thread) */
	FRHIViewport** ViewportPtr = Windows.Find(&SlateWindow);
    if (ViewportPtr && CVarEnableFFXFI.GetValueOnAnyThread())
    {
        FViewportRHIRef Viewport = *ViewportPtr;
        FFXFrameInterpolationCustomPresent* Presenter = (FFXFrameInterpolationCustomPresent*)Viewport->GetCustomPresent();

		if (Presenter)
		{
			PresentCount += (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative) ? 1llu : 0llu;
			if (ResetState)
			{
				Presenter->CopyBackBufferRT(BackBuffer);
			}
		}
    }

	ResetState = ((ResetState > 0) && !bInterpolatedFrame) ? ResetState - 1 : ResetState;
	bInterpolatedFrame = false;

	if (ResetState == 0)
	{
		if (ViewportPtr && CVarEnableFFXFI.GetValueOnAnyThread())
		{
			FViewportRHIRef Viewport = *ViewportPtr;
			FFXFrameInterpolationCustomPresent* Presenter = (FFXFrameInterpolationCustomPresent*)Viewport->GetCustomPresent();
			if (Presenter)
			{
				Presenter->SetEnabled(false);
				if (Presenter->GetContext())
				{
					ffxConfigureDescFrameGeneration ConfigDesc;
					FMemory::Memzero(ConfigDesc);
					ConfigDesc.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
					ConfigDesc.swapChain = Presenter->GetBackend()->GetSwapchain(Viewport->GetNativeSwapChain());
					ConfigDesc.frameGenerationEnabled = false;
					ConfigDesc.allowAsyncWorkloads = false;

					Presenter->GetBackend()->UpdateSwapChain(Presenter->GetContext(), ConfigDesc);
				}
			}
		}
	}
}