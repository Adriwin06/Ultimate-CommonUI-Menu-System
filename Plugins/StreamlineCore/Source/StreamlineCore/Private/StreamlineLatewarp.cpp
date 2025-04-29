/*
* Copyright (c) 2022 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/

#include "StreamlineLatewarp.h"
#include "StreamlineCore.h"
#include "StreamlineCorePrivate.h"
#include "StreamlineShaders.h"
#include "StreamlineViewExtension.h"
#include "StreamlineAPI.h"
#include "StreamlineRHI.h"

#include "UIHintExtractionPass.h"
#include "CoreMinimal.h"
#include "RenderGraphBuilder.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SystemTextures.h"
#include "SceneTextureParameters.h"
#include "PostProcess/PostProcessMaterial.h"


#include "sl_helpers.h"
#if WITH_LATEWARP
#include "sl_latewarp.h"
#endif

static int32 NumLatewarpInstances = 0;
static FDelegateHandle LatewarpOnBackBufferReadyToPresentHandle;

static TAutoConsoleVariable<int32> CVarLatewarpEnable(
	TEXT("r.Streamline.Latewarp.Enable"), 0,
	TEXT("Enable/disable Latewarp (default = 1)\n"),
	ECVF_Default);

DEFINE_LOG_CATEGORY_STATIC(LogStreamlineLatewarp, Log, All);

DECLARE_GPU_STAT(Latewarp)


static FIntRect LatewarpGetViewportRect(SWindow& InWindow)
{
	// During app shutdown, the window might not have a viewport anymore, so using SWindow::GetViewportSize() that handles that transparently.
	FIntRect ViewportRect = FIntRect(FIntPoint::ZeroValue,InWindow.GetViewportSize().IntPoint());
	
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

	return ViewportRect;
}
#if	((ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 1))
static void LatewarpOnBackBufferReadyToPresent(SWindow& InWindow, const FTextureRHIRef& InBackBuffer)
#else
static void LatewarpOnBackBufferReadyToPresent(SWindow& InWindow, const FTexture2DRHIRef& InBackBuffer)
#endif
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

	// TODO maybe add a helper function to add the RDG pass to tag a resource and use that everywhere
	FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	FRDGBuilder GraphBuilder(RHICmdList);

	FSLUIHintTagShaderParameters* PassParameters = GraphBuilder.AllocParameters<FSLUIHintTagShaderParameters>();
	
	const float AlphaThreshold= 0.0f;
	FRDGTextureRef UIHintTexture = AddStreamlineUIHintExtractionPass(GraphBuilder, AlphaThreshold, InBackBuffer);
	PassParameters->UIColorAndAlpha = UIHintTexture;
	PassParameters->BackBuffer = InBackBuffer;
#if	((ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 1))
	FIntPoint BackBufferDimension = { int32(InBackBuffer->GetDesc().Extent.X), int32(InBackBuffer->GetDesc().Extent.Y) };
#else
	FIntPoint BackBufferDimension = { int32(InBackBuffer->GetTexture2D()->GetSizeX()), int32(InBackBuffer->GetTexture2D()->GetSizeY()) };
#endif
	
	FStreamlineRHI* RHIExtensions = FStreamlineCoreModule::GetStreamlineRHI();
	TArray<FTrackedView>& TrackedViews = FStreamlineViewExtension::GetTrackedViews();
	TArray<FTrackedView> ViewsInThisBackBuffer;
	int32 ViewRectIndex = 0;
#if	((ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 1))
	FRHITextureReference* RealOrBufferedBackBuffer = InBackBuffer->GetTextureReference();
#else
	FRHITexture2D* RealOrBufferedBackBuffer = InBackBuffer->GetTexture2D();
#endif
	while (ViewRectIndex < TrackedViews.Num())
	{

#if	((ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 1))
		if (TrackedViews[ViewRectIndex].Texture->GetTextureReference() == RealOrBufferedBackBuffer)
#else
		if (TrackedViews[ViewRectIndex].Texture->GetTexture2D() == RealOrBufferedBackBuffer)
#endif
		{
			ViewsInThisBackBuffer.Add(TrackedViews[ViewRectIndex]);
			TrackedViews.RemoveAtSwap(ViewRectIndex);
		}
		else
		{
			++ViewRectIndex;
		}
	}
	const FIntRect WindowClientAreaRect = LatewarpGetViewportRect(InWindow);
	AddStreamlineUIHintTagPass(GraphBuilder, true, true, BackBufferDimension, PassParameters, 0, RHIExtensions, ViewsInThisBackBuffer, WindowClientAreaRect, true);
}

void RegisterStreamlineLatewarpHooks(FStreamlineRHI* InStreamlineRHI)
{
#if WITH_LATEWARP
	UE_LOG(LogStreamline, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));

	check(ShouldTagStreamlineBuffers() || IsStreamlineLatewarpSupported());

	{
		check(FSlateApplication::IsInitialized());
		FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer();

		LatewarpOnBackBufferReadyToPresentHandle = SlateRenderer->OnBackBufferReadyToPresent().AddStatic(&LatewarpOnBackBufferReadyToPresent);

		// ShutdownModule is too late for this
		FSlateApplication::Get().OnPreShutdown().AddLambda(
		[]()
		{
			UE_LOG(LogStreamline, Log, TEXT("Unregistering of OnBackBufferReadyToPresent callback during FSlateApplication::OnPreShutdown"));
			FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer();
			check(SlateRenderer);

			SlateRenderer->OnBackBufferReadyToPresent().Remove(LatewarpOnBackBufferReadyToPresentHandle);
		}
		);

		
	}
	UE_LOG(LogStreamline, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
#else
	return;
#endif
}
void UnregisterStreamlineLatewarpHooks()
{
	// see  FSlateApplication::OnPreShutdown lambda in RegisterStreamlineDLSSGHooks
}
static bool IsStreamlineLatewarpSupportedInternal()
{
	static bool bStreamlineLatewarpSupportedInitialized = false;
	static bool bStreamlineLatewarpSupported = false;

	if (!bStreamlineLatewarpSupportedInitialized)
	{
		FStreamlineRHI* StreamlineRHI = GetPlatformStreamlineRHI();
		sl::Result Result = SLisFeatureSupported(sl::kFeatureLatewarp, *StreamlineRHI->GetAdapterInfo());
		bStreamlineLatewarpSupported = (Result == sl::Result::eOk);

		bStreamlineLatewarpSupportedInitialized = true;
	}
	return bStreamlineLatewarpSupported;
}

static Streamline::EStreamlineFeatureSupport GStreamlineLatewarpSupport = Streamline::EStreamlineFeatureSupport::NotSupported;

STREAMLINECORE_API Streamline::EStreamlineFeatureSupport QueryStreamlineLatewarpSupport()
{
#if WITH_LATEWARP
	static bool bStreamlineLatewarpSupportedInitialized = false;

	if (!bStreamlineLatewarpSupportedInitialized)
	{
	
		if (!FApp::CanEverRender( ))
		{
			GStreamlineLatewarpSupport = Streamline::EStreamlineFeatureSupport::NotSupported;
		}
		else if (!IsRHIDeviceNVIDIA())
		{
			GStreamlineLatewarpSupport = Streamline::EStreamlineFeatureSupport::NotSupportedIncompatibleHardware;
		}
		else if(!IsStreamlineSupported())
		{
			GStreamlineLatewarpSupport = Streamline::EStreamlineFeatureSupport::NotSupported;
		}
		else
		{
			FStreamlineRHI* StreamlineRHI = GetPlatformStreamlineRHI();
			if (StreamlineRHI->IsLatewarpSupportedByRHI())
			{
				const sl::Feature Feature = sl::kFeatureLatewarp;
				sl::Result SupportedResult = SLisFeatureSupported(Feature, *StreamlineRHI->GetAdapterInfo());
				LogStreamlineFeatureSupport(Feature, *StreamlineRHI->GetAdapterInfo());

				GStreamlineLatewarpSupport = TranslateStreamlineResult(SupportedResult);

			}
			else
			{
				GStreamlineLatewarpSupport = Streamline::EStreamlineFeatureSupport::NotSupportedIncompatibleRHI;
			}
		}

		// setting this to true here so we don't recurse when we call GetLatewarpStatusFromStreamline, which calls us
		bStreamlineLatewarpSupportedInitialized = true;
	}

	return GStreamlineLatewarpSupport;
#else
	return  Streamline::EStreamlineFeatureSupport::NotSupported;
#endif
}

bool IsStreamlineLatewarpSupported()
{
#if WITH_LATEWARP
	if (!FApp::CanEverRender())
	{
		return false;
	}

	if (!IsRHIDeviceNVIDIA())
	{
		return false;
	}

	if (!IsStreamlineSupported())
	{
		return false;
	}

	if (GIsEditor)
	{
		return false;
	}

	return IsStreamlineLatewarpSupportedInternal();
#else
	return false;
#endif
}

bool IsLatewarpActive()
{
#if WITH_LATEWARP
	if (!IsStreamlineLatewarpSupportedInternal())
	{
		return false;
	}
	else
	{
		return CVarLatewarpEnable.GetValueOnAnyThread() != 0;
	}
#else
	return false;
#endif
}

void AddStreamlineLatewarpStateRenderPass(FRDGBuilder& GraphBuilder, uint32 ViewID, const FIntRect& SecondaryViewRect)
{
#if WITH_LATEWARP
	AddStreamlineStateRenderPass(TEXT("Latewarp"), GraphBuilder, ViewID, SecondaryViewRect,
		// this lambda computes the SL options struct based on cvars and other state
		[](uint32 ViewID, const FIntRect& SecondaryViewRect) ->sl::LatewarpOptions
		{
			// the callsite is expcted to not call this, so we don't need to if bail out here
			check(IsStreamlineLatewarpSupported());
			check(IsInRenderingThread());
			
			sl:: LatewarpOptions{};
			
			sl::LatewarpOptions SLConstants;

			// TODO: implement when we have an SDK RC that has that implemented
			//SLConstants.onErrorCallback = DLSSGAPIErrorCallBack;
			

#if (ENGINE_MAJOR_VERSION == 4)
			const bool bIsForeground = FApp::HasVRFocus() || FApp::IsBenchmarking() || FPlatformApplicationMisc::IsThisApplicationForeground();
#else
			const bool bIsForeground = FApp::HasFocus();
#endif
			const bool bIsLargeEnough = true; // FMath::Min(SecondaryViewRect.Width(), SecondaryViewRect.Height()) >= GDLSSGMinWidthOrHeight;


			// TODO hook up to cvar 
			// 
			SLConstants.latewarpActive = (true ||  bIsForeground && bIsLargeEnough) ? IsLatewarpActive() : false;
		
			return SLConstants;
		},
		// this lambda is only here since templating the function pointer and functin name and such below is inconvenient
		[](FRHICommandListImmediate& RHICmdList, uint32 ViewID, const FIntRect& SecondaryViewRect, const sl::LatewarpOptions& Options)
		{
			CALL_SL_FEATURE_FN(sl::kFeatureLatewarp, slLatewarpSetOptions, sl::ViewportHandle(ViewID), Options);
		}
	);
#else
	return;
#endif
}