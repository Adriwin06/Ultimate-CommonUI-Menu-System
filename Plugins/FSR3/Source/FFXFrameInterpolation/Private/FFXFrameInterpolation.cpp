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

#include "FFXFrameInterpolation.h"
#include "FFXFrameInterpolationViewExtension.h"
#include "FFXSharedBackend.h"
#include "FFXFrameInterpolationSlate.h"
#include "FFXFrameInterpolationCustomPresent.h"
#include "FFXFSR3History.h"
#include "FFXFSR3Settings.h"

#include "PostProcess/PostProcessing.h"
#include "PostProcess/TemporalAA.h"

#include "FFXFrameInterpolationApi.h"
#include "FFXOpticalFlowApi.h"
#include "FFXFSR3.h"

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
		RDG_TEXTURE_ACCESS(HudTexture, ERHIAccess::CopyDest)
		RDG_TEXTURE_ACCESS(InterpolatedRT, ERHIAccess::CopyDest)
		RDG_TEXTURE_ACCESS(Interpolated, ERHIAccess::CopyDest)
	END_SHADER_PARAMETER_STRUCT()
};

//------------------------------------------------------------------------------------------------------
// Implementation for the Frame Interpolation.
//------------------------------------------------------------------------------------------------------
FFXFrameInterpolation::FFXFrameInterpolation()
: GameDeltaTime(0.0)
, LastTime(FPlatformTime::Seconds())
, AverageTime(0.f)
, AverageFPS(0.f)
, Index(0u)
, bInterpolatedFrame(false)
, bNeedsReset(true)
{
	UGameViewportClient::OnViewportCreated().AddRaw(this, &FFXFrameInterpolation::OnViewportCreatedHandler_SetCustomPresent);
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FFXFrameInterpolation::OnPostEngineInit);
}

FFXFrameInterpolation::~FFXFrameInterpolation()
{
	ViewExtension = nullptr;
}

IFFXFrameInterpolationCustomPresent* FFXFrameInterpolation::CreateCustomPresent(IFFXSharedBackend* Backend, uint32_t Flags, FIntPoint RenderSize, FIntPoint DisplaySize, FfxSwapchain RawSwapChain, FfxCommandQueue Queue, FfxSurfaceFormat Format, FfxPresentCallbackFunc CompositionFunc)
{
	FFXFrameInterpolationCustomPresent* Result = new FFXFrameInterpolationCustomPresent;
	if (Result)
	{
		if (Result->InitSwapChain(Backend, Flags, RenderSize, DisplaySize, RawSwapChain, Queue, Format, CompositionFunc))
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
		if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
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
		static const auto CVarFSR3UseRHIBackend = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.UseRHI"));
		auto ViewportRHI = GEngine->GameViewport->Viewport->GetViewportRHI();
		void* NativeSwapChain = ViewportRHI->GetNativeSwapChain();
		FFXFrameInterpolationCustomPresent** PresentHandler = SwapChains.Find(NativeSwapChain);
		if (PresentHandler)
		{
			(*PresentHandler)->InitViewport(GEngine->GameViewport->Viewport, GEngine->GameViewport->Viewport->GetViewportRHI());
		}
		else if (CVarFSR3UseRHIBackend && CVarFSR3UseRHIBackend->GetValueOnAnyThread() != 0)
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
	static const auto CVarFSR3Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.Enabled"));
	auto* Engine = GEngine;
	auto GameViewport = Engine ? Engine->GameViewport : nullptr;
	auto Viewport = GameViewport ? GameViewport->Viewport : nullptr;
	auto ViewportRHI = Viewport ? Viewport->GetViewportRHI() : nullptr;
	FFXFrameInterpolationCustomPresent* Presenter = ViewportRHI.IsValid() ? (FFXFrameInterpolationCustomPresent*)ViewportRHI->GetCustomPresent() : nullptr;
	if (CVarEnableFFXFI.GetValueOnAnyThread() != 0 && (CVarFSR3Enabled && CVarFSR3Enabled->GetValueOnAnyThread() != 0) && Presenter && Presenter->GetMode() == EFFXFrameInterpolationPresentModeRHI)
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
		View.ViewRect = ((FViewInfo const&)InView).ViewRect;
		View.InputExtentsQuantized = View.ViewRect.Size();
		QuantizeSceneBufferSize(((FViewInfo const&)InView).GetSecondaryViewRectSize(), View.OutputExtents);
		View.OutputExtents = FIntPoint(FMath::Max(View.InputExtentsQuantized.X, View.OutputExtents.X), FMath::Max(View.InputExtentsQuantized.Y, View.OutputExtents.Y));
		View.bReset = InView.bCameraCut;
		View.CameraNear = InView.ViewMatrices.ComputeNearPlane();
		View.CameraFOV = InView.ViewMatrices.ComputeHalfFieldOfViewPerAxis().Y * 2.0f;
		View.bEnabled = InView.bIsGameView && !InView.bIsSceneCapture && !InView.bIsSceneCaptureCube && !InView.bIsReflectionCapture && !InView.bIsPlanarReflection;
		if (View.bEnabled)
		{
			GameDeltaTime = InView.Family->Time.GetDeltaWorldTimeSeconds();
			Views.Add(&InView, View);
		}
	}
}

static FfxCommandList GCommandList = nullptr;

static FfxBackbufferTransferFunction GetFfxTransferFunction(EDisplayOutputFormat UEFormat)
{
	FfxBackbufferTransferFunction Output = FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB;
	switch (UEFormat)
	{
		// Gamma ST.2084
	case EDisplayOutputFormat::HDR_ACES_1000nit_ST2084:
	case EDisplayOutputFormat::HDR_ACES_2000nit_ST2084:
		Output = FFX_BACKBUFFER_TRANSFER_FUNCTION_PQ;
		break;

		// Gamma 1.0 (Linear)
	case EDisplayOutputFormat::HDR_ACES_1000nit_ScRGB:
	case EDisplayOutputFormat::HDR_ACES_2000nit_ScRGB:
		// Linear. Still supports expanded color space with values >1.0f and <0.0f.
		// The actual range is determined by the pixel format (e.g. a UNORM format can only ever have 0-1).
		Output = FFX_BACKBUFFER_TRANSFER_FUNCTION_SCRGB;
		break;
	
		// Gamma 2.2
	case EDisplayOutputFormat::SDR_sRGB:
	case EDisplayOutputFormat::SDR_Rec709:
		Output = FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB;
		break;

		// Unsupported types that require modifications to the FidelityFX code in order to support
	case EDisplayOutputFormat::SDR_ExplicitGammaMapping:
	case EDisplayOutputFormat::HDR_LinearEXR:
	case EDisplayOutputFormat::HDR_LinearNoToneCurve:
	case EDisplayOutputFormat::HDR_LinearWithToneCurve:
	default:
		check(false);
		Output = FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB;
		break;
	}

	return Output;
}

bool FFXFrameInterpolation::InterpolateView(FRDGBuilder& GraphBuilder, FFXFrameInterpolationCustomPresent* Presenter, const FSceneView* View, FFXFrameInterpolationView const& ViewDesc, FRDGTextureRef FinalBuffer, FRDGTextureRef InterpolatedRDG, FRDGTextureRef BackBufferRDG)
{
	bool bInterpolated = false;
	auto* Engine = GEngine;
	auto GameViewport = Engine ? Engine->GameViewport : nullptr;
	auto Viewport = GameViewport ? GameViewport->Viewport : nullptr;
	auto ViewportRHI = Viewport ? Viewport->GetViewportRHI() : nullptr;
	FIntPoint ViewportSizeXY = Viewport ? Viewport->GetSizeXY() : FIntPoint::ZeroValue;

#if UE_VERSION_AT_LEAST(5, 3, 0)
	TRefCountPtr<IFFXFSR3CustomTemporalAAHistory> CustomTemporalAAHistory = (((FSceneViewState*)View->State)->PrevFrameViewInfo.ThirdPartyTemporalUpscalerHistory);
#else
	TRefCountPtr<IFFXFSR3CustomTemporalAAHistory> CustomTemporalAAHistory = (((FSceneViewState*)View->State)->PrevFrameViewInfo.CustomTemporalAAHistory);
#endif
	TRefCountPtr<IFFXFSR3History> FSRContext = (IFFXFSR3History*)(CustomTemporalAAHistory.GetReference());

	FRDGTextureRef ViewFamilyTexture = ViewDesc.ViewFamilyTexture;
	FIntRect ViewRect = ViewDesc.ViewRect;
	FIntPoint InputExtents = ViewDesc.ViewRect.Size();
	FIntPoint InputExtentsQuantized = ViewDesc.InputExtentsQuantized;
	FIntPoint OutputExtents = ViewDesc.OutputExtents;
	FIntPoint OutputPoint = FIntPoint(
		FMath::CeilToInt(((FViewInfo*)View)->UnscaledViewRect.Min.X * View->Family->SecondaryViewFraction),
		FMath::CeilToInt(((FViewInfo*)View)->UnscaledViewRect.Min.Y * View->Family->SecondaryViewFraction));
	float CameraNear = ViewDesc.CameraNear;
	float CameraFOV = ViewDesc.CameraFOV;
	bool bEnabled = ViewDesc.bEnabled;
	bool bReset = ViewDesc.bReset || bNeedsReset;
	bool const bResized = Presenter->Resized();
	FRHICopyTextureInfo Info;

	FfxFsr3UpscalerContextDescription UpscalerDesc = *FSRContext->GetFSRContextDesc();
	FfxFsr3UpscalerSharedResources SharedResources = *FSRContext->GetFSRResources();

	FRDGTextureRef ColorBuffer = FinalBuffer;
	FRDGTextureRef InterBuffer = InterpolatedRDG;
	FRDGTextureRef HudBuffer = nullptr;
	FFXFIResourceRef Context = Presenter->UpdateContexts(GraphBuilder, ((FSceneViewState*)View->State)->UniqueID, UpscalerDesc, ViewportSizeXY, GetFFXFormat(BackBufferRDG->Desc.Format, false));

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

	float DeltaTimeMs = GameDeltaTime * 1000.f;
	static const auto CVarHDRMinLuminanceLog10 = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.HDR.Display.MinLuminanceLog10"));
	static const auto CVarHDRMaxLuminance = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.HDR.Display.MaxLuminance"));

	float GHDRMinLuminnanceLog10 = CVarHDRMinLuminanceLog10 ? CVarHDRMinLuminanceLog10->GetValueOnAnyThread() : 0.f;
	int32 GHDRMaxLuminnance = CVarHDRMinLuminanceLog10 ? CVarHDRMaxLuminance->GetValueOnAnyThread() : 1;
	EDisplayOutputFormat ViewportOutputFormat = Viewport->GetDisplayOutputFormat();

	bool bAllowAsyncWorkloads = (CVarFSR3AllowAsyncWorkloads.GetValueOnAnyThread() != 0);
	bool bShowDebugMode = false;
#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
	bShowDebugMode = CVarFFXFIShowDebugView.GetValueOnAnyThread() != 0;
#endif

	// compute how many VSync intervals interpolated and real frame should be displayed
	FfxFrameInterpolationDispatchDescription* interpolateParams = new FfxFrameInterpolationDispatchDescription;
	{
		interpolateParams->flags = 0;
#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
		interpolateParams->flags |= CVarFFXFIShowDebugTearLines.GetValueOnAnyThread() ? FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_TEAR_LINES : 0;
		interpolateParams->flags |= CVarFFXFIShowDebugView.GetValueOnAnyThread() ? FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_VIEW : 0;
#endif
		interpolateParams->renderSize.width = InputExtents.X;
		interpolateParams->renderSize.height = InputExtents.Y;
		interpolateParams->displaySize.width = ColorBuffer->Desc.Extent.X;
		interpolateParams->displaySize.height = ColorBuffer->Desc.Extent.Y;
		interpolateParams->interpolationRect.left = 0;
		interpolateParams->interpolationRect.top = 0;
		interpolateParams->interpolationRect.width = interpolateParams->displaySize.width;
		interpolateParams->interpolationRect.height = interpolateParams->displaySize.height;
		interpolateParams->frameTimeDelta = DeltaTimeMs;
		interpolateParams->reset = bReset;
		interpolateParams->viewSpaceToMetersFactor = 1.f / View->WorldToMetersScale;

		interpolateParams->opticalFlowBufferSize.width = interpolateParams->displaySize.width / s_opticalFlowBlockSize;
		interpolateParams->opticalFlowBufferSize.height = interpolateParams->displaySize.height / s_opticalFlowBlockSize;
		interpolateParams->opticalFlowScale = { 1.f / interpolateParams->displaySize.width, 1.f / interpolateParams->displaySize.height };
		interpolateParams->opticalFlowBlockSize = s_opticalFlowBlockSize;

		if (bool(ERHIZBuffer::IsInverted))
		{
			interpolateParams->cameraNear = FLT_MAX;
			interpolateParams->cameraFar = CameraNear;
		}
		else
		{
			interpolateParams->cameraNear = CameraNear;
			interpolateParams->cameraFar = FLT_MAX;
		}
		interpolateParams->cameraFovAngleVertical = CameraFOV;
		interpolateParams->dilatedDepth = SharedResources.dilatedDepth.Resource;
		interpolateParams->dilatedMotionVectors = SharedResources.dilatedMotionVectors.Resource;
		interpolateParams->reconstructPrevNearDepth = SharedResources.reconstructedPrevNearestDepth.Resource;
	}

	if (Presenter->GetBackend()->GetAPI() == EFFXBackendAPI::Unreal)
	{
		bInterpolated = true;
		Presenter->GetBackend()->UpdateSwapChain(Presenter->GetInterface(), ViewportRHI->GetNativeSwapChain(), true, bAllowAsyncWorkloads, bShowDebugMode);
		interpolateParams->currentBackBuffer = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture, FFX_RESOURCE_STATE_COPY_DEST);
		FMemory::Memzero(interpolateParams->currentBackBuffer_HUDLess);

		Presenter->GetBackend()->SetFeatureLevel(Presenter->GetInterface(), View->GetFeatureLevel());

		GraphBuilder.AddPass(RDG_EVENT_NAME("FidelityFX-FrameInterpolation"), PassParameters, ERDGPassFlags::Compute | ERDGPassFlags::NeverCull | ERDGPassFlags::Copy, [Presenter, PassParameters, interpolateParams, DeltaTimeMs](FRHICommandListImmediate& RHICmdList)
		{
			PassParameters->ColorTexture->MarkResourceAsUsed();
			PassParameters->InterpolatedRT->MarkResourceAsUsed();

			Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::InterpolateRT);
			RHICmdList.EnqueueLambda([Presenter](FRHICommandListImmediate& cmd) mutable
			{
				Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::InterpolateRHI);
			});
		});

		{
			FfxOpticalflowDispatchDescription ofDispatchDesc{};
			ofDispatchDesc.commandList = Presenter->GetBackend()->GetCommandList(&GraphBuilder);
			ofDispatchDesc.color = interpolateParams->currentBackBuffer;
			ofDispatchDesc.reset = interpolateParams->reset;
			ofDispatchDesc.opticalFlowVector = Context->OpticalFlowResources.opticalFlow.Resource;
			ofDispatchDesc.opticalFlowSCD = Context->OpticalFlowResources.opticalFlowSCD.Resource;
    		ofDispatchDesc.backbufferTransferFunction = GetFfxTransferFunction(ViewportOutputFormat);
    		ofDispatchDesc.minMaxLuminance.x = ofDispatchDesc.backbufferTransferFunction != FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB ? FMath::Pow(10, GHDRMinLuminnanceLog10) : 0.f;
    		ofDispatchDesc.minMaxLuminance.y = ofDispatchDesc.backbufferTransferFunction != FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB ? GHDRMaxLuminnance : 1.0f;

			FfxErrorCode Code = ffxOpticalflowContextDispatch(&Context->OpticalFlowContext, &ofDispatchDesc);
			check(Code == FFX_OK);
		}

		// Interpolate the frame
		{
			interpolateParams->commandList = Presenter->GetBackend()->GetCommandList(&GraphBuilder);

			FfxResource InterpolatedRes = Presenter->GetBackend()->GetNativeResource(PassParameters->InterpolatedRT, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
			interpolateParams->output = InterpolatedRes;

			interpolateParams->opticalFlowVector = Context->OpticalFlowResources.opticalFlow.Resource;
			interpolateParams->opticalFlowSceneChangeDetection = Context->OpticalFlowResources.opticalFlowSCD.Resource;
    		interpolateParams->backBufferTransferFunction = GetFfxTransferFunction(ViewportOutputFormat);
    		interpolateParams->minMaxLuminance[0] = interpolateParams->backBufferTransferFunction != FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB ? FMath::Pow(10, GHDRMinLuminnanceLog10) : 0.f;
    		interpolateParams->minMaxLuminance[1] = interpolateParams->backBufferTransferFunction != FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB ? GHDRMaxLuminnance : 1.f;

			FfxErrorCode Code = ffxFrameInterpolationDispatch(&Context->Context, interpolateParams);
			check(Code == FFX_OK);

			Info.Size.X = interpolateParams->displaySize.width;
			Info.Size.Y = interpolateParams->displaySize.height;
			if (PassParameters->Interpolated != PassParameters->InterpolatedRT)
			{
				Info.DestPosition.X = OutputPoint.X;
				Info.DestPosition.Y = OutputPoint.Y;
				Info.Size.X = FMath::Min((uint32)interpolateParams->displaySize.width, (uint32)PassParameters->Interpolated->Desc.Extent.X);
				Info.Size.Y = FMath::Min((uint32)interpolateParams->displaySize.height, (uint32)PassParameters->Interpolated->Desc.Extent.Y);
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
		GraphBuilder.AddPass(RDG_EVENT_NAME("FidelityFX-FrameInterpolation"), PassParameters, ERDGPassFlags::Compute | ERDGPassFlags::NeverCull | ERDGPassFlags::Copy, [FSRContext, OutputExtents, OutputPoint, ViewportRHI, Presenter, Context, PassParameters, interpolateParams, DeltaTimeMs, Engine, ViewportOutputFormat, GHDRMinLuminnanceLog10, GHDRMaxLuminnance, bAllowAsyncWorkloads, bShowDebugMode](FRHICommandListImmediate& RHICmdList)
		{
			PassParameters->ColorTexture->MarkResourceAsUsed();
			PassParameters->InterpolatedRT->MarkResourceAsUsed();
			if (PassParameters->HudTexture)
			{
				PassParameters->HudTexture->MarkResourceAsUsed();
			}

			bool const bWholeScreen = (PassParameters->Interpolated.GetTexture() == PassParameters->InterpolatedRT.GetTexture());

			if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
			{
				interpolateParams->currentBackBuffer_HUDLess = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture, FFX_RESOURCE_STATE_COPY_DEST);
				interpolateParams->currentBackBuffer = Presenter->GetBackend()->GetNativeResource(bWholeScreen ? PassParameters->BackBufferTexture.GetTexture() : PassParameters->HudTexture.GetTexture(), bWholeScreen ? FFX_RESOURCE_STATE_PRESENT : FFX_RESOURCE_STATE_COPY_DEST);
			}
			else
			{
				FMemory::Memzero(interpolateParams->currentBackBuffer_HUDLess);
				interpolateParams->currentBackBuffer = Presenter->GetBackend()->GetNativeResource(PassParameters->ColorTexture, FFX_RESOURCE_STATE_COPY_DEST);
			}

			FfxResource InterpolatedRes = Presenter->GetBackend()->GetNativeResource(PassParameters->InterpolatedRT, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
			Presenter->SetCustomPresentStatus(Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative ? FFXFrameInterpolationCustomPresentStatus::PresentRT : FFXFrameInterpolationCustomPresentStatus::InterpolateRT);
			RHICmdList.EnqueueLambda([ViewportRHI, Presenter, FSRContext, Context, InterpolatedRes, interpolateParams, OutputExtents, OutputPoint, bWholeScreen, ViewportOutputFormat, GHDRMinLuminnanceLog10, GHDRMaxLuminnance, bAllowAsyncWorkloads, bShowDebugMode](FRHICommandListImmediate& cmd) mutable
			{
				Presenter->GetBackend()->UpdateSwapChain(Presenter->GetInterface(), ViewportRHI->GetNativeSwapChain(), true, bAllowAsyncWorkloads, bShowDebugMode);
				Presenter->SetCustomPresentStatus(Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative ? FFXFrameInterpolationCustomPresentStatus::PresentRHI : FFXFrameInterpolationCustomPresentStatus::InterpolateRHI);
				if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
				{
					Presenter->GetBackend()->RegisterFrameResources(Context.GetReference(), FSRContext.GetReference());
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
					{
						FfxOpticalflowDispatchDescription ofDispatchDesc{};
						ofDispatchDesc.commandList = CmdBuffer;
						ofDispatchDesc.color = interpolateParams->currentBackBuffer_HUDLess.resource ? interpolateParams->currentBackBuffer_HUDLess : interpolateParams->currentBackBuffer;
						ofDispatchDesc.reset = interpolateParams->reset;
						ofDispatchDesc.opticalFlowVector = Context->OpticalFlowResources.opticalFlow.Resource;
						ofDispatchDesc.opticalFlowSCD = Context->OpticalFlowResources.opticalFlowSCD.Resource;
    					ofDispatchDesc.backbufferTransferFunction = GetFfxTransferFunction(ViewportOutputFormat);
						ofDispatchDesc.minMaxLuminance.x = ofDispatchDesc.backbufferTransferFunction != FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB ? FMath::Pow(10, GHDRMinLuminnanceLog10) : 0.f;
						ofDispatchDesc.minMaxLuminance.y = ofDispatchDesc.backbufferTransferFunction != FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB ? GHDRMaxLuminnance : 1.f;

						FfxErrorCode Code = ffxOpticalflowContextDispatch(&Context->OpticalFlowContext, &ofDispatchDesc);
						check(Code == FFX_OK);
					}

					// Interpolate the frame
					{
						FfxResource OutputRes = Presenter->GetBackend()->GetInterpolationOutput(Presenter->GetBackend()->GetSwapchain(ViewportRHI->GetNativeSwapChain()));
						interpolateParams->output = (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative && bWholeScreen) ? OutputRes : InterpolatedRes;
						interpolateParams->commandList = CmdBuffer;

						interpolateParams->opticalFlowVector = Context->OpticalFlowResources.opticalFlow.Resource;
						interpolateParams->opticalFlowSceneChangeDetection = Context->OpticalFlowResources.opticalFlowSCD.Resource;
						interpolateParams->backBufferTransferFunction = GetFfxTransferFunction(ViewportOutputFormat);
						interpolateParams->minMaxLuminance[0] = interpolateParams->backBufferTransferFunction != FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB ? FMath::Pow(10, GHDRMinLuminnanceLog10) : 0.f;
						interpolateParams->minMaxLuminance[1] = interpolateParams->backBufferTransferFunction != FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB ? GHDRMaxLuminnance : 1.f;

						FfxErrorCode Code = ffxFrameInterpolationDispatch(&Context->Context, interpolateParams);
						check(Code == FFX_OK);

                        if (!bWholeScreen && (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative))
                        {
							Presenter->GetBackend()->CopySubRect(CmdBuffer, InterpolatedRes, OutputRes, OutputExtents, OutputPoint);
                        }
					}
				}
				delete interpolateParams;
			});

			RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);

			if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeNative)
			{
				FSRContext->AdvanceIndex();
			}
			else
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
					FTexture2DRHIRef InterpolatedFrame = PassParameters->InterpolatedRT->GetRHI();
					TransitionAndCopyTexture(RHICmdList, InterpolatedFrame, PassParameters->Interpolated->GetRHI(), CopyInfo);
					if (Presenter->GetMode() == EFFXFrameInterpolationPresentModeRHI)
					{
						check(PassParameters->Interpolated->Desc.Extent == BackBuffer->GetDesc().Extent);
						TransitionAndCopyTexture(RHICmdList, InterpolatedFrame, BackBuffer, CopyInfo);
					}
				}
				else
				{
					FTexture2DRHIRef InterpolatedFrame = PassParameters->InterpolatedRT->GetRHI();
					check(InterpolatedFrame->GetDesc().Extent == BackBuffer->GetDesc().Extent);
					TransitionAndCopyTexture(RHICmdList, InterpolatedFrame, BackBuffer, {});
				}
			}
		});
	}

	return bInterpolated;
}

void FFXFrameInterpolation::InterpolateFrame(FRDGBuilder& GraphBuilder)
{
	static const auto CVarFSR3Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.Enabled"));
	auto* Engine = GEngine;
	auto GameViewport = Engine ? Engine->GameViewport : nullptr;
	auto Viewport = GameViewport ? GameViewport->Viewport : nullptr;
	auto ViewportRHI = Viewport ? Viewport->GetViewportRHI() : nullptr;
	FIntPoint ViewportSizeXY = Viewport ? Viewport->GetSizeXY() : FIntPoint::ZeroValue;
	FFXFrameInterpolationCustomPresent* Presenter = ViewportRHI.IsValid() ? (FFXFrameInterpolationCustomPresent*)ViewportRHI->GetCustomPresent() : nullptr;
	bool bAllowed = CVarEnableFFXFI.GetValueOnAnyThread() != 0 && Presenter && (CVarFSR3Enabled && CVarFSR3Enabled->GetValueOnAnyThread() != 0);
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
				BackBuffer->GetDesc().Format,
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
				BackBuffer->GetDesc().Format,
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
		}

		FRDGTextureUAVDesc Interpolatedesc(InterpolatedRDG);
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Interpolatedesc), FVector::ZeroVector);

		bAllowed = false;
		for (auto Pair : Views)
		{
			if (Pair.Key->State)
			{
#if UE_VERSION_AT_LEAST(5, 3, 0)
				TRefCountPtr<IFFXFSR3CustomTemporalAAHistory> CustomTemporalAAHistory = (((FSceneViewState*)Pair.Key->State)->PrevFrameViewInfo.ThirdPartyTemporalUpscalerHistory);
#else
				TRefCountPtr<IFFXFSR3CustomTemporalAAHistory> CustomTemporalAAHistory = (((FSceneViewState*)Pair.Key->State)->PrevFrameViewInfo.CustomTemporalAAHistory);
#endif
				TRefCountPtr<IFFXFSR3History> FSRContext = (IFFXFSR3History*)(CustomTemporalAAHistory.GetReference());
				if ((Pair.Key->Family->GetTemporalUpscalerInterface() && Pair.Key->Family->GetTemporalUpscalerInterface()->GetDebugName() == FString(TEXT("FFXFSR3TemporalUpscaler"))) && Pair.Value.bEnabled && Pair.Value.ViewFamilyTexture && FSRContext.IsValid() && (ViewportSizeXY.X == Pair.Value.ViewFamilyTexture->Desc.Extent.X) && (ViewportSizeXY.Y == Pair.Value.ViewFamilyTexture->Desc.Extent.Y))
				{
					bAllowed |= InterpolateView(GraphBuilder, Presenter, Pair.Key, Pair.Value, FinalBuffer, InterpolatedRDG, BackBufferRDG);
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
				Presenter->GetBackend()->UpdateSwapChain(Presenter->GetInterface(), ViewportRHI->GetNativeSwapChain(), false, false, false);
			}
		}
	}

	bInterpolatedFrame = bAllowed;
}

void FFXFrameInterpolation::OnSlateWindowRendered(SWindow& SlateWindow, void* ViewportRHIPtr)
{
	static const auto CVarFSR3Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.Enabled"));
    static bool bProcessing = false;

	FViewportRHIRef Viewport = *((FViewportRHIRef*)ViewportRHIPtr);
	FFXFrameInterpolationCustomPresent* PresentHandler = (FFXFrameInterpolationCustomPresent*)Viewport->GetCustomPresent();

	if (IsInGameThread() && PresentHandler && PresentHandler->Enabled() && CVarEnableFFXFI.GetValueOnAnyThread() && (CVarFSR3Enabled && CVarFSR3Enabled->GetValueOnAnyThread() != 0))
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
#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
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
							FTexture2DRHIRef InterpolatedFrame = BackBufferRT->GetRHI();
							RHICmdList.PushEvent(TEXT("FFXFrameInterpolation::OnSlateWindowRendered"), FColor::White);
							check(InterpolatedFrame->GetDesc().Extent == BackBuffer->GetDesc().Extent);
							TransitionAndCopyTexture(RHICmdList, InterpolatedFrame, BackBuffer, {});
							RHICmdList.PopEvent();

							Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::PresentRT);
							RHICmdList.EnqueueLambda([this, Presenter](FRHICommandListImmediate& cmd) mutable
							{
								Presenter->SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus::PresentRHI);
							});
						}
				});

				App.ForceRedrawWindow(WindowPtr.ToSharedRef());
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
	static const auto CVarFSR3Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.Enabled"));
    /** Callback for when a backbuffer is ready for reading (called on render thread) */
	FRHIViewport** ViewportPtr = Windows.Find(&SlateWindow);
    if (ViewportPtr && CVarEnableFFXFI.GetValueOnAnyThread() && (CVarFSR3Enabled && CVarFSR3Enabled->GetValueOnAnyThread() != 0))
    {
        FViewportRHIRef Viewport = *ViewportPtr;
        FFXFrameInterpolationCustomPresent* Presenter = (FFXFrameInterpolationCustomPresent*)Viewport->GetCustomPresent();

		if (bInterpolatedFrame)
		{
			if (Presenter)
			{
				Presenter->CopyBackBufferRT(BackBuffer);
			}
		}
		else
		{
			if (Presenter)
			{
				Presenter->SetEnabled(false);
				if (Presenter->GetContext())
				{
					Presenter->GetBackend()->UpdateSwapChain(Presenter->GetInterface(), Viewport->GetNativeSwapChain(), false, false, false);
				}
			}
		}
    }
	bNeedsReset = !bInterpolatedFrame;
	bInterpolatedFrame = false;
}