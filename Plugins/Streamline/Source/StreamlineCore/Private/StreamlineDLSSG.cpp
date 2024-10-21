/*
* Copyright (c) 2022 - 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/

#include "StreamlineDLSSG.h"
#include "StreamlineCore.h"
#include "StreamlineCorePrivate.h"
#include "StreamlineAPI.h"
#include "StreamlineRHI.h"
#include "StreamlineViewExtension.h"
#include "sl_helpers.h"
#include "sl_dlss_g.h"
#include "UIHintExtractionPass.h"

#include "CoreMinimal.h"
#include "Framework/Application/SlateApplication.h"
#include "RenderGraphBuilder.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ScenePrivate.h"
#include "SystemTextures.h"
#include "HAL/PlatformApplicationMisc.h"

static FDelegateHandle OnPreRHIViewportCreateHandle;
static FDelegateHandle OnPostRHIViewportCreateHandle;
static FDelegateHandle OnSlateWindowDestroyedHandle;

static FDelegateHandle OnPreResizeWindowBackBufferHandle;
static FDelegateHandle OnPostResizeWindowBackBufferHandle;
static FDelegateHandle OnBackBufferReadyToPresentHandle;
static TAutoConsoleVariable<int32> CVarStreamlineDLSSGEnable(
	TEXT("r.Streamline.DLSSG.Enable"),
	0,
	TEXT("DLSS-FG mode (default = 0)\n")
	TEXT("0: off\n")
	TEXT("1: always on\n")
	TEXT("2: auto mode (on only when it helps)\n"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarStreamlineDLSSGAdjustMotionBlurTimeScale(
	TEXT("r.Streamline.DLSSG.AdjustMotionBlurTimeScale"), 2,
	TEXT("When DLSS-G is active, adjust the motion blur timescale based on the generated frames\n")
	TEXT("0: disabled\n")
	TEXT("1: enabled, not supporting auto mode\n")
	TEXT("2: enabled, supporting auto mode by using last frame's actually presented frames (default)\n"),
	ECVF_Default);


static TAutoConsoleVariable<bool> CVarStreamlineTagUIColorAlpha(
	TEXT("r.Streamline.TagUIColorAlpha"),
	true,
	TEXT("Pass UI color and alpha into Streamline (default = true)\n"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<bool> CVarStreamlineTagBackbuffer(
	TEXT("r.Streamline.TagBackbuffer"),
	true,
	TEXT("Pass backbuffer extent into Streamline (default = true)\n"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarStreamlineTagUIColorAlphaThreshold(
	TEXT("r.Streamline.TagUIColorAlphaThreshold"),
	0.0,
	TEXT("UI extraction pass alpha threshold value(default = 0.0) \n"),
	ECVF_RenderThreadSafe);


static TAutoConsoleVariable<bool> CVarStreamlineEditorTagUIColorAlpha(
	TEXT("r.Streamline.Editor.TagUIColorAlpha"),
	false,
	TEXT("Experimental: Pass UI color and alpha into Streamline in Editor PIE windows (default = false)\n"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<bool> CVarStreamlineDLSSGCheckStatusPerFrame(
	TEXT("r.Streamline.DLSSG.CheckStatusPerFrame"),
	true,
	TEXT("Check the DLSSG status at runtime and assert if it's failing somehow (default = true)\n"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<bool> CVarStreamlineForceTagging(
	TEXT("r.Streamline.ForceTagging"),
	false,
	TEXT("Force tagging Streamline resources even if they are not required based on active Streamline features (default = false)\n"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<bool> CVarStreamlineFullScreenMenuDetection(
	TEXT("r.Streamline.DLSSG.FullScreenMenuDetection"),
	false,
	TEXT("Automatically disable DLSS-FG if full screen menus are detected (default = false)\n"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarStreamlineDLSSGDynamicResolutionMode(
	TEXT("r.Streamline.DLSSG.DynamicResolutionMode"),
	0,
	TEXT("Automatically disable DLSS-FG if full screen menus are detected (default = 1)\n")
		TEXT("0: off\n")
	TEXT("1: on\n"),
	ECVF_RenderThreadSafe);


inline static int32 GetViewIndexToTag()
{
	static const auto CVarStreamlineViewIndexToTag = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.ViewIndexToTag"));
	check(CVarStreamlineViewIndexToTag);

	if (CVarStreamlineViewIndexToTag)
	{
		return CVarStreamlineViewIndexToTag->GetInt();
	}
	else
	{
		return 0;
	}
}


static int32 NumDLSSGInstances = 0;


BEGIN_SHADER_PARAMETER_STRUCT(FSLUIHintTagShaderParameters, )
SHADER_PARAMETER_TEXTURE(Texture2D, BackBuffer)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, UIColorAndAlpha)
END_SHADER_PARAMETER_STRUCT()

bool ForceTagStreamlineBuffers()
{
	static bool bStreamlineForceTagging = FParse::Param(FCommandLine::Get(), TEXT("slforcetagging"));
	return bStreamlineForceTagging || CVarStreamlineForceTagging.GetValueOnRenderThread();
}


bool ShouldTagStreamlineBuffersForDLSSFG()
{
	return ForceTagStreamlineBuffers() || IsDLSSGActive();
}


static void DLSSGAPIErrorCallBack(const sl::APIError& lastError)
{
	FStreamlineCoreModule::GetStreamlineRHI()->APIErrorHandler(lastError);
}


// TODO template shenanigans to infer from TSharedPtr mode, to allow modifed UE4 with threadsafe shared pointers work automatically
constexpr bool AreSlateSharedPointersThreadSafe()
{
#if ENGINE_MAJOR_VERSION == 4
	return false;
#else
	return true;
#endif
}

static FIntRect GetViewportRect(SWindow& InWindow)
{
	// During app shutdown, the window might not have a viewport anymore, so using SWindow::GetViewportSize() that handles that transparently.
	FIntRect ViewportRect = FIntRect(FIntPoint::ZeroValue,InWindow.GetViewportSize().IntPoint());

	
	if (AreSlateSharedPointersThreadSafe())
	{
		if (TSharedPtr<ISlateViewport> Viewport = InWindow.GetViewport())
		{
			if (TSharedPtr<SWidget> Widget = Viewport->GetWidget().Pin())
			{
				FGeometry Geom = Widget->GetPaintSpaceGeometry();

				FIntPoint Min = { int32(Geom.GetAbsolutePosition().X),int32(Geom.GetAbsolutePosition().Y) };
				FIntPoint Max = { int32((Geom.GetAbsolutePosition() + Geom.GetAbsoluteSize()).X),
									int32((Geom.GetAbsolutePosition() + Geom.GetAbsoluteSize()).Y) };

				ViewportRect = FIntRect(Min.X, Min.Y, Max.X, Max.Y);
			}
		}
	}
	else
	{
		// this is off by a bit in UE5 due to additional borders and editor UI scaling that's not present in UE4
		// but we expect to run this only in UE4, if at all
		const FSlateRect ClientRectInScreen = InWindow.GetClientRectInScreen();
		const FSlateRect ClientRectInWindow = ClientRectInScreen.OffsetBy(-InWindow.GetPositionInScreen());

		const FIntRect RectFromWindow = FIntRect(ClientRectInWindow.Left, ClientRectInWindow.Top, ClientRectInWindow.Right, ClientRectInWindow.Bottom);
		ViewportRect = RectFromWindow;
	}

	return ViewportRect;
}

static void DLSSGOnBackBufferReadyToPresent(SWindow& InWindow, const FTextureRHIRef& InBackBuffer)
{
	check(IsInRenderingThread());

	const bool bIsGameWindow = InWindow.GetType() == EWindowType::GameWindow;
#if WITH_EDITOR
	const bool bIsPIEWindow = GIsEditor && (InWindow.GetTitle().ToString().Contains(TEXT("Preview [NetMode:")));
#else
	const bool bIsPIEWindow = false;
#endif
	if (!(bIsGameWindow || bIsPIEWindow))
	{
		return;
	}

	// we need to "consume" the views for this backbuffer, even if we don't tag them
#if DEBUG_STREAMLINE_VIEW_TRACKING
	FStreamlineViewExtension::LogTrackedViews(*FString::Printf(TEXT("%s Entry %s Backbuffer=%p"), ANSI_TO_TCHAR(__FUNCTION__), *CurrentThreadName(), InBackBuffer->GetTexture2D()));
#endif
	TArray<FStreamlineViewExtension::FTrackedView>& TrackedViews = FStreamlineViewExtension::GetTrackedViews();


	// the sceneview extension (via viewfamily) knows the texture it is getting rendered into.
	// in game mode, this is the actual backbuffer (same as the argument to this callback)
	// in the editor, this is a different, intermediate rendertarget (BufferedRT)
	// so we need to handle either case to associate views to this backbuffer
	FRHITexture* RealOrBufferedBackBuffer = InBackBuffer->GetTexture2D();

	if (AreSlateSharedPointersThreadSafe())
	{
		if (TSharedPtr<ISlateViewport> Viewport = InWindow.GetViewport())
		{
			FSceneViewport* SceneViewport = static_cast<FSceneViewport*> (Viewport.Get());
			const FTextureRHIRef& SceneViewPortRenderTarget = SceneViewport->GetRenderTargetTexture();

			if (SceneViewPortRenderTarget.IsValid())
			{
#if ENGINE_MAJOR_VERSION == 4 
				check(GIsEditor);
#else
				// TODO: the following check asserts when taking a screenshot in game with F9.
				// We should fix this properly, but it's Friday afternoon so I'm changing it to a non-fatal ensure for now
				ensure(GIsEditor || (FRDGBuilder::IsDumpingFrame() && (InBackBuffer == SceneViewPortRenderTarget->GetTexture2D())));
#endif

				RealOrBufferedBackBuffer = SceneViewPortRenderTarget->GetTexture2D();
			}
		}
		else
		{
			check(!GIsEditor);
		}
	}
	else
	{
		// this is not trivial/impossible to implement without getting the window/ rendertarget information from the gamethread
		// this is OK in UE5 since by default we can talk to the gamethread from the renderthread here in a thread safe way
		// but not in UE4
	}

	// Note: we cannot empty the array after we found the views for the current backbufffer since we get multiple present callbacks in case when we have multiple 
	// swapchains / windows so selectively removing those only for the current backbuffer still keeps those around for the next time we get the present callback for a different swapchain.
	// This can happen in PIE mode with multiple active PIE windows


	TArray<FStreamlineViewExtension::FTrackedView> ViewsInThisBackBuffer;
	int32 ViewRectIndex = 0;
	while (ViewRectIndex < TrackedViews.Num())
	{
		if (TrackedViews[ViewRectIndex].Texture->GetTexture2D() == RealOrBufferedBackBuffer)
		{
			ViewsInThisBackBuffer.Add(TrackedViews[ViewRectIndex]);
			TrackedViews.RemoveAtSwap(ViewRectIndex);
		}
		else
		{
			++ViewRectIndex;
		}
	}

	const static auto CVarStreamlineViewIndexToTag = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.ViewIndexToTag"));
	if (CVarStreamlineViewIndexToTag )
	{
		if (const int32 ViewIndexToTag = CVarStreamlineViewIndexToTag->GetInt() != -1)
		{
			for (int32 ViewIndex = 0; ViewIndex < ViewsInThisBackBuffer.Num(); ++ViewIndex)
			{
				if (ViewIndex == ViewIndexToTag)
				{
					const FStreamlineViewExtension::FTrackedView ViewToTrack = ViewsInThisBackBuffer[ViewIndex];
					ViewsInThisBackBuffer.Empty();
					ViewsInThisBackBuffer.Add(ViewToTrack);
					break;
				}
			}
		}
	}

#if DEBUG_STREAMLINE_VIEW_TRACKING
	if (FStreamlineViewExtension::DebugViewTracking())
	{
		const FString ViewRectString = FString::JoinBy(ViewsInThisBackBuffer, TEXT(", "), [](const FStreamlineViewExtension::FTrackedView& State)
		{
			return FString::FromInt(State.ViewKey);
		}
		);
		UE_LOG(LogStreamline, Log, TEXT("  ViewsInThisBackBuffer=%s"), *ViewRectString);
		FStreamlineViewExtension::LogTrackedViews(*FString::Printf(TEXT("%s Exit %s Backbuffer=%p "), ANSI_TO_TCHAR(__FUNCTION__), *CurrentThreadName(), InBackBuffer->GetTexture2D()));
	}
#endif
	

	if (!ShouldTagStreamlineBuffersForDLSSFG())
	{
		return;
	}

	if (!ViewsInThisBackBuffer.Num())
	{
		return;
	}

	const bool bTagUIColorAlpha = ForceTagStreamlineBuffers() ||(GIsEditor ? CVarStreamlineEditorTagUIColorAlpha.GetValueOnRenderThread() : CVarStreamlineTagUIColorAlpha.GetValueOnRenderThread());
	const bool bTagBackbuffer = ForceTagStreamlineBuffers() || (CVarStreamlineTagBackbuffer.GetValueOnRenderThread());
	
	// TODO maybe add a helper function to add the RDG pass to tag a resource and use that everywhere
	FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	FRDGBuilder GraphBuilder(RHICmdList);

	FSLUIHintTagShaderParameters* PassParameters = GraphBuilder.AllocParameters<FSLUIHintTagShaderParameters>();
	FStreamlineRHI* RHIExtensions = FStreamlineCoreModule::GetStreamlineRHI();
	
	FIntPoint BackBufferDimension = { int32(InBackBuffer->GetTexture2D()->GetSizeX()), int32(InBackBuffer->GetTexture2D()->GetSizeY()) };
	
	const FIntRect WindowClientAreaRect = GetViewportRect(InWindow);

	// in PIE windows, the actual client area the scene gets rendered into is offset to make space
	// for the window title bar and such.
	// game mode (via -game or client configs) should have this to be 0
	const FIntPoint ViewportOffsetInWindow = WindowClientAreaRect.Min;

	// For multi view, we need to tag all off those. And be careful about lifetime of the UI buffer since that's only alive inside the RDG pass when we tag
	// backbuffer is alive through present 🤞
	for(FStreamlineViewExtension::FTrackedView& View : ViewsInThisBackBuffer)
	{
		// this is a bit weird, but we might end up having multiple view families of different number of views, but since we have only one cvar
		// we need to be careful
		View.UnscaledViewRect += ViewportOffsetInWindow;
	}
	check(!WindowClientAreaRect.IsEmpty());
	check(WindowClientAreaRect.Width() <= BackBufferDimension.X);
	check(WindowClientAreaRect.Height() <= BackBufferDimension.Y);
	check(WindowClientAreaRect.Min.X >= 0);
	check(WindowClientAreaRect.Min.Y >= 0);
	
	if(bTagBackbuffer)
	{
		PassParameters->BackBuffer = InBackBuffer;
	}
		
	if (bTagUIColorAlpha)
	{
		const float AlphaThreshold = CVarStreamlineTagUIColorAlphaThreshold.GetValueOnRenderThread();
		FRDGTextureRef UIHintTexture = AddStreamlineUIHintExtractionPass(GraphBuilder, AlphaThreshold, InBackBuffer);
		PassParameters->UIColorAndAlpha = UIHintTexture;
	}
	else
	{
		PassParameters->UIColorAndAlpha = nullptr;
	}


	// TODO generate event name for multiple view
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("Streamline Tag {Backbuffer=%u UIColorAndAlpha=%u} NumViews=%u  WindowClient%dx%d [%d,%d -> %d,%d] Texture=%s",
			bTagBackbuffer, bTagUIColorAlpha,
			ViewsInThisBackBuffer.Num(),
			WindowClientAreaRect.Width(), WindowClientAreaRect.Height(),
			WindowClientAreaRect.Min.X, WindowClientAreaRect.Min.Y,
			WindowClientAreaRect.Max.X, WindowClientAreaRect.Max.Y
			,  *BackBufferDimension.ToString()),
		PassParameters,
		ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass | ERDGPassFlags::NeverCull,
		[RHIExtensions, PassParameters, WindowClientAreaRect, ViewsInThisBackBuffer](FRHICommandListImmediate& RHICmdList) mutable
		{
			for (const FStreamlineViewExtension::FTrackedView& View : ViewsInThisBackBuffer)
			{
				FRHIStreamlineResource Backbuffer = FRHIStreamlineResource::NullResource(EStreamlineResource::Backbuffer);
				if (PassParameters->BackBuffer)
				{
					// this is not an RDG resource so we don't mark it as used
					check(PassParameters->BackBuffer);
					Backbuffer = { PassParameters->BackBuffer, View.UnscaledViewRect ,EStreamlineResource::Backbuffer };
				}

				FRHIStreamlineResource UIColorAndAlpha = FRHIStreamlineResource::NullResource(EStreamlineResource::UIColorAndAlpha);
				if (PassParameters->UIColorAndAlpha)
				{
					check(PassParameters->UIColorAndAlpha);
					PassParameters->UIColorAndAlpha->MarkResourceAsUsed();
					UIColorAndAlpha = { PassParameters->UIColorAndAlpha->GetRHI(), View.UnscaledViewRect, EStreamlineResource::UIColorAndAlpha };
				}

				const uint32 ViewID = HasViewIdOverride () ? 0 : View.ViewKey;;

				RHICmdList.EnqueueLambda(
					[RHIExtensions, UIColorAndAlpha, Backbuffer, ViewID](FRHICommandListImmediate& Cmd) mutable
					{
						RHIExtensions->TagTextures(Cmd, ViewID, { Backbuffer,UIColorAndAlpha });
					});
			}
	});
	GraphBuilder.Execute();
}

void RegisterStreamlineDLSSGHooks(FStreamlineRHI* InStreamlineRHI)
{
	UE_LOG(LogStreamline, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));

	check(IsStreamlineDLSSGSupported());

	{
		check(FSlateApplication::IsInitialized());
		FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer();

		OnBackBufferReadyToPresentHandle = SlateRenderer->OnBackBufferReadyToPresent().AddStatic(&DLSSGOnBackBufferReadyToPresent);

		// ShutdownModule is too late for this
		FSlateApplication::Get().OnPreShutdown().AddLambda(
		[]()
		{
			UE_LOG(LogStreamline, Log, TEXT("Unregistering of OnBackBufferReadyToPresent callback during FSlateApplication::OnPreShutdown"));
			FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer();
			check(SlateRenderer);

			SlateRenderer->OnBackBufferReadyToPresent().Remove(OnBackBufferReadyToPresentHandle);
		}
		);

		
	}
	UE_LOG(LogStreamline, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
}

void UnregisterStreamlineDLSSGHooks()
{
	// see  FSlateApplication::OnPreShutdown lambda in RegisterStreamlineDLSSGHooks
}

static EStreamlineFeatureSupport GStreamlineDLSSGSupport = EStreamlineFeatureSupport::NotSupported;


// this is currently unreliable so 
#define WITH_DLSS_FG_VRAM_ESTIMATE 0



namespace
{
	float GLastDLSSGFrameRate = 0.0f;
	int32 GLastDLSSGFramesPresented = 0;
#if WITH_DLSS_FG_VRAM_ESTIMATE
	float GLastDLSSGVRAMEstimate = 0;
#endif
	int32 GDLSSGMinWidthOrHeight = 0;

}


STREAMLINECORE_API EStreamlineFeatureSupport QueryStreamlineDLSSGSupport()
{
	static bool bStreamlineDLSSGSupportedInitialized = false;

	if (!bStreamlineDLSSGSupportedInitialized)
	{
	
		if (!FApp::CanEverRender( ))
		{
			GStreamlineDLSSGSupport = EStreamlineFeatureSupport::NotSupported;
		}
		else if (!IsRHIDeviceNVIDIA())
		{
			GStreamlineDLSSGSupport = EStreamlineFeatureSupport::NotSupportedIncompatibleHardware;
		}
		else if(!IsStreamlineSupported())
		{
			GStreamlineDLSSGSupport = EStreamlineFeatureSupport::NotSupported;
		}
		else
		{
			FStreamlineRHI* StreamlineRHI = GetPlatformStreamlineRHI();
			if (StreamlineRHI->IsDLSSGSupportedByRHI())
			{
				const sl::Feature Feature = sl::kFeatureDLSS_G;
				sl::Result SupportedResult = SLisFeatureSupported(Feature, *StreamlineRHI->GetAdapterInfo());
				LogStreamlineFeatureSupport(Feature, *StreamlineRHI->GetAdapterInfo());

				GStreamlineDLSSGSupport = TranslateStreamlineResult(SupportedResult);

			}
			else
			{
				GStreamlineDLSSGSupport = EStreamlineFeatureSupport::NotSupportedIncompatibleRHI;
			}
		}

		// setting this to true here so we don't recurse when we call GetDLSSGStatusFromStreamline, which calls us
		bStreamlineDLSSGSupportedInitialized = true;

		if (EStreamlineFeatureSupport::Supported == GStreamlineDLSSGSupport)
		{
			// to get the min suppported width/height
			GetDLSSGStatusFromStreamline(true);
		}
	}

	return GStreamlineDLSSGSupport;
}

bool IsStreamlineDLSSGSupported()
{
	return EStreamlineFeatureSupport::Supported == QueryStreamlineDLSSGSupport();
}

static sl::DLSSGMode SLDLSSGModeFromCvar()
{
	int32 DLSSGMode = CVarStreamlineDLSSGEnable.GetValueOnAnyThread();
	switch (DLSSGMode)
	{
	case 0:
		return sl::DLSSGMode::eOff;
	case 1:
		return sl::DLSSGMode::eOn;
	case 2:
		return sl::DLSSGMode::eAuto;
	default:
		UE_LOG(LogStreamline, Error, TEXT("Invalid r.Streamline.DLSSG.Enable value %d"), DLSSGMode);
		return sl::DLSSGMode::eOff;
	}
}

bool IsDLSSGActive()
{
	if (!IsStreamlineDLSSGSupported())
	{
		return false;
	}
	else
	{
		return SLDLSSGModeFromCvar() != sl::DLSSGMode::eOff ? true : false;
	}
}

int32 GetStreamlineDLSSGNumFramesToGenerate()
{
	// currently Streamline only supports 1
	return 1;
}

DECLARE_STATS_GROUP(TEXT("DLSS-G"), STATGROUP_DLSSG, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT(TEXT("DLSS-G: Frames Presented"), STAT_DLSSGFramesPresented, STATGROUP_DLSSG);
DECLARE_FLOAT_COUNTER_STAT(TEXT("DLSS-G: Average FPS"), STAT_DLSSGAverageFPS, STATGROUP_DLSSG);
#if WITH_DLSS_FG_VRAM_ESTIMATE
DECLARE_FLOAT_COUNTER_STAT(TEXT("DLSS-G: VRAM Estimate (MiB)"), STAT_DLSSGVRAMEstimate, STATGROUP_DLSSG);
#endif
DECLARE_DWORD_COUNTER_STAT(TEXT("DLSS-G: Minimum Width or Height "), STAT_DLSSGMinWidthOrHeight, STATGROUP_DLSSG);


namespace sl
{

	inline const char* getDLSSGStatusAsStr(DLSSGStatus v)
	{
		switch (v)
		{
			SL_CASE_STR(DLSSGStatus::eOk);
			SL_CASE_STR(DLSSGStatus::eFailResolutionTooLow);
			SL_CASE_STR(DLSSGStatus::eFailReflexNotDetectedAtRuntime);
			SL_CASE_STR(DLSSGStatus::eFailHDRFormatNotSupported);
			SL_CASE_STR(DLSSGStatus::eFailCommonConstantsInvalid);
			SL_CASE_STR(DLSSGStatus::eFailGetCurrentBackBufferIndexNotCalled);
		};
		return "Unknown";
	}

}


void GetDLSSGStatusFromStreamline(bool bQueryOncePerAppLifetimeValues)
{
	extern ENGINE_API float GAverageFPS;

	GLastDLSSGFrameRate = GAverageFPS;
	GLastDLSSGFramesPresented = 1;


#if WITH_DLSS_FG_VRAM_ESTIMATE
	GLastDLSSGVRAMEstimate = 0;
#endif

	if (bQueryOncePerAppLifetimeValues)
	{
		GDLSSGMinWidthOrHeight = 0;
	}

	if (IsStreamlineDLSSGSupported())
	{
		// INSERT AWKWARD MUPPET FACE HERE


		// below we disable FG if we are using actual view ids, see SetStreamlineDLSSGState
		//checkf(CVarStreamlineViewIdOverride && CVarStreamlineViewIdOverride->GetInt() != 0, TEXT("r.Streamline.ViewIdOverride must be set to 1 since DLSS-G only supports a single viewport."));

		sl::ViewportHandle Viewport(0);

		sl::DLSSGState State;

		sl::DLSSGOptions StreamlineConstantsDLSSG;

#if WITH_DLSS_FG_VRAM_ESTIMATE
		StreamlineConstantsDLSSG.flags = sl::DLSSGFlags::eRequestVRAMEstimate;
#endif
		StreamlineConstantsDLSSG.mode = (!HasViewIdOverride()) ?  SLDLSSGModeFromCvar() : sl::DLSSGMode::eOff;

		// TODO incorporate the checks (foreground, viewport large enough) from SetStreamlineDLSSGState
		StreamlineConstantsDLSSG.numFramesToGenerate = GetStreamlineDLSSGNumFramesToGenerate();

		CALL_SL_FEATURE_FN(sl::kFeatureDLSS_G, slDLSSGGetState, Viewport, State, &StreamlineConstantsDLSSG);


		GLastDLSSGFramesPresented = State.numFramesActuallyPresented;
		SET_DWORD_STAT(STAT_DLSSGFramesPresented, GLastDLSSGFramesPresented);

		GLastDLSSGFrameRate = GAverageFPS * GLastDLSSGFramesPresented;
		SET_FLOAT_STAT(STAT_DLSSGAverageFPS, GLastDLSSGFrameRate);

#if WITH_DLSS_FG_VRAM_ESTIMATE
		GLastDLSSGVRAMEstimate = float(State.estimatedVRAMUsageInBytes) / (1024 * 1024);
		SET_FLOAT_STAT(STAT_DLSSGVRAMEstimate, GLastDLSSGVRAMEstimate);
#endif
		if (bQueryOncePerAppLifetimeValues)
		{
			GDLSSGMinWidthOrHeight = State.minWidthOrHeight;
			SET_DWORD_STAT(STAT_DLSSGMinWidthOrHeight, GDLSSGMinWidthOrHeight);

		}

#if DO_CHECK
		if (CVarStreamlineDLSSGCheckStatusPerFrame.GetValueOnAnyThread())
		{
			checkf(State.status == sl::DLSSGStatus::eOk, TEXT("DLSS-FG failed at runtime with %s (%d). This runtime check can be disabled with the r.Streamline.DLSSG.CheckStatusPerFrame console variable"),
				ANSI_TO_TCHAR(sl::getDLSSGStatusAsStr(State.status)), State.status);
		}
#endif
	}


}
STREAMLINECORE_API void GetStreamlineDLSSGFrameTiming(float& FrameRateInHertz, int32& FramesPresented)
{
	FrameRateInHertz = GLastDLSSGFrameRate;
	FramesPresented = GLastDLSSGFramesPresented;
}
namespace
{

void SetStreamlineDLSSGState(FRHICommandListImmediate& RHICmdList, uint32 ViewID, const FIntRect& SecondaryViewRect, bool bEnableFullScreenMenuDetection, bool bEnableDynamicResolution)
{
	if (IsStreamlineDLSSGSupported())
	{
#if (ENGINE_MAJOR_VERSION == 4)
		const bool bIsForeground = FApp::HasVRFocus() || FApp::IsBenchmarking() || FPlatformApplicationMisc::IsThisApplicationForeground();
#else
		const bool bIsForeground = FApp::HasFocus();
#endif
		const bool bIsLargeEnough = FMath::Min( SecondaryViewRect.Width(), SecondaryViewRect.Height()) >= GDLSSGMinWidthOrHeight;
		
		sl::DLSSGMode DLSSGMode = (bIsForeground && bIsLargeEnough) ? SLDLSSGModeFromCvar() : sl::DLSSGMode::eOff;
		
		sl::DLSSGFlags DLSSGFlags  = static_cast<sl::DLSSGFlags>(0);

		if (bEnableFullScreenMenuDetection)
		{
			EnumAddFlags(DLSSGFlags, sl::DLSSGFlags::eEnableFullscreenMenuDetection);
		}
		
		if (bEnableDynamicResolution)
		{
			EnumAddFlags(DLSSGFlags, sl::DLSSGFlags::eDynamicResolutionEnabled);
		}

		RHICmdList.EnqueueLambda(
			[ViewID, DLSSGMode, DLSSGFlags](FRHICommandListImmediate& Cmd) mutable
			{
				sl::DLSSGOptions StreamlineConstantsDLSSG;
				StreamlineConstantsDLSSG.mode = DLSSGMode;
				StreamlineConstantsDLSSG.numFramesToGenerate = GetStreamlineDLSSGNumFramesToGenerate();
				StreamlineConstantsDLSSG.flags = DLSSGFlags;
				StreamlineConstantsDLSSG.onErrorCallback = DLSSGAPIErrorCallBack;
				CALL_SL_FEATURE_FN(sl::kFeatureDLSS_G, slDLSSGSetOptions, sl::ViewportHandle(ViewID), StreamlineConstantsDLSSG);
			});
	}
}


	BEGIN_SHADER_PARAMETER_STRUCT(FSLDLSSGShaderParameters, )
	END_SHADER_PARAMETER_STRUCT()

}

void AddStreamlineDLSSGStateRenderPass(FRDGBuilder& GraphBuilder, uint32 ViewID, const FIntRect& SecondaryViewRect)
{

	FSLDLSSGShaderParameters* PassParameters = GraphBuilder.AllocParameters<FSLDLSSGShaderParameters>();


	GraphBuilder.AddPass(
		RDG_EVENT_NAME("Streamline DLSS-G ViewID=%u", ViewID),
		PassParameters,
		ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass | ERDGPassFlags::NeverCull,
		[PassParameters, ViewID, SecondaryViewRect](FRHICommandListImmediate& RHICmdList) mutable
		{
			const bool bEnableFullScreenMenuDetection = CVarStreamlineFullScreenMenuDetection.GetValueOnRenderThread();
			
			// TODO implement automatic mode
			const bool bEnableDynamicResolution = CVarStreamlineDLSSGDynamicResolutionMode.GetValueOnAnyThread() != 0;

			RHICmdList.EnqueueLambda(
				[ViewID, SecondaryViewRect, bEnableFullScreenMenuDetection, bEnableDynamicResolution](FRHICommandListImmediate& Cmd) mutable
				{
					SetStreamlineDLSSGState(Cmd, ViewID, SecondaryViewRect, bEnableFullScreenMenuDetection, bEnableDynamicResolution);
				});
		});
}

void BeginRenderViewFamilyDLSSG(FSceneViewFamily& InViewFamily)
{
	if(IsDLSSGActive() && CVarStreamlineDLSSGAdjustMotionBlurTimeScale.GetValueOnAnyThread() && InViewFamily.Views.Num())
	{
		// this is 1 when FG is off (or auto modes turns it off)
		const int32 PresentedFrames = CVarStreamlineDLSSGAdjustMotionBlurTimeScale.GetValueOnAnyThread() == 2 ? FMath::Max(1, GLastDLSSGFramesPresented) : 1 + GetStreamlineDLSSGNumFramesToGenerate();
		const float TimeScaleCorrection = 1.0f / float(PresentedFrames);

		for (int32 ViewIndex = 0; ViewIndex < InViewFamily.Views.Num(); ++ViewIndex)
		{
			if (FSceneViewStateInterface* ViewStateInterface = InViewFamily.Views[ViewIndex]->State)
			{
				// The things we do to avoid engine changes ...
				FSceneViewState* ViewState = static_cast<FSceneViewState*>(ViewStateInterface);
				
				float& MotionBlurTimeScale = ViewState->MotionBlurTimeScale;
				float& MotionBlurTargetDeltaTime = ViewState->MotionBlurTargetDeltaTime;

				MotionBlurTimeScale *= TimeScaleCorrection;
				MotionBlurTargetDeltaTime *= TimeScaleCorrection;
			}
		}
	}
}
