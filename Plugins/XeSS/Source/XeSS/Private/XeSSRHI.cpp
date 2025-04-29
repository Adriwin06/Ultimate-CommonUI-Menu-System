/*******************************************************************************
 * Copyright 2021 Intel Corporation
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

#include "XeSSRHI.h"

#include "XeSSSDKLoader.h"
#include "XeSSSDKUtil.h"
#include "XeSSSDKWrapperBase.h"
#include "XeSSUnrealRHI.h"
#include "XeSSUtil.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSRHI, Log, All);

// Log category for XeSS SDK
DEFINE_LOG_CATEGORY_STATIC(LogXeSSSDK, Log, All);

struct FResolutionFractionSetting
{
	float Min = 0.f;
	float Max = 0.f;
	float Optimal = 0.f;
};
static FResolutionFractionSetting ResolutionFractionSettings[XeSSUtil::XESS_QUALITY_SETTING_COUNT] = {};
static float MinResolutionFraction = 100.f;
static float MaxResolutionFraction = 0.f;

static TAutoConsoleVariable<FString> CVarXeSSVersion(
	TEXT("r.XeSS.Version"),
	TEXT("Unknown"),
	TEXT("Show XeSS SDK's version."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarXeSSFrameDumpStart(
	TEXT("r.XeSS.FrameDump.Start"),
	0,
	TEXT("DEPRECATED, please use XeSS Inspector tool instead."),
	ECVF_Default);

static TAutoConsoleVariable<FString> CVarXeSSFrameDumpMode(
	TEXT("r.XeSS.FrameDump.Mode"),
	TEXT("all"),
	TEXT("DEPRECATED, please use XeSS Inspector tool instead."),
	ECVF_Default);

static TAutoConsoleVariable<FString> CVarXeSSFrameDumpPath(
	TEXT("r.XeSS.FrameDump.Path"),
	TEXT("."),
	TEXT("DEPRECATED, please use XeSS Inspector tool instead."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarXeSSLogLevel(
	TEXT("r.XeSS.LogLevel"), 1,
	TEXT("[default: 1] Minimum log level of XeSS SDK, set it via command line -XeSSLogLevel=")
	TEXT(" 0: debug,")
	TEXT(" 1: info,")
	TEXT(" 2: warning,")
	TEXT(" 3: error,")
	TEXT(" 4: off."),
	ECVF_ReadOnly
);

static TAutoConsoleVariable<float> CVarXeSSOptimalScreenPercentage(
	TEXT("r.XeSS.OptimalScreenPercentage"),
	100.f,
	TEXT("Optimal screen percentage for current XeSS quality."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarXeSSAutoExposure(
	TEXT("r.XeSS.AutoExposure"),
	1,
	TEXT("[default: 1] Use XeSS internal auto exposure."),
	ECVF_Default | ECVF_RenderThreadSafe);

static void XeSSLogCallback(const char* Message, xess_logging_level_t InLogLevel)
{
	switch (InLogLevel)
	{
	case XESS_LOGGING_LEVEL_DEBUG:
		UE_LOG(LogXeSSSDK, Verbose, TEXT("%hs"), Message);
		break;
	case XESS_LOGGING_LEVEL_INFO:
		UE_LOG(LogXeSSSDK, Log, TEXT("%hs"), Message);
		break;
	case XESS_LOGGING_LEVEL_ERROR:
		UE_LOG(LogXeSSSDK, Error, TEXT("%hs"), Message);
		break;
	case XESS_LOGGING_LEVEL_WARNING:
	default:
		UE_LOG(LogXeSSSDK, Warning, TEXT("%hs"), Message);
		break;
	}
}

bool FXeSSRHI::EffectRecreationIsRequired(const FXeSSInitArguments& NewArgs) const {
	if (InitArgs.OutputWidth != NewArgs.OutputWidth ||
		InitArgs.OutputHeight != NewArgs.OutputHeight ||
		InitArgs.QualitySetting != NewArgs.QualitySetting ||
		InitArgs.InitFlags != NewArgs.InitFlags)
	{
		return true;
	}
	return false;
}

float FXeSSRHI::GetOptimalResolutionFraction()
{
	return GetOptimalResolutionFraction(
		XeSSUtil::ToXeSSQualitySetting(InitArgs.QualitySetting)
	);
}

float FXeSSRHI::GetMinSupportedResolutionFraction()
{
	return MinResolutionFraction;
}

float FXeSSRHI::GetMaxSupportedResolutionFraction()
{
	return MaxResolutionFraction;
}

float FXeSSRHI::GetOptimalResolutionFraction(xess_quality_settings_t InQualitySetting)
{
	check(XeSSUtil::IsValid(InQualitySetting));

	return ResolutionFractionSettings[XeSSUtil::ToIndex(InQualitySetting)].Optimal;
}

uint32 FXeSSRHI::GetXeSSInitFlags()
{
	uint32 InitFlags = XESS_INIT_FLAG_HIGH_RES_MV;
	if (CVarXeSSAutoExposure->GetBool())
	{
		InitFlags |= XESS_INIT_FLAG_ENABLE_AUTOEXPOSURE;
	}
	return InitFlags;
}

FXeSSRHI::FXeSSRHI()
{
	check(GDynamicRHI);
}

FXeSSRHI::~FXeSSRHI()
{
	if (!bXeSSInitialized)
	{
		return;
	}
	xess_result_t Result = XeSSSDKWrapper->xessDestroyContext(XeSSContext);
	if (Result == XESS_RESULT_SUCCESS)
	{
		UE_LOG(LogXeSSRHI, Log, TEXT("Removed Intel XeSS effect"));
	}
	else
	{
		UE_LOG(LogXeSSRHI, Warning, TEXT("Failed to remove XeSS effect"));
	}
	FXeSSSDKLoader::Unload();
}

bool FXeSSRHI::Initialize()
{
	check(GDynamicRHI);

	const TCHAR* RHIName = GDynamicRHI->GetName();
	XeSSSDKWrapper = FXeSSSDKLoader::Load(RHIName);
	check(XeSSSDKWrapper);

	if (!XeSSSDKWrapper->IsSupported())
	{
		UE_LOG(LogXeSSRHI, Log, TEXT("XeSS not supported with RHI: %s, platform: %hs"), RHIName, FPlatformProperties::IniPlatformName());
		return false;
	}
	if (!XeSSSDKWrapper->IsLoaded())
	{
		UE_LOG(LogXeSSRHI, Error, TEXT("Failed to load XeSS SDK, library file name: %s"), XeSSSDKWrapper->GetLibraryFileName());
		return false;
	}
	OnXeSSSDKLoaded();

	// Get XeSS SDK version
	xess_version_t XeSSVersion;
	xess_result_t Result = XeSSSDKWrapper->xessGetVersion(&XeSSVersion);
	if (XESS_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeSSRHI, Error, TEXT("Failed to get XeSS SDK version, result: %d"), Result);
		return false;
	}
	FString XeSSVersionString = XeSSSDKUtil::ToString(XeSSVersion);
	UE_LOG(LogXeSSRHI, Log, TEXT("Loading XeSS library %s on %s RHI %s"), *XeSSVersionString, RHIVendorIdToString(), RHIName);
	CVarXeSSVersion->Set(*FString::Format(TEXT("XeSS version: {0}"), { *XeSSVersionString }));

	Result = CreateXeSSContext(&XeSSContext);
	if (XESS_RESULT_SUCCESS == Result)
	{
		UE_LOG(LogXeSSRHI, Log, TEXT("Intel XeSS effect supported"));
	}
	else
	{
		UE_LOG(LogXeSSRHI, Log, TEXT("Intel XeSS effect NOT supported, result: %d"), Result);
		return false;
	}

	Result = XeSSSDKWrapper->xessSetLoggingCallback(XeSSContext, static_cast<xess_logging_level_t>(CVarXeSSLogLevel->GetInt()), XeSSLogCallback);
	// NOTE: xessSetLoggingCallback may not be implemented yet for D3D11
	if (XESS_RESULT_SUCCESS != Result && XESS_RESULT_ERROR_NOT_IMPLEMENTED != Result)
	{
		UE_LOG(LogXeSSRHI, Error, TEXT("Failed to set XeSS log callback, result: %d"), Result);
		return false;
	}

	// Print XeFX library version if it was loaded, XeFX will only be used when running on Intel platforms
	xess_version_t XeFXVersion;
	Result = XeSSSDKWrapper->xessGetIntelXeFXVersion(XeSSContext, &XeFXVersion);
	if (XESS_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeSSRHI, Error, TEXT("Failed to get Intel XeFX version, result: %d"), Result);
		return false;
	}
	FString XeFGVersionString = XeSSSDKUtil::ToString(XeFXVersion);
	// Append XeFX library info to version string when running on Intel
	if (XeSSSDKUtil::IsValid(XeFXVersion))
	{
		UE_LOG(LogXeSSRHI, Log, TEXT("Loading Intel XeFX library %s"), *XeFGVersionString);
		CVarXeSSVersion->Set(*FString::Format(TEXT("{0}, XeFG version: {1}"), { *CVarXeSSVersion->GetString(), *XeFGVersionString }));
	}

	InitResolutionFractions();

	Result = BuildXeSSPipelines(XeSSContext, GetXeSSInitFlags());
	if (XESS_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeSSRHI, Error, TEXT("Failed to build XeSS pipe lines, result: %d"), Result);
		return false;
	}

	static const auto CVarXeSSEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));
	static const auto CVarXeSSQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Quality"));

	// Handle value set by ini file
	HandleXeSSEnabledSet(CVarXeSSEnabled->AsVariable());
	// NOTE: OnChangedCallback will always be called when set even if the value is not changed
	CVarXeSSEnabled->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FXeSSRHI::HandleXeSSEnabledSet));

	// Handle value set by ini file
	HandleXeSSQualitySet(CVarXeSSQuality->AsVariable());
	CVarXeSSQuality->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FXeSSRHI::HandleXeSSQualitySet));

	bXeSSInitialized = true;
	return true;
}

void FXeSSRHI::InitResolutionFractions()
{
	for (int32 QualitySettingInt = XeSSUtil::XESS_QUALITY_SETTING_MIN; QualitySettingInt <= XeSSUtil::XESS_QUALITY_SETTING_MAX; ++QualitySettingInt)
	{
		// Use 16384(D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) to avoid potential API errors
		constexpr uint32_t TEXTURE2D_DIMENSION_MAX = 16384;
		xess_2d_t OutputResolution{ TEXTURE2D_DIMENSION_MAX, TEXTURE2D_DIMENSION_MAX };
		xess_2d_t MinInputResolution{};
		xess_2d_t MaxInputResolution{};
		xess_2d_t OptimalInputResolution{};
		xess_quality_settings_t TempQualitySetting = static_cast<xess_quality_settings_t>(QualitySettingInt);
		xess_result_t Result = XeSSSDKWrapper->xessGetOptimalInputResolution(XeSSContext, &OutputResolution, TempQualitySetting, &OptimalInputResolution, &MinInputResolution, &MaxInputResolution);
		if (XESS_RESULT_SUCCESS != Result)
		{
			UE_LOG(LogXeSSRHI, Warning, TEXT("Failed to get XeSS optimal input resolution, result: %d"), Result);
			continue;
		}
		FResolutionFractionSetting Setting;
		Setting.Optimal = static_cast<float>(OptimalInputResolution.x) / static_cast<float>(OutputResolution.x);
		Setting.Min = static_cast<float>(MinInputResolution.x) / static_cast<float>(OutputResolution.x);
		Setting.Max = static_cast<float>(MaxInputResolution.x) / static_cast<float>(OutputResolution.x);
		if (Setting.Min < MinResolutionFraction)
		{
			MinResolutionFraction = Setting.Min;
		}
		if (Setting.Max > MaxResolutionFraction)
		{
			MaxResolutionFraction = Setting.Max;
		}
		ResolutionFractionSettings[XeSSUtil::ToIndex(TempQualitySetting)] = Setting;
	}
}

void FXeSSRHI::HandleXeSSEnabledSet(IConsoleVariable* Variable)
{
	// Return if no change as bool
	if (bCurrentXeSSEnabled == Variable->GetBool())
	{
		return;
	}
	bCurrentXeSSEnabled = Variable->GetBool();
	if (bCurrentXeSSEnabled)
	{
		// Re-initialize XeSS each time it is re-enabled
		InitArgs = FXeSSInitArguments();
	}
}

void FXeSSRHI::HandleXeSSQualitySet(IConsoleVariable* Variable)
{
	CVarXeSSOptimalScreenPercentage->Set(100.f *
		GetOptimalResolutionFraction(
			XeSSUtil::ToXeSSQualitySetting(Variable->GetInt())
		)
	);
}
