/*******************************************************************************
 * Copyright 2023 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "XeFGRHI.h"

#include "HAL/IConsoleManager.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Features/IModularFeatures.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "SystemTextures.h"
#include "UnrealClient.h"
#include "XeFGDXGISwapChainProvider.h"
#include "XeFGRHIUtil.h"
#include "XeLLDelegates.h"
#include "XeLLModule.h"
#include "XeSSCommonUtil.h"
#include "XeSSUnrealD3D12RHI.h"
#include "XeSSUnrealD3D12RHIIncludes.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xess_fg/xefg_swapchain_d3d12.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

static TAutoConsoleVariable<bool> CVarXeFGSupported(
	TEXT("r.XeFG.Supported"),
	false,
	TEXT("If XeFG is supported."),
	ECVF_ReadOnly);

#if XESS_ENGINE_WITH_XEFG_API
DEFINE_LOG_CATEGORY_STATIC(LogXeFGRHI, Log, All);

// Log category for XeFG SDK
DEFINE_LOG_CATEGORY_STATIC(LogXeFGSDK, Log, All);

DECLARE_STATS_GROUP(TEXT("XeFG"), STATGROUP_XeFG, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT(TEXT("Frames Presented"), STAT_XeFGFramesPresented, STATGROUP_XeFG);
DECLARE_FLOAT_COUNTER_STAT(TEXT("Average FPS"), STAT_XeFGAverageFPS, STATGROUP_XeFG);

extern ENGINE_API float GAverageFPS;
XEFGRHI_API float GXeFGAverageFPS = 0.0f;

static const IID IID_XeFGDSwapChainIID = { 0x157E1000, 0x16F8, 0x16F8, {0x16, 0xF8, 0x15, 0x7E, 0x94, 0x01, 0xA7, 0x10} };

static TAutoConsoleVariable<int32> CVarXeFGEnabled(
	TEXT("r.XeFG.Enabled"),
	0,
	TEXT("[default: 0] Set to 1 to enable XeFG."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarXeFGMotionVectorScale(
	TEXT("r.XeFG.MotionVectorScale"),
	1.0f,
	TEXT("[default: 1.0f] Scale applied to motion vector."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarXeFGUIMode(
	TEXT("r.XeFG.UIMode"),
	5,
	TEXT("[default: 5] UI mode.")
	TEXT(" 0: auto based on provided inputs,")
	TEXT(" 1: without any UI handling,")
	TEXT(" 2: refine UI using UI texture + alpha,")
	TEXT(" 3: blend UI from UI texture + alpha,")
	TEXT(" 4: extract UI from backbuffer,")
	TEXT(" 5: blend UI from UI texture + alpha and refine it by extracting from backbuffer."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarXeFGLogLevel(
	TEXT("r.XeFG.LogLevel"),
	1,
	TEXT("[default: 1] Minimum log level of XeFG SDK, set it via command line -XeFGLogLevel=")
	TEXT(" 0: debug,")
	TEXT(" 1: info,")
	TEXT(" 2: warning,")
	TEXT(" 3: error,")
	TEXT(" 4: off."),
	ECVF_ReadOnly
);

static TAutoConsoleVariable<int32> CVarXeFGResourceValidity(
	TEXT("r.XeFG.ResourceValidity"),
	1,
	TEXT("[default: 1] Validity of input resources of XeFG, set it via command line -XeFGResourceValidity=")
	TEXT(" 0: XEFG_SWAPCHAIN_RV_UNTIL_NEXT_PRESENT,")
	TEXT(" 1: XEFG_SWAPCHAIN_RV_ONLY_NOW."),
	ECVF_ReadOnly
);

static TAutoConsoleVariable<bool> CVarXeFGOverrideSwapChain(
	TEXT("r.XeFG.OverrideSwapChain"),
	true,
	TEXT("[default: true] If override swap chain, it should help prevent conflicts with other similar plugins."),
	ECVF_ReadOnly);

static void XeFGLogCallback(const char* Message, xefg_swapchain_logging_level_t InLogLevel, void*)
{
	switch (InLogLevel)
	{
	case XEFG_SWAPCHAIN_LOGGING_LEVEL_DEBUG:
		UE_LOG(LogXeFGSDK, Verbose, TEXT("%s"), ANSI_TO_TCHAR(Message));
		break;
	case XEFG_SWAPCHAIN_LOGGING_LEVEL_INFO:
		UE_LOG(LogXeFGSDK, Log, TEXT("%s"), ANSI_TO_TCHAR(Message));
		break;
	case XEFG_SWAPCHAIN_LOGGING_LEVEL_ERROR:
		UE_LOG(LogXeFGSDK, Error, TEXT("%s"), ANSI_TO_TCHAR(Message));
		break;
	case XEFG_SWAPCHAIN_LOGGING_LEVEL_WARNING:
	default:
		UE_LOG(LogXeFGSDK, Warning, TEXT("%s"), ANSI_TO_TCHAR(Message));
		break;
	}
}

// NOTE: use it only to transition state back and forth, limited by implementation.
static void TransitionResource(XeSSUnreal::XD3D12DynamicRHI* D3D12DynamicRHI, FRHITexture* Texture, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter, uint32 SubResource, FRHICommandListBase& ExecutingCmdList)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	// Use DirectX API directly instead of D3D12DynamicRHI->RHITransitionResource to make it simple and synchronized
	ID3D12GraphicsCommandList* CommandList = XeSSUnreal::RHIGetGraphicsCommandList(D3D12DynamicRHI, ExecutingCmdList);
	ID3D12Resource* Resource = static_cast<ID3D12Resource*>(Texture->GetNativeResource());
	CD3DX12_RESOURCE_BARRIER Barriers[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(Resource, StateBefore, StateAfter, SubResource)
	};
	CommandList->ResourceBarrier(1, Barriers);
#else
	(void)StateBefore;
	const FD3D12TextureBase* D3D12Texture = GetD3D12TextureFromRHITexture(Texture);
	FD3D12Device* D3D12Device = D3D12Texture->GetParentDevice();
	#if XESS_ENGINE_VERSION_EQU(5, 0)
	D3D12DynamicRHI->TransitionResource(D3D12Device->GetDefaultCommandContext().CommandListHandle, D3D12Texture->GetResource(), StateBefore, StateAfter, SubResource, FD3D12DynamicRHI::ETransitionMode::Apply);
	#else
	D3D12DynamicRHI->TransitionResource(D3D12Device->GetDefaultCommandContext().CommandListHandle, D3D12Texture->GetResource(), StateAfter, SubResource);
	#endif
#endif
}

FXeFGRHI::FXeFGRHI(XeSSUnreal::XD3D12DynamicRHI* InD3D12RHI, FXeLLModule& InXeLLModule):
	D3D12RHI(InD3D12RHI),
	XeFGSwapChainInitFlags(XEFG_SWAPCHAIN_INIT_FLAG_INVERTED_DEPTH),
	XeLLModule(InXeLLModule)
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		return;
	}
#endif
	XeSSCommon::PushThirdPartyDllDirectory();
	xefg_swapchain_result_t Result = xefgSwapChainD3D12CreateContext(XeSSUnreal::GetDevice(InD3D12RHI), &XeFGSwapChainContext);
	XeSSCommon::PopThirdPartyDllDirectory();
	UE_LOG(LogXeFGRHI, Log, TEXT("Create XeFG swap chain context, result: %d"), Result);
	if (XEFG_SWAPCHAIN_RESULT_SUCCESS == Result)
	{
		// Note: context created should be treated as supported, later failures should be treated as internal errors
		bIsXeFGSupported = true;
		CVarXeFGSupported->Set(true);
		xefg_swapchain_result_t LoggerResult = xefgSwapChainSetLoggingCallback(XeFGSwapChainContext, static_cast<xefg_swapchain_logging_level_t>(CVarXeFGLogLevel->GetInt()), XeFGLogCallback, nullptr);
		if (XEFG_SWAPCHAIN_RESULT_SUCCESS != LoggerResult)
		{
			UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to set XeFG log callback, result: %d"), LoggerResult);
		}

		void* XeLLContext = XeLLModule.GetXeLLContext();
		if (XeLLContext)
		{
			xefg_swapchain_result_t SetLatencyReductionResult = xefgSwapChainSetLatencyReduction(XeFGSwapChainContext, XeLLContext);
			UE_LOG(LogXeFGRHI, Log, TEXT("Set latency reduction context result: %d"), SetLatencyReductionResult);
			if (XEFG_SWAPCHAIN_RESULT_SUCCESS == SetLatencyReductionResult)
			{
				// Handle value set by ini file
				HandleXeFGEnabledSet(CVarXeFGEnabled->AsVariable());
				CVarXeFGEnabled->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FXeFGRHI::HandleXeFGEnabledSet));

				if (CVarXeFGOverrideSwapChain->GetBool())
				{
					CustomSwapChainProvider = MakeUnique<FXeFGDXGISwapChainProvider>(this);
					IModularFeatures::Get().RegisterModularFeature(IDXGISwapchainProvider::GetModularFeatureName(), CustomSwapChainProvider.Get());
					FXeLLDelegates::OnPresentLatencyMarkerEnd.BindRaw(this, &FXeFGRHI::OnXeLLPresentLatencyMarkerEnd);
				}
			}
		}
		else
		{
			UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to get XeLL context used by XeFG, probably not supported."));
		}
	}
}

FXeFGRHI::~FXeFGRHI()
{
	if (FXeLLDelegates::OnPresentLatencyMarkerEnd.IsBound())
	{
		FXeLLDelegates::OnPresentLatencyMarkerEnd.Unbind();
	}
	if (CustomSwapChainProvider.IsValid())
	{
		IModularFeatures::Get().UnregisterModularFeature(IDXGISwapchainProvider::GetModularFeatureName(), CustomSwapChainProvider.Get());
		CustomSwapChainProvider.Reset();
	}
	if (XeFGSwapChainContext)
	{
		xefgSwapChainDestroy(XeFGSwapChainContext);
	}
}

bool FXeFGRHI::IsXeFGEnabled() const
{
	return IsXeFGSupported() && CVarXeFGEnabled->GetBool();
}

HRESULT FXeFGRHI::CreateInterpolationSwapChain(HWND WindowsHandle, const DXGI_SWAP_CHAIN_DESC1* SwapChainDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* FullscreenDesc, IDXGIFactory2* Factory, const IID& SwapChainInterfaceID, IDXGISwapChain** OutSwapChain)
{
	check(XeFGSwapChainContext);

	xefg_swapchain_properties_t Properties = {};
	xefg_swapchain_d3d12_init_params_t InitParams{};
	xefg_swapchain_result_t Result = xefgSwapChainGetProperties(XeFGSwapChainContext, &Properties);

	if (XEFG_SWAPCHAIN_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeFGRHI, Error, TEXT("Failed to get XeFG swap chain properties, result: %d"), Result);
		return E_FAIL;
	}
	InitParams.uiMode = static_cast<xefg_swapchain_ui_mode_t>(CVarXeFGUIMode->GetInt());
	InitParams.pApplicationSwapChain = nullptr;
	InitParams.maxInterpolatedFrames = Properties.maxSupportedInterpolations;
	InitParams.initFlags = XeFGSwapChainInitFlags;
	Result = xefgSwapChainD3D12InitFromSwapChainDesc(XeFGSwapChainContext, WindowsHandle, SwapChainDesc, FullscreenDesc, XeSSUnreal::RHIGetCommandQueue(D3D12RHI), Factory, &InitParams);
	if (XEFG_SWAPCHAIN_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeFGRHI, Error, TEXT("Failed to create XeFG swap chain, result: %d"), Result);
		return E_FAIL;
	}
	IDXGISwapChain* TempSwapChain = nullptr;
	Result = xefgSwapChainD3D12GetSwapChainPtr(XeFGSwapChainContext, SwapChainInterfaceID, reinterpret_cast<void**>(&TempSwapChain));
	if (XEFG_SWAPCHAIN_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeFGRHI, Error, TEXT("Failed to get XeFG swap chain pointer, result: %d"), Result);
		return E_FAIL;
	}
	*OutSwapChain = TempSwapChain;
	return S_OK;
}

void FXeFGRHI::TagUI(uint32 FrameID, FRHITexture* Texture, const FIntRect& TextureRect, FRHICommandListBase& ExecutingCmdList)
{
	check(bIsXeFGSupported);
	if (!CVarXeFGEnabled->GetBool())
	{
		UE_LOG(LogXeFGRHI, Log, TEXT("Skip tagging XeFG UI due to XeFG is disabled, frame ID: %d"), FrameID);
		return;
	}
	// TODO(sunzhuoshi): move checking to FXeFGUIHandler::OnBackBufferReadyToPresent() to avoid unnecessary UI extraction pass
	// NOTE: render thread should be used to check, not current RHI thread
	if (FrameID != TargetFrameID || bTargetFrameHandled)
	{
		UE_LOG(LogXeFGRHI, Log, TEXT("Skip tagging UI with frame ID: %d, target frame ID: %d, target frame handled: %d"),
			FrameID, TargetFrameID, bTargetFrameHandled);
		// UI frame ID mis-match with target frame ID, disable XeFG temporarily,
		// until FXeFGViewExtension::PostProcessPassAfterVisualizeDepthOfField_RenderThread is called.
		if (bIsXeFGEnabledWithAPI)
		{
			UE_LOG(LogXeFGRHI, Log, TEXT("Frame data mis-match detected, disable XeFG temporarily"));
			SetXeFGEnabledWithTemporary(false, true);
		}
		return;
	}
	TagTexture(XEFG_SWAPCHAIN_RES_UI, FrameID, Texture, TextureRect, ExecutingCmdList);
	bTargetFrameHandled = true;
}

void FXeFGRHI::TagConstants(const FXeFGRHIArguments& Arguments)
{
	check(bIsXeFGSupported);

	xefg_swapchain_frame_constant_data_t Constants = {};
	float MotionVectorScale = CVarXeFGMotionVectorScale->GetFloat();
	Constants.motionVectorScaleX = MotionVectorScale;
	Constants.motionVectorScaleY = MotionVectorScale;
	Constants.jitterOffsetX = Arguments.JitterOffset.X;
	Constants.jitterOffsetY = Arguments.JitterOffset.Y;
	XeFGRHIUtil::Assign(Arguments.ViewMatrix, Constants.viewMatrix);
	XeFGRHIUtil::Assign(Arguments.ProjectionMatrix, Constants.projectionMatrix);
	Constants.resetHistory = Arguments.bCameraCut;
	xefg_swapchain_result_t Result = xefgSwapChainTagFrameConstants(XeFGSwapChainContext, Arguments.FrameID, &Constants);
	if (XEFG_SWAPCHAIN_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to tag XeFG swap chain frame constants, result: %d, frame ID: %d"), Result, Arguments.FrameID);
	}
}

void FXeFGRHI::TagTexture(xefg_swapchain_resource_type_t Type, uint32 FrameID, FRHITexture* Texture, const FIntRect& TextureRect, FRHICommandListBase& ExecutingCmdList)
{
	check(bIsXeFGSupported);

	ID3D12Resource* Resource = static_cast<ID3D12Resource*>(Texture->GetNativeResource());
	check(Resource);
	auto CommandList = XeSSUnreal::RHIGetGraphicsCommandList(D3D12RHI, ExecutingCmdList);
	xefg_swapchain_d3d12_resource_data_t ResData{};
	D3D12_RESOURCE_DESC Desc = Resource->GetDesc();
	D3D12_RESOURCE_STATES ResourceState = D3D12_RESOURCE_STATE_COMMON;

	switch (Type)
	{
	case XEFG_SWAPCHAIN_RES_HUDLESS_COLOR:
		ResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
		break;
	case XEFG_SWAPCHAIN_RES_UI:
		ResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		break;
	case XEFG_SWAPCHAIN_RES_DEPTH:
#if XESS_ENGINE_VERSION_EQU(5, 2)
		ResourceState = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
#else
		ResourceState = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
#endif
		break;
	case XEFG_SWAPCHAIN_RES_MOTION_VECTOR:
#if XESS_ENGINE_VERSION_GEQ(5, 2)
		if (Texture == GSystemTextures.BlackDummy->GetRHI())
		{
			ResourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		}
		else
		{
			ResourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}
#else
		ResourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
#endif
		break;
	default:
		checkf(false, TEXT("Resource type not supported yet."));
		break;
	}
	ResData.type = Type;
	ResData.validity = static_cast<xefg_swapchain_resource_validity_t>(CVarXeFGResourceValidity->GetInt());
	ResData.resourceBase.x = TextureRect.Min.X;
	ResData.resourceBase.y = TextureRect.Min.Y;
	ResData.resourceSize.x = TextureRect.Width();
	ResData.resourceSize.y = TextureRect.Height();
	ResData.pResource = Resource;
	ResData.incomingState = ResourceState;
	/*
	* DepthStencil texture uses planar resources with subresource index and subresources are in different states here:
	*   0: depth plane, state: D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	*   1: stencil plane, state: D3D12_RESOURCE_STATE_DEPTH_WRITE
	* Transition stencil plane to the same state as depth plane before calling and revert after calling.
	* Reference: https://github.com/microsoft/DirectX-Specs/blob/master/d3d/PlanarDepthStencilDDISpec.md
	*/
	if (XEFG_SWAPCHAIN_RES_DEPTH == Type)
	{
		TransitionResource(D3D12RHI, Texture, D3D12_RESOURCE_STATE_DEPTH_WRITE, ResourceState, 1, ExecutingCmdList);
	}
	xefg_swapchain_result_t Result = xefgSwapChainD3D12TagFrameResource(XeFGSwapChainContext, CommandList, FrameID, &ResData);
	if (XEFG_SWAPCHAIN_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to tag XeFG swap chain frame resource, result: %d, frame ID: %d"), Result, FrameID);
	}
	if (XEFG_SWAPCHAIN_RES_DEPTH == Type)
	{
		TransitionResource(D3D12RHI, Texture, ResourceState, D3D12_RESOURCE_STATE_DEPTH_WRITE, 1, ExecutingCmdList);
	}
}

bool FXeFGRHI::IsXeFGSwapChain(IDXGISwapChain* SwapChain)
{
	check(SwapChain);

	TRefCountPtr<IUnknown> XeFGSwapChain;
	return SUCCEEDED(SwapChain->QueryInterface(IID_XeFGDSwapChainIID, reinterpret_cast<void**>(XeFGSwapChain.GetInitReference())));
}

bool FXeFGRHI::IfGameViewportUsesXeFGSwapChain()
{
	bool Result = false;

	FRHIViewport* Viewport = GetRHIGameViewport();
	if (Viewport)
	{
		return IsXeFGSwapChain(reinterpret_cast<IDXGISwapChain*>(Viewport->GetNativeSwapChain()));
	}
	return false;
}

void FXeFGRHI::TagForPresent(const FXeFGRHIArguments& Arguments, FRHICommandListBase& ExecutingCmdList)
{
	check(bIsXeFGSupported);

	if (!CVarXeFGEnabled->GetBool())
	{
		UE_LOG(LogXeFGRHI, Log, TEXT("Skip tagging XeFG resources for present due to XeFG is disabled, frame ID: %d"), Arguments.FrameID);
		return;
	}
	TagConstants(Arguments);
	TagTexture(XEFG_SWAPCHAIN_RES_HUDLESS_COLOR, Arguments.FrameID, Arguments.HUDLessColorTexture, Arguments.HUDLessColorRect, ExecutingCmdList);
	TagTexture(XEFG_SWAPCHAIN_RES_MOTION_VECTOR, Arguments.FrameID, Arguments.VelocityTexture, Arguments.VelocityRect, ExecutingCmdList);
	TagTexture(XEFG_SWAPCHAIN_RES_DEPTH, Arguments.FrameID, Arguments.DepthTexture, Arguments.DepthRect, ExecutingCmdList);
	xefg_swapchain_result_t Result = xefgSwapChainSetPresentId(XeFGSwapChainContext, Arguments.FrameID);
	if (XEFG_SWAPCHAIN_RESULT_SUCCESS == Result)
	{
		// Only re-enable XeFG if it is still enabled (it may be disabled by API since internal temporary disabling)
		if (!bIsXeFGEnabledWithAPI && CVarXeFGEnabled->GetBool())
		{
			UE_LOG(LogXeFGRHI, Log, TEXT("Re-enable XeFG from disabled temporarily"));
			SetXeFGEnabledWithTemporary(true, true);
		}
	}
	else
	{
		UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to set XeFG swap chain present ID, result: %d, frame ID: %d"), Result, Arguments.FrameID);
	}
	TargetFrameID = Arguments.FrameID;
	bTargetFrameHandled = false;
}

void FXeFGRHI::SetXeFGEnabledWithTemporary(bool bEnabled, bool bTemporary)
{
	check(bIsXeFGSupported);
	check(IsInGameThread() || bTemporary)  // Either called in game thread or in other thread temporarily

	UE_LOG(LogXeFGRHI, Log, TEXT("Set XeFG enabled, enabled: %d, Is XeFG enabled with API: %d"), bEnabled, bIsXeFGEnabledWithAPI);
	if (bIsXeFGEnabledWithAPI != bEnabled)
	{
		// Don't notify XeLL if XeFG enabled set temporarily
		if (!bTemporary)
		{
			if (!XeLLModule.OnPreSetXeFGEnabled(bEnabled))
			{
				UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to set XeLL before enabling/disabling XeFG"));
				return;
			}
		}
		xefg_swapchain_result_t Result = xefgSwapChainSetEnabled(XeFGSwapChainContext, bEnabled);
		if (XEFG_SWAPCHAIN_RESULT_SUCCESS != Result)
		{
			UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to set XeFG swap chain enabled, result: %d, enabled: %d"), Result, bEnabled);
			return;
		}
		bIsXeFGEnabledWithAPI = bEnabled;
	}
}

FRHIViewport* FXeFGRHI::GetRHIGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		FViewport* Viewport = GEngine->GameViewport->Viewport;
		if (Viewport)
		{
			return static_cast<FRHIViewport*>(Viewport->GetViewportRHI().GetReference());
		}
	}
	return nullptr;
}
void FXeFGRHI::HandleXeFGEnabledSet(IConsoleVariable* Variable)
{
	if (!IsXeFGSupported())
	{
		UE_LOG(LogXeFGRHI, Display, TEXT("XeFG not supported."));
		return;
	}
	bool bEnabled = Variable->GetBool();
	UE_LOG(LogXeFGRHI, Log, TEXT("r.XeFG.Enabled set, value: %d"), bEnabled);
	SetXeFGEnabledWithTemporary(bEnabled, false);
}
void FXeFGRHI::OnXeLLPresentLatencyMarkerEnd(uint64 FrameNumber)
{
	(void)FrameNumber;
	check(IsInRHIThread());  // It should be call in RHI thread
	xefg_swapchain_present_status_t PresentStatus;
	xefg_swapchain_result_t Result = xefgSwapChainGetLastPresentStatus(XeFGSwapChainContext, &PresentStatus);

	switch (Result)
	{
	case XEFG_SWAPCHAIN_RESULT_SUCCESS:
		{
			uint32_t FramesPresented = PresentStatus.framesPresented;
			// Temp workaround for XeFG SDK 1.0
			// TODO(sunzhuoshi): fix it in XeFG SDK
			if (FramesPresented == 0)
			{
				FramesPresented = 1;
			}
#if STATS
			SET_DWORD_STAT(STAT_XeFGFramesPresented, FramesPresented);
			SET_FLOAT_STAT(STAT_XeFGAverageFPS, GAverageFPS * FramesPresented);
#endif
			GXeFGAverageFPS = GAverageFPS * FramesPresented;  // Data may not be accurate without synchronization, but it should be acceptable to make code simple
		}
		break;
	default:
		check(false);
		GXeFGAverageFPS = GAverageFPS;  // Data may not be accurate without synchronization, but it should be acceptable to make code simple
		break;
	}
}
#endif
