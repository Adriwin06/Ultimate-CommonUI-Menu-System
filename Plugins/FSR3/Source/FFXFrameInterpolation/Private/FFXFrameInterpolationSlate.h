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

#pragma once

#include "CoreMinimal.h"
#include "Rendering/SlateRenderer.h"
#include "Rendering/SlateDrawBuffer.h"
#include "Slate/SlateTextures.h"
#include "Widgets/Accessibility/SlateAccessibleMessageHandler.h"
#include "Framework/Application/SlateApplication.h"
#include "FFXShared.h"
#if UE_VERSION_OLDER_THAN(5, 0, 0)
#include "PixelFormat.h"
#endif

//------------------------------------------------------------------------------------------------------
// Slate override code that allows Frame Interpolation to re-render and present both the Interpolated and Real frame.
//------------------------------------------------------------------------------------------------------
class FFXFrameInterpolationSlateRenderer : public FSlateRenderer
{
	static const uint32 NumDrawBuffers = 6;
public:
	void OnSlateWindowRenderedThunk(SWindow& Window, void* Ptr) { return SlateWindowRendered.Broadcast(Window, Ptr); }

	void OnSlateWindowDestroyedThunk(void* Ptr) { return OnSlateWindowDestroyedDelegate.Broadcast(Ptr); }

	void OnPreResizeWindowBackBufferThunk(void* Ptr) { return PreResizeBackBufferDelegate.Broadcast(Ptr); }

	void OnPostResizeWindowBackBufferThunk(void* Ptr) { return PostResizeBackBufferDelegate.Broadcast(Ptr); }

#if UE_VERSION_AT_LEAST(5, 5, 0)
	void OnBackBufferReadyToPresentThunk(SWindow& Window, const FTextureRHIRef& Texture) { return OnBackBufferReadyToPresentDelegate.Broadcast(Window, Texture); }
#else
	void OnBackBufferReadyToPresentThunk(SWindow& Window, const FTexture2DRHIRef& Texture) { return OnBackBufferReadyToPresentDelegate.Broadcast(Window, Texture); }
#endif

	FFXFrameInterpolationSlateRenderer(TSharedRef<FSlateRenderer> InUnderlyingRenderer);
	virtual ~FFXFrameInterpolationSlateRenderer();

#if UE_VERSION_AT_LEAST(5, 1, 0)
	/** Returns a draw buffer that can be used by Slate windows to draw window elements */
	virtual FSlateDrawBuffer& AcquireDrawBuffer();

	virtual void ReleaseDrawBuffer(FSlateDrawBuffer& InWindowDrawBuffer);
#else
	/** Returns a draw buffer that can be used by Slate windows to draw window elements */
	virtual FSlateDrawBuffer& GetDrawBuffer();
#endif

	virtual bool Initialize();
	virtual void Destroy();
	virtual void CreateViewport(const TSharedRef<SWindow> InWindow);
	virtual void RequestResize(const TSharedPtr<SWindow>& InWindow, uint32 NewSizeX, uint32 NewSizeY);
	virtual void UpdateFullscreenState(const TSharedRef<SWindow> InWindow, uint32 OverrideResX = 0, uint32 OverrideResY = 0);
	virtual void SetSystemResolution(uint32 Width, uint32 Height);
	virtual void RestoreSystemResolution(const TSharedRef<SWindow> InWindow);
	virtual void DrawWindows(FSlateDrawBuffer& InWindowDrawBuffer);
	virtual void SetColorVisionDeficiencyType(EColorVisionDeficiency Type, int32 Severity, bool bCorrectDeficiency, bool bShowCorrectionWithDeficiency);
	virtual FIntPoint GenerateDynamicImageResource(const FName InTextureName);
	virtual bool GenerateDynamicImageResource(FName ResourceName, uint32 Width, uint32 Height, const TArray< uint8 >& Bytes);

	virtual bool GenerateDynamicImageResource(FName ResourceName, FSlateTextureDataRef TextureData);

#if UE_VERSION_AT_LEAST(5, 1, 0)
	virtual FSlateResourceHandle GetResourceHandle(const FSlateBrush& Brush, FVector2f LocalSize, float DrawScale);
#elif UE_VERSION_AT_LEAST(5, 0, 0)
	virtual FSlateResourceHandle GetResourceHandle(const FSlateBrush& Brush, FVector2D LocalSize, float DrawScale);
#endif

	virtual FSlateResourceHandle GetResourceHandle(const FSlateBrush& Brush);

	virtual bool CanRenderResource(UObject& InResourceObject) const;

	virtual void RemoveDynamicBrushResource(TSharedPtr<FSlateDynamicImageBrush> BrushToRemove);

	virtual void ReleaseDynamicResource(const FSlateBrush& Brush);

	virtual void OnWindowDestroyed(const TSharedRef<SWindow>& InWindow);

	virtual void OnWindowFinishReshaped(const TSharedPtr<SWindow>& InWindow);

	virtual void* GetViewportResource(const SWindow& Window);

	virtual void FlushCommands() const;

	virtual void Sync() const;

	virtual void BeginFrame() const;

	virtual void EndFrame() const;

	virtual void ReloadTextureResources();

	virtual void LoadStyleResources(const ISlateStyle& Style);

	virtual bool AreShadersInitialized() const;

	virtual void InvalidateAllViewports();

	virtual void ReleaseAccessedResources(bool bImmediatelyFlush);

	virtual void PrepareToTakeScreenshot(const FIntRect& Rect, TArray<FColor>* OutColorData, SWindow* InScreenshotWindow);

#if UE_VERSION_OLDER_THAN(5, 5, 0)
	virtual void SetWindowRenderTarget(const SWindow& Window, class IViewportRenderTargetProvider* Provider);
#endif

	virtual FSlateUpdatableTexture* CreateUpdatableTexture(uint32 Width, uint32 Height);

#if UE_VERSION_AT_LEAST(5, 0, 0)
	virtual FSlateUpdatableTexture* CreateSharedHandleTexture(void* SharedHandle);
#endif

	virtual void ReleaseUpdatableTexture(FSlateUpdatableTexture* Texture);

	virtual ISlateAtlasProvider* GetTextureAtlasProvider();

	virtual ISlateAtlasProvider* GetFontAtlasProvider();

	virtual void CopyWindowsToVirtualScreenBuffer(const TArray<FString>& KeypressBuffer);

	virtual void MapVirtualScreenBuffer(FMappedTextureBuffer* OutImageData);
	virtual void UnmapVirtualScreenBuffer();

	virtual FCriticalSection* GetResourceCriticalSection();

	virtual int32 RegisterCurrentScene(FSceneInterface* Scene);

	virtual int32 GetCurrentSceneIndex() const;

	virtual void ClearScenes();

	virtual void DestroyCachedFastPathRenderingData(struct FSlateCachedFastPathRenderingData* VertexData);
	virtual void DestroyCachedFastPathElementData(struct FSlateCachedElementData* ElementData);

	virtual bool HasLostDevice() const;

	virtual void AddWidgetRendererUpdate(const struct FRenderThreadUpdateContext& Context, bool bDeferredRenderTargetUpdate);

	virtual EPixelFormat GetSlateRecommendedColorFormat();

#if UE_VERSION_AT_LEAST(5, 5, 0)
	void SetCurrentSceneIndex(int32 index) override;
#endif

private:
	FSlateDrawBuffer DrawBuffers[NumDrawBuffers];
	TArray<TSharedPtr<FSlateDynamicImageBrush>> DynamicBrushesToRemove[NumDrawBuffers];
	TSharedPtr<FSlateRenderer> UnderlyingRenderer;
	uint32 FreeBufferIndex;
	uint32 ResourceVersion;
};

//------------------------------------------------------------------------------------------------------
// Accessor for the Slate application so that we can swizzle the renderer.
//------------------------------------------------------------------------------------------------------
#if UE_VERSION_AT_LEAST(4, 26, 0) && UE_VERSION_OLDER_THAN(5, 6, 0)
class FFXFISlateApplicationAccessor
{
public:
	FFXFISlateApplicationAccessor()
	: HitTesting(&FSlateApplication::Get())
	{}
	virtual ~FFXFISlateApplicationAccessor() {}

	DECLARE_EVENT_OneParam(FSlateApplicationBase, FOnInvalidateAllWidgets, bool);
	DECLARE_EVENT_OneParam(FSlateApplicationBase, FOnGlobalInvalidationToggled, bool);
	TArray<TWeakPtr<FActiveTimerHandle>> ActiveTimerHandles;
	enum class ECustomSafeZoneState : uint8
	{
		Unset,
		Set,
		Debug
	};

public:
	const static uint32 CursorPointerIndex;
	const static uint32 CursorUserIndex;
#if UE_VERSION_AT_LEAST(4, 27, 0)
	const static FPlatformUserId SlateAppPrimaryPlatformUser;
#endif
	TSharedPtr<FSlateRenderer> Renderer;
	FHitTesting HitTesting;
	static TSharedPtr<FSlateApplicationBase> CurrentBaseApplication;
	static TSharedPtr<class GenericApplication> PlatformApplication;
	FDisplayMetrics CachedDisplayMetrics;
	float CachedDebugTitleSafeRatio;
#if WITH_EDITORONLY_DATA
	FOnDebugSafeZoneChanged OnDebugSafeZoneChanged;
#endif
#if WITH_ACCESSIBILITY
	TSharedRef<FSlateAccessibleMessageHandler> AccessibleMessageHandler;
#endif
	FOnInvalidateAllWidgets OnInvalidateAllWidgetsEvent;
	FOnGlobalInvalidationToggled OnGlobalInvalidationToggledEvent;
	FCriticalSection ActiveTimerCS;
	bool bIsSlateAsleep;
#if UE_VERSION_AT_LEAST(4, 27, 0)
	ECustomSafeZoneState CustomSafeZoneState;
#endif
#if UE_VERSION_AT_LEAST(4, 27, 0) || WITH_EDITORONLY_DATA
	FMargin CustomSafeZoneRatio;
#endif
};
static_assert(sizeof(FSlateApplicationBase) == sizeof(FFXFISlateApplicationAccessor), "FFXFISlateApplicationAccessor must match the layout of FSlateApplicationBase so we can access the renderer!");

class FFXFISlateApplication
	: public FFXFISlateApplicationAccessor
	, public FGenericApplicationMessageHandler
{
public:
	DECLARE_MULTICAST_DELEGATE_FiveParams(FOnFocusChanging, const FFocusEvent&, const FWeakWidgetPath&, const TSharedPtr<SWidget>&, const FWidgetPath&, const TSharedPtr<SWidget>&);
#if WITH_EDITORONLY_DATA
	FDragDropCheckingOverride OnDragDropCheckOverride;
#endif
	TSet<FKey> PressedMouseButtons;
	bool bAppIsActive;
	bool bSlateWindowActive;
	bool bRenderOffScreen;
	float Scale;
	float DragTriggerDistance;
	TArray< TSharedRef<SWindow> > SlateWindows;
	TArray< TSharedRef<SWindow> > SlateVirtualWindows;
	TWeakPtr<SWindow> ActiveTopLevelWindow;
	TArray< TSharedPtr<SWindow> > ActiveModalWindows;
	TArray< TSharedRef<SWindow> > WindowDestroyQueue;
	FMenuStack MenuStack;
	float CursorRadius;
	TArray<TSharedPtr<FSlateUser>> Users;
	TArray<TWeakPtr<FSlateVirtualUserHandle>> VirtualUsers;
	TWeakPtr<SWidget> LastAllUsersFocusWidget;
	EFocusCause LastAllUsersFocusCause;
#if UE_VERSION_AT_LEAST(5, 2, 0)
	TWeakPtr<SWidget> CurrentDebugContextWidget;
	TWeakPtr<SWindow> CurrentDebuggingWindow;
#endif
	FThrottleRequest MouseButtonDownResponsivnessThrottle;
	FThrottleRequest UserInteractionResponsivnessThrottle;
	double LastUserInteractionTime;
	double LastUserInteractionTimeForThrottling;
	DECLARE_EVENT_OneParam(FFXFISlateApplication, FSlateLastUserInteractionTimeUpdateEvent, double);
	FSlateLastUserInteractionTimeUpdateEvent LastUserInteractionTimeUpdateEvent;
	double LastMouseMoveTime;
	FPopupSupport PopupSupport;
	TWeakPtr<SViewport> GameViewportWidget;
#if WITH_EDITOR
	TSet<TWeakPtr<SViewport>> AllGameViewports;
#if UE_VERSION_AT_LEAST(5, 4, 0)
	TArray<TPair<FText, int32>> PreventDebuggingModeStack;
#endif
#endif
#if UE_VERSION_AT_LEAST(5, 4, 0)
	TWeakPtr<SNotificationItem> DebuggingModeNotificationMessage;
#endif
	TSharedPtr<ISlateSoundDevice> SlateSoundDevice;
	double CurrentTime;
	double LastTickTime;
	float AverageDeltaTime;
	float AverageDeltaTimeForResponsiveness;
	FSimpleDelegate OnExitRequested;
	TWeakPtr<IWidgetReflector> WidgetReflectorPtr;
	FAccessSourceCode SourceCodeAccessDelegate;
	FQueryAccessSourceCode QuerySourceCodeAccessDelegate;
	FAccessAsset AssetAccessDelegate;
	int32 NumExternalModalWindowsActive;
	TArray<FOnWindowAction> OnWindowActionNotifications;
	const class FStyleNode* RootStyleNode;
	bool bRequestLeaveDebugMode;
	bool bLeaveDebugForSingleStep;
	TAttribute<bool> NormalExecutionGetter;
	FModalWindowStackStarted ModalWindowStackStartedDelegate;
	FModalWindowStackEnded ModalWindowStackEndedDelegate;
	bool bIsExternalUIOpened;
	FThrottleRequest ThrottleHandle;
	bool DragIsHandled;
	TUniquePtr<IPlatformTextField> SlateTextField;
	bool bIsFakingTouch;
	bool bIsGameFakingTouch;
	bool bIsFakingTouched;
#if UE_VERSION_AT_LEAST(5, 4, 0)
	bool bAllowFakingTouch;
#endif
	bool bHandleDeviceInputWhenApplicationNotActive;
	FOnKeyEvent UnhandledKeyDownEventHandler;
	FOnKeyEvent UnhandledKeyUpEventHandler;
	bool bTouchFallbackToMouse;
	bool bSoftwareCursorAvailable;
	bool bMenuAnimationsEnabled;
	const FSlateBrush* AppIcon;
	DECLARE_EVENT_OneParam(FFXFISlateApplication, FApplicationActivationStateChangedEvent, const bool /*IsActive*/)
	FApplicationActivationStateChangedEvent ApplicationActivationStateChangedEvent;
	FSlateRect VirtualDesktopRect;
	TSharedRef<FNavigationConfig> NavigationConfig;
#if WITH_EDITOR
	TSharedRef<FNavigationConfig> EditorNavigationConfig;
#endif
	TBitArray<FDefaultBitArrayAllocator> SimulateGestures;
	DECLARE_EVENT_OneParam(FFXFISlateApplication, FSlateTickEvent, float);
	FSlateTickEvent PreTickEvent;
	FSlateTickEvent PostTickEvent;
	FSimpleMulticastDelegate PreShutdownEvent;
	DECLARE_EVENT_OneParam(FFXFISlateApplication, FUserRegisteredEvent, int32);
	FUserRegisteredEvent UserRegisteredEvent;
	DECLARE_EVENT_OneParam(FFXFISlateApplication, FOnWindowBeingDestroyed, const SWindow&);
	FOnWindowBeingDestroyed WindowBeingDestroyedEvent;
#if UE_VERSION_AT_LEAST(5, 5, 0)
	FOnMenuDestroyed MenuBeingDestroyedEvent;
#endif
	DECLARE_EVENT_OneParam(FFXFISlateApplication, FOnModalLoopTickEvent, float);
	FOnModalLoopTickEvent ModalLoopTickEvent;
	FOnFocusChanging FocusChangingDelegate;
	FCriticalSection SlateTickCriticalSection;
	int32 ProcessingInput;
	bool bSynthesizedCursorMove = false;
#if UE_VERSION_AT_LEAST(5, 4, 0)
	bool bIsTicking = false;
#endif
#if UE_VERSION_AT_LEAST(5, 0, 0)
	uint64 PlatformMouseMovementEvents = 0;
#endif
	class InputPreProcessorsHelper
	{
	public:
#if UE_VERSION_AT_LEAST(5, 5, 0)
		using FProcessorTypeStorage = TArray<TSharedPtr<IInputProcessor>>;
		using FInputProcessorStorage = TSparseArray<FProcessorTypeStorage, TInlineSparseArrayAllocator<(uint32)EInputPreProcessorType::Count>>;
		FInputProcessorStorage InputPreProcessors;
		TArray<TSharedPtr<IInputProcessor>> InputPreProcessorsIteratorList;
#else
		TArray<TSharedPtr<IInputProcessor>> InputPreProcessorList;
#endif
		
		bool bIsIteratingPreProcessors = false;
		TArray<TSharedPtr<IInputProcessor>> ProcessorsPendingRemoval;
#if UE_VERSION_AT_LEAST(5, 5, 0)
		TArray<FInputPreprocessorRegistration> ProcessorsPendingAddition;
#else
		TMap<TSharedPtr<IInputProcessor>, int32> ProcessorsPendingAddition;
#endif
		
	};
	InputPreProcessorsHelper InputPreProcessors;
	TSharedRef<ISlateInputManager> InputManager;
#if WITH_EDITOR
	DECLARE_EVENT_OneParam(FFXFISlateApplication, FOnApplicationPreInputKeyDownListener, const FKeyEvent&);
	FOnApplicationPreInputKeyDownListener OnApplicationPreInputKeyDownListenerEvent;
	DECLARE_EVENT_OneParam(FFXFISlateApplication, FOnApplicationMousePreInputButtonDownListener, const FPointerEvent&);
	FOnApplicationMousePreInputButtonDownListener OnApplicationMousePreInputButtonDownListenerEvent;
	DECLARE_EVENT_OneParam(FFXFISlateApplication, FOnWindowDPIScaleChanged, TSharedRef<SWindow>);
	FOnWindowDPIScaleChanged OnSignalSystemDPIChangedEvent;
	FOnWindowDPIScaleChanged OnWindowDPIScaleChangedEvent;
#endif // WITH_EDITOR
};
static_assert(sizeof(FSlateApplication) == sizeof(FFXFISlateApplication), "FFXFISlateApplication must match the layout of FSlateApplication so we can access the time detla!");

#else
#error "Implement support for this engine version!"
#endif