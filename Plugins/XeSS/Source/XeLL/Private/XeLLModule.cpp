/*******************************************************************************
 * Copyright 2024 Intel Corporation
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

#include "XeLLModule.h"

#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "Features/IModularFeatures.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xell/xell.h"
#include "xell/xell_d3d12.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "XeSSCommonUtil.h"
#include "XeSSUnrealD3D12RHI.h"
#include "XeSSUnrealD3D12RHIIncludes.h"
#include "XeSSUnrealRHI.h"

static xell_context_handle_t XeLLContext = nullptr;
#endif

DEFINE_LOG_CATEGORY_STATIC(LogXeLLModule, Log, All);

DEFINE_LOG_CATEGORY_STATIC(LogXeLLSDK, Log, All);

static TAutoConsoleVariable<FString> CVarXeLLVersion(
	TEXT("r.XeLL.Version"),
	TEXT("Unknown"),
	TEXT("Show XeLL SDK's version."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<bool> CVarXeLLSupported(
	TEXT("r.XeLL.Supported"),
	false,
	TEXT("If XeLL is supported."),
	ECVF_ReadOnly);

#if XESS_ENGINE_WITH_XEFG_API
static TAutoConsoleVariable<int32> CVarXeLLEnabled(
	TEXT("r.XeLL.Enabled"),
	0,
	TEXT("0: off, 1: latency reduce enable"),
	ECVF_Default | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarXeLLLogLevel(
	TEXT("r.XeLL.LogLevel"), 1,
	TEXT("[default: 1] Minimum log level of XeLL SDK, set it via command line -XeLLLogLevel=")
	TEXT(" 0: debug,")
	TEXT(" 1: info,")
	TEXT(" 2: warning,")
	TEXT(" 3: error,")
	TEXT(" 4: off."),
	ECVF_ReadOnly
);

static void XeLLLogCallback(const char* Message, xell_logging_level_t InLogLevel)
{
	switch (InLogLevel)
	{
	case XELL_LOGGING_LEVEL_DEBUG:
		UE_LOG(LogXeLLSDK, Verbose, TEXT("%s"), ANSI_TO_TCHAR(Message));
		break;
	case XELL_LOGGING_LEVEL_INFO:
		UE_LOG(LogXeLLSDK, Log, TEXT("%s"), ANSI_TO_TCHAR(Message));
		break;
	case XELL_LOGGING_LEVEL_ERROR:
		UE_LOG(LogXeLLSDK, Error, TEXT("%s"), ANSI_TO_TCHAR(Message));
		break;
	case XELL_LOGGING_LEVEL_WARNING:
	default:
		UE_LOG(LogXeLLSDK, Warning, TEXT("%s"), ANSI_TO_TCHAR(Message));
		break;
	}
}
#endif

void FXeLLModule::StartupModule()
{
	// Do not init module if XeLL is explicitly disabled
	if (FParse::Param(FCommandLine::Get(), TEXT("XeLLDisabled")))
	{
		UE_LOG(LogXeLLModule, Log, TEXT("XeLL disabled by command line option"));
		return;
	}

	int32 XeLLLogLevel = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("XeLLLogLevel="), XeLLLogLevel))
	{
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeLL.LogLevel"))->Set(XeLLLogLevel, ECVF_SetByCommandline);
		UE_LOG(LogXeLLModule, Log, TEXT("XeLL SDK log level set by command line option, value: %d"), XeLLLogLevel);
	}

#if XESS_ENGINE_WITH_XEFG_API
	xell_version_t XeLLVersion;
	XeSSCommon::PushThirdPartyDllDirectory();
	xell_result_t VersionResult = xellGetVersion(&XeLLVersion);
	XeSSCommon::PopThirdPartyDllDirectory();
	if (XELL_RESULT_SUCCESS != VersionResult)
	{
		UE_LOG(LogXeLLModule, Error, TEXT("Failed to get XeLL SDK version, result: %d"), VersionResult);
		return;
	}
	XeSSCommon::SetSDKVersionConsoleVariable(CVarXeLLVersion->AsVariable(), TEXT("XeLL"), &XeLLVersion, sizeof(XeLLVersion));
	UE_LOG(LogXeLLModule, Log, TEXT("Loaded XeLL SDK %d.%d.%d on %s RHI %s"),
		XeLLVersion.major, XeLLVersion.minor, XeLLVersion.patch, RHIVendorIdToString(), GDynamicRHI->GetName());

	XeLLContext = nullptr;
	if (XeSSUnreal::IsRHIInterfaceD3D12())
	{
		XeSSUnreal::XD3D12DynamicRHI* D3D12RHI = static_cast<XeSSUnreal::XD3D12DynamicRHI*>(GDynamicRHI);
		ID3D12Device* Direct3DDevice = XeSSUnreal::GetDevice(D3D12RHI);

		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("XeSS");
		SetDllDirectory(*FPaths::Combine(Plugin->GetBaseDir(), TEXT("/Binaries/ThirdParty/Win64")));

		xell_result_t ret = xellD3D12CreateContext(Direct3DDevice, /*D3D12RHI->RHIGetCommandQueue(),*/ &XeLLContext);
		if (ret != XELL_RESULT_SUCCESS)
		{
			XeLLContext = nullptr;
			UE_LOG(LogXeLLModule, Warning, TEXT("failed to create XeLL, ret = %d!"), ret);
			return;
		}

		UE_LOG(LogXeLLModule, Log, TEXT("successful to create XeLL!"));

		xell_result_t Result = xellSetLoggingCallback(XeLLContext, static_cast<xell_logging_level_t>(CVarXeLLLogLevel->GetInt()), XeLLLogCallback);
		if (XELL_RESULT_SUCCESS != Result)
		{
			UE_LOG(LogXeLLModule, Warning, TEXT("Failed to set XeLL log callback, result: %d"), Result);
		}
		CVarXeLLSupported->Set(true);

		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FXeLLModule::DelayInit);
	}
#endif
}

void FXeLLModule::ShutdownModule()
{
#if XESS_ENGINE_WITH_XEFG_API
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	if (XeLLContext)
	{
		xellDestroyContext(XeLLContext);
		XeLLContext = nullptr;
	}

	if (XeLLMaxTickRateHandler.IsValid())
	{
		IModularFeatures::Get().UnregisterModularFeature(XeLLMaxTickRateHandler->GetModularFeatureName(), XeLLMaxTickRateHandler.Get());
	}

	if (XeLLLatencyMarker.IsValid())
	{
		IModularFeatures::Get().UnregisterModularFeature(XeLLLatencyMarker->GetModularFeatureName(), XeLLLatencyMarker.Get());
	}
#endif
}

void FXeLLModule::DelayInit()
{
#if XESS_ENGINE_WITH_XEFG_API
	check(CVarXeLLSupported->GetBool());

	XeLLMaxTickRateHandler = MakeUnique<FXeLLMaxTickRateHandler>(XeLLContext);
	XeLLMaxTickRateHandler->Initialize();
	IModularFeatures::Get().RegisterModularFeature(XeLLMaxTickRateHandler->GetModularFeatureName(), XeLLMaxTickRateHandler.Get());

	XeLLLatencyMarker = MakeUnique<FXeLLLatencyMarkers>(XeLLContext);
	XeLLLatencyMarker->Initialize();
	IModularFeatures::Get().RegisterModularFeature(XeLLLatencyMarker->GetModularFeatureName(), XeLLLatencyMarker.Get());
#endif
}

bool FXeLLModule::Tick(float DeltaTime)
{
#if XESS_ENGINE_WITH_XEFG_API
	static const auto CVarXeFGMode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.Enabled"));
	int32 newXeLLMode = CVarXeLLEnabled.GetValueOnGameThread();
	if (newXeLLMode != XeLLMode)
	{
		if (newXeLLMode != 0 && newXeLLMode != 1)
		{
			CVarXeLLEnabled->SetWithCurrentPriority(XeLLMode);
			UE_LOG(LogXeLLModule, Warning, TEXT("Not supported r.XeLL.Enabled value:%d"), newXeLLMode);
			return true;
		}

		if (IsXeLLSupported())
		{
			if (CVarXeFGMode && CVarXeFGMode->GetInt() && newXeLLMode == 0)
			{
				// If XeFG is ON, can't set XeLL to OFF
				CVarXeLLEnabled->SetWithCurrentPriority(XeLLMode);
				UE_LOG(LogXeLLModule, Warning, TEXT("Can't close XeLL as XeFG is on."));
				return true;
			}
			SetXeLLMode(newXeLLMode);
			XeLLMode = newXeLLMode;
		}
		else
		{
			if (newXeLLMode != 0)
			{
				UE_LOG(LogXeLLModule, Warning, TEXT("XeLL is not supported on the platform."));
			}
			XeLLMode = 0;
			CVarXeLLEnabled->SetWithCurrentPriority(0);
		}
	}
#endif
	return true;
}

xell_context_handle_t FXeLLModule::GetXeLLContext()
{
#if XESS_ENGINE_WITH_XEFG_API
	return XeLLContext;
#else
	return nullptr;
#endif
}

XELLCORE_API bool FXeLLModule::OnPreSetXeFGEnabled(bool bEnabled)
{
#if XESS_ENGINE_WITH_XEFG_API
	check(IsInGameThread());  // Console variables can only be set in game thread
	check(CVarXeLLSupported->GetBool());  // TODO(sunzhuoshi): refactor IsXeLLSupported()?

	if (!bEnabled)
		return true;

	// TODO(sunzhuoshi): move following code to XeFG SDK
	{
		xell_sleep_params_t Params{};
		xell_result_t Result = xellGetSleepMode(XeLLContext, &Params);
		if (XELL_RESULT_SUCCESS != Result)
		{
			UE_LOG(LogXeLLModule, Warning, TEXT("Failed to get XeLL sleep mode, result: %d"), Result);
			return false;
		}
		if (Params.bLowLatencyMode != 1)
		{
			Params.bLowLatencyMode = 1;
			Result = xellSetSleepMode(XeLLContext, &Params);
			if (XELL_RESULT_SUCCESS != Result)
			{
				UE_LOG(LogXeLLModule, Warning, TEXT("Failed to set XeLL sleep mode, result: %d"), Result);
				return false;
			}
		}
	}
	if (0 == CVarXeLLEnabled->GetInt())
	{
		constexpr int32 XELL_ENABLED_BY_XEFG = 1;
		CVarXeLLEnabled->SetWithCurrentPriority(XELL_ENABLED_BY_XEFG);
		UE_LOG(LogXeLLModule, Log, TEXT("r.XeLL.Enabled set by XeFG, value: %d"), XELL_ENABLED_BY_XEFG);
	}
	return true;
#else
	(void)bEnabled;
	return false;
#endif
}

XELLCORE_API bool FXeLLModule::IsXeLLSupported()
{
#if XESS_ENGINE_WITH_XEFG_API
	bool bIsMaxTickRateHandlerEnabled = false;

	TArray<IMaxTickRateHandlerModule*> MaxTickRateHandlerModules = IModularFeatures::Get()
		.GetModularFeatureImplementations<IMaxTickRateHandlerModule>(IMaxTickRateHandlerModule::GetModularFeatureName());
	for (IMaxTickRateHandlerModule* MaxTickRateHandler : MaxTickRateHandlerModules)
	{
		// check if the max tick rate handler available
		if (MaxTickRateHandler && MaxTickRateHandler == GetMaxTickRateHandler())
		{
			bIsMaxTickRateHandlerEnabled = MaxTickRateHandler->GetAvailable();
			break;
		}
	}

	bool bIsLatencyMarkerModuleEnabled = false;

	TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get()
		.GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
	for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
	{
		// check if the latency marker available
		if (LatencyMarkerModule && LatencyMarkerModule == GetLatencyMarker())
		{
			bIsLatencyMarkerModuleEnabled = LatencyMarkerModule->GetAvailable();
			break;
		}
	}

	return bIsMaxTickRateHandlerEnabled && bIsLatencyMarkerModuleEnabled;
#else
	return false;
#endif
}

XELLCORE_API void FXeLLModule::SetXeLLMode(const uint32 flags)
{
#if XESS_ENGINE_WITH_XEFG_API
	if (flags != 0 && flags != 1)
	{
		UE_LOG(LogXeLLModule, Warning, TEXT("Please set the right value for XeLL mode, 0: off, 1 : latency reduce enable"));
		return;
	}

	// clear reflex settings (especially for ReflexStatsLatencyMarker) when enable XeLL
	if (flags != 0)
	{
		// clear reflex settings
		TArray<IMaxTickRateHandlerModule*> MaxTickRateHandlerModules = IModularFeatures::Get()
			.GetModularFeatureImplementations<IMaxTickRateHandlerModule>(IMaxTickRateHandlerModule::GetModularFeatureName());
		for (IMaxTickRateHandlerModule* MaxTickRateHandler : MaxTickRateHandlerModules)
		{
			MaxTickRateHandler->SetEnabled(false);
			MaxTickRateHandler->SetFlags(0);
		}

		TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get()
			.GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
		for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
		{
			LatencyMarkerModule->SetEnabled(false);
		}
	}

	// set XeLL mode
	if (GetMaxTickRateHandler())
	{
		GetMaxTickRateHandler()->SetEnabled(flags != 0);
		GetMaxTickRateHandler()->SetFlags(flags);
	}
	if (GetLatencyMarker())
	{
		GetLatencyMarker()->SetEnabled(true);
	}
#endif
}

IMPLEMENT_MODULE(FXeLLModule, XeLLCore)
