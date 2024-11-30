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

#include "XeSSBlueprintLibrary.h"

#if USE_XESS
#include "xess.h"
#include "XeSSRHI.h"
#include "XeSSUpscaler.h"
#include "XeSSUtil.h"
#endif // USE_XESS

#define LOCTEXT_NAMESPACE "UXeSSBlueprintLibrary"

bool UXeSSBlueprintLibrary::bXeSSSupported = false;
#if USE_XESS
FXeSSRHI* UXeSSBlueprintLibrary::XeSSRHI = nullptr;
FXeSSUpscaler* UXeSSBlueprintLibrary::XeSSUpscaler = nullptr;
// QUALITY EDIT:
static TMap<EXeSSQualityMode, xess_quality_settings_t> EnabledQualityMap = {
	{EXeSSQualityMode::UltraPerformance, XESS_QUALITY_SETTING_ULTRA_PERFORMANCE},
	{EXeSSQualityMode::Performance, XESS_QUALITY_SETTING_PERFORMANCE},
	{EXeSSQualityMode::Balanced, XESS_QUALITY_SETTING_BALANCED},
	{EXeSSQualityMode::Quality, XESS_QUALITY_SETTING_QUALITY},
	{EXeSSQualityMode::UltraQuality, XESS_QUALITY_SETTING_ULTRA_QUALITY},
	{EXeSSQualityMode::UltraQualityPlus, XESS_QUALITY_SETTING_ULTRA_QUALITY_PLUS},
	{EXeSSQualityMode::AntiAliasing, XESS_QUALITY_SETTING_AA},
};

static FString GetDisplayName(EXeSSQualityMode QualityMode)
{
	static UEnum* Enum = StaticEnum<EXeSSQualityMode>();
	return Enum->GetDisplayNameTextByValue(int32(QualityMode)).ToString();
}

static bool IsValidEnumValue(EXeSSQualityMode QualityMode)
{
	static UEnum* Enum = StaticEnum<EXeSSQualityMode>();
	int32 QualityModeInt = (int32)QualityMode;
	return Enum->IsValidEnumValue(QualityModeInt) && QualityModeInt != Enum->GetMaxEnumValue();
}

static xess_quality_settings_t ToXeSSQualitySetting(EXeSSQualityMode QualityMode)
{
	xess_quality_settings_t QualitySetting = XESS_QUALITY_SETTING_BALANCED;

	if (EnabledQualityMap.Contains(QualityMode))
	{
		QualitySetting = EnabledQualityMap[QualityMode];
	}
	else
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("ToXeSSQualitySetting called with invalid enum value (%d) %s"),
			int32(QualityMode), *GetDisplayName(QualityMode)), ELogVerbosity::Error);
	}
	return QualitySetting;
}

static EXeSSQualityMode ToXeSSQualityMode(int32 CVarInt)
{
	xess_quality_settings_t QualitySetting = XeSSUtil::ToXeSSQualitySetting(CVarInt);

	for (const auto& Pair : EnabledQualityMap)
	{
		if (Pair.Value == QualitySetting) 
		{
			return Pair.Key;
		}
	}
	FFrame::KismetExecutionMessage(*FString::Printf(TEXT("ToXeSSQualityMode called with invalid value (%d)"),
		CVarInt), ELogVerbosity::Error);
	return EXeSSQualityMode::Off;
}

#endif

bool UXeSSBlueprintLibrary::IsXeSSSupported()
{
	return UXeSSBlueprintLibrary::bXeSSSupported;
}

TArray<EXeSSQualityMode> UXeSSBlueprintLibrary::GetSupportedXeSSQualityModes()
{
	TArray<EXeSSQualityMode> SupportedXeSSQualityModes;
	const UEnum* QualityModeEnum = StaticEnum<EXeSSQualityMode>();

	for (int32 EnumIndex = 0; EnumIndex < QualityModeEnum->NumEnums(); ++EnumIndex)
	{
		const int32 EnumValue = (int32)QualityModeEnum->GetValueByIndex(EnumIndex);
		if (EnumValue != QualityModeEnum->GetMaxEnumValue())
		{
			SupportedXeSSQualityModes.Add(EXeSSQualityMode(EnumValue));
		}
	}

	return SupportedXeSSQualityModes;
}

EXeSSQualityMode UXeSSBlueprintLibrary::GetXeSSQualityMode()
{
#if USE_XESS
	static const auto CVarXeSSQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Quality"));

	// If XeSS Upscaler did not initialize correctly
	if (!UXeSSBlueprintLibrary::XeSSUpscaler)
	{
		return EXeSSQualityMode::Off;
	}

	if (!UXeSSBlueprintLibrary::XeSSUpscaler->IsXeSSEnabled())
	{
		return EXeSSQualityMode::Off;
	}

	return ToXeSSQualityMode(CVarXeSSQuality->GetInt());
#else 
	return EXeSSQualityMode::Off;
#endif
}

EXeSSQualityMode UXeSSBlueprintLibrary::GetDefaultXeSSQualityMode(FIntPoint ScreenResolution)
{
	// For resolutions equal to and lower than 2560x1440 the default quality mode should be set to Balanced
	// Otherwise Performance should be used
	int PixelCount = ScreenResolution.X * ScreenResolution.Y;
	if (PixelCount <= 2560 * 1440)
	{
		return EXeSSQualityMode::Balanced;
	}
	
	return EXeSSQualityMode::Performance;
}

void UXeSSBlueprintLibrary::SetXeSSQualityMode(EXeSSQualityMode QualityMode)
{
#if USE_XESS
	if (!IsValidEnumValue(QualityMode))
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("SetXeSSQualityMode called with invalid enum value (%d) %s"),
			int32(QualityMode), *GetDisplayName(QualityMode)), ELogVerbosity::Error);
		return;
	}

	static const auto CVarXeSSEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));
	static const auto CVarXeSSQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Quality"));
	static const auto CVarScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));

	if (QualityMode == EXeSSQualityMode::Off)
	{
		CVarXeSSEnabled->SetWithCurrentPriority(0);

#if XESS_ENGINE_VERSION_GEQ(5, 1)
		// Only set if not in editor(no effect by default)
		if (!GIsEditor) 
		{
			CVarScreenPercentage->SetWithCurrentPriority(100.f);
		}
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)

		return;
	}

	CVarXeSSQuality->SetWithCurrentPriority(XeSSUtil::ToCVarInt(ToXeSSQualitySetting(QualityMode)));
	CVarXeSSEnabled->SetWithCurrentPriority(1);

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	// Only set if not in editor(no effect by default)
	if (!GIsEditor)
	{
		float ScreenPercentage = 100.f;
		if (GetXeSSQualityModeInformation(QualityMode, ScreenPercentage)) 
		{
			CVarScreenPercentage->SetWithCurrentPriority(ScreenPercentage);
		}
	}
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)

#endif // USE_XESS
}

XESSBLUEPRINT_API bool UXeSSBlueprintLibrary::GetXeSSQualityModeInformation(EXeSSQualityMode QualityMode, float& ScreenPercentage)
{
#if USE_XESS
	if (!IsValidEnumValue(QualityMode))
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("GetXeSSQualityModeInformation called with invalid enum value (%d) %s"),
			int32(QualityMode), *GetDisplayName(QualityMode)), ELogVerbosity::Error);
		return false;
	}
	if (!XeSSRHI)
	{
		return false;
	}

	if (QualityMode == EXeSSQualityMode::Off) {
		return false;
	}
	ScreenPercentage = XeSSRHI->GetOptimalResolutionFraction(ToXeSSQualitySetting(QualityMode)) * 100.f;
	return true;
#else // USE_XESS
	return false;
#endif // USE_XESS
}

#undef LOCTEXT_NAMESPACE
