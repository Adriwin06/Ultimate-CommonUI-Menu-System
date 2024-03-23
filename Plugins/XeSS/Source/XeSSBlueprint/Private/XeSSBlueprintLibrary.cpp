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

	switch (QualityMode)
	{
	case EXeSSQualityMode::Performance:
		QualitySetting = XESS_QUALITY_SETTING_PERFORMANCE;
		break;
	case EXeSSQualityMode::Balanced:
		QualitySetting = XESS_QUALITY_SETTING_BALANCED;
		break;
	case EXeSSQualityMode::Quality:
		QualitySetting = XESS_QUALITY_SETTING_QUALITY;
		break;
	case EXeSSQualityMode::UltraQuality:
		QualitySetting = XESS_QUALITY_SETTING_ULTRA_QUALITY;
		break;
	default:
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("ToXeSSQualitySetting called with invalid enum value (%d) %s"),
			int32(QualityMode), *GetDisplayName(QualityMode)), ELogVerbosity::Error);
		break;
	}
	return QualitySetting;
}

static EXeSSQualityMode ToXeSSQualityMode(int32 CVarInt)
{
	xess_quality_settings_t QualitySetting = XeSSUtil::ToXeSSQualitySetting(CVarInt);

	switch (QualitySetting)
	{
	case XESS_QUALITY_SETTING_PERFORMANCE:
		return EXeSSQualityMode::Performance;
	case XESS_QUALITY_SETTING_BALANCED:
		return EXeSSQualityMode::Balanced;
	case XESS_QUALITY_SETTING_QUALITY:
		return EXeSSQualityMode::Quality;
	case XESS_QUALITY_SETTING_ULTRA_QUALITY:
		return EXeSSQualityMode::UltraQuality;
	default:
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("ToXeSSQualityMode called with invalid value (%d)"),
			CVarInt), ELogVerbosity::Error);
		return EXeSSQualityMode::Off;
	}
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
		CVarXeSSEnabled->Set(0, ECVF_SetByCode);

#if XESS_ENGINE_VERSION_GEQ(5, 1)
		// Only set if not in editor(no effect by default)
		if (!GIsEditor) 
		{
			CVarScreenPercentage->Set(100.f, ECVF_SetByCode);
		}
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)

		return;
	}

	CVarXeSSQuality->Set(XeSSUtil::ToCVarInt(ToXeSSQualitySetting(QualityMode)), ECVF_SetByCode);
	CVarXeSSEnabled->Set(1, ECVF_SetByCode);

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	// Only set if not in editor(no effect by default)
	if (!GIsEditor)
	{
		float ScreenPercentage = 100.f;
		if (GetXeSSQualityModeInformation(QualityMode, ScreenPercentage)) 
		{
			CVarScreenPercentage->Set(ScreenPercentage, ECVF_SetByCode);
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
