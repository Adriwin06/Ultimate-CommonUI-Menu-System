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

#include "StreamlineLibraryDeepDVC.h"
#if WITH_STREAMLINE
#include "StreamlineLibrary.h"
#include "StreamlineCore.h"
#include "StreamlineRHI.h"
#include "StreamlineDeepDVC.h"
#include "StreamlineAPI.h"
#include "sl.h"
#include "sl_deepdvc.h"
#endif

#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#ifdef __INTELLISENSE__
#define WITH_STREAMLINE 1
#endif

#define LOCTEXT_NAMESPACE "FStreamlineDeepDVCBlueprintModule"
DEFINE_LOG_CATEGORY_STATIC(LogStreamlineDeepDVCBlueprint, Log, All);

static const FName SetDeepDVCModeInvalidEnumValueError= FName("SetDeepDVCModeInvalidEnumValueError");
static const FName IsDeepDVCModeSupportedInvalidEnumValueError = FName("IsDeepDVCModeSupportedInvalidEnumValueError");

UStreamlineFeatureSupport UStreamlineLibraryDeepDVC::DeepDVCSupport = UStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime;
#if WITH_STREAMLINE

bool UStreamlineLibraryDeepDVC::bDeepDVCLibraryInitialized = false;

static bool ShowDeepDVCSDebugOnScreenMessages()
{
	return true;
}


#if !UE_BUILD_SHIPPING

UStreamlineLibraryDeepDVC::FDLSSErrorState UStreamlineLibraryDeepDVC::DLSSErrorState;
FDelegateHandle UStreamlineLibraryDeepDVC::DeepDVCOnScreenMessagesDelegateHandle;
void UStreamlineLibraryDeepDVC::GetDeepDVCOnScreenMessages(TMultiMap<FCoreDelegates::EOnScreenMessageSeverity, FText>& OutMessages)
{
	check(IsInGameThread());

	// We need a valid DLSSSupport, so calling this here in case other UStreamlineLibraryDeepDVC functions which call TryInitStreamlineLibrary() haven't been called
	if (!TryInitDeepDVCLibrary())
	{
		return;
	}
}
#endif

#endif

bool UStreamlineLibraryDeepDVC::IsDeepDVCModeSupported(UStreamlineDeepDVCMode DeepDVCMode)
{
	const UEnum* Enum = StaticEnum<UStreamlineDeepDVCMode>();

	// UEnums are strongly typed, but then one can also cast a byte to an UEnum ...
	if (Enum->IsValidEnumValue(int64(DeepDVCMode)) && (Enum->GetMaxEnumValue() != int64(DeepDVCMode)))
	{
		if (DeepDVCMode == UStreamlineDeepDVCMode::Off)
		{
			return true;
		}
#if WITH_STREAMLINE
		if (!TryInitDeepDVCLibrary())
		{
			UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("IsDeepDVCModeSupported should not be called before PostEngineInit"));
			return false;
		}
		if (!IsDeepDVCSupported())
		{
			return false;
		}
		else
		{
			return true; // TODO
		}
#else
		return false;
#endif
	}
	else
	{
#if !UE_BUILD_SHIPPING
		FFrame::KismetExecutionMessage(*FString::Printf(
			TEXT("IsDeepDVCModeSupported should not be called with an invalid DeepDVCMode enum value (%d) \"%s\""),
			int64(DeepDVCMode), *StaticEnum<UStreamlineDeepDVCMode>()->GetDisplayNameTextByValue(int64(DeepDVCMode)).ToString()),
			ELogVerbosity::Error, IsDeepDVCModeSupportedInvalidEnumValueError);
#endif 
		return false;
	}

}

TArray<UStreamlineDeepDVCMode> UStreamlineLibraryDeepDVC::GetSupportedDeepDVCModes()
{
	TArray<UStreamlineDeepDVCMode> SupportedQualityModes;
#if WITH_STREAMLINE
	if (!TryInitDeepDVCLibrary())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("GetSupportedDeepDVCModes should not be called before PostEngineInit"));
		return SupportedQualityModes;
	}
#endif
	{
		const UEnum* Enum = StaticEnum<UStreamlineDeepDVCMode>();
		for (int32 EnumIndex = 0; EnumIndex < Enum->NumEnums(); ++EnumIndex)
		{
			const int64 EnumValue = Enum->GetValueByIndex(EnumIndex);
			if (EnumValue != Enum->GetMaxEnumValue())
			{
				const UStreamlineDeepDVCMode QualityMode = UStreamlineDeepDVCMode(EnumValue);
				if (IsDeepDVCModeSupported(QualityMode))
				{
					SupportedQualityModes.Add(QualityMode);
				}
			}
		}
	}
	return SupportedQualityModes;
}

bool UStreamlineLibraryDeepDVC::IsDeepDVCSupported()
{
#if WITH_STREAMLINE
	if (!TryInitDeepDVCLibrary())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("IsDeepDVCSupported should not be called before PostEngineInit"));
		return false;
	}

	return QueryDeepDVCSupport() == UStreamlineFeatureSupport::Supported;
#else
	return false;
#endif
}

UStreamlineFeatureSupport UStreamlineLibraryDeepDVC::QueryDeepDVCSupport()
{
#if WITH_STREAMLINE
	if (!TryInitDeepDVCLibrary())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("QueryDeepDVCSupport should not be called before PostEngineInit"));
		return UStreamlineFeatureSupport::NotSupported;
	}
#endif
	return DeepDVCSupport;
}

#if WITH_STREAMLINE
static int32 DeepDVCModeIntCvarFromEnum(UStreamlineDeepDVCMode DeepDVCMode)
{
	switch (DeepDVCMode)
	{
	case UStreamlineDeepDVCMode::Off:
		return 0;
	case UStreamlineDeepDVCMode::On:
		return 1;
	default:
		checkf(false, TEXT("dear DeepDVC plugin developer, please support new enum type!"));
		return 0;
	}
}

static UStreamlineDeepDVCMode DeepDVCModeEnumFromIntCvar(int32 DeepDVCMode)
{
	switch (DeepDVCMode)
	{
	case 0:
		return UStreamlineDeepDVCMode::Off;
	case 1:
		return UStreamlineDeepDVCMode::On;
	default:
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("Invalid r.Streamline.DeepDVC.Enable value %d"), DeepDVCMode);
		return UStreamlineDeepDVCMode::Off;
	}
}
#endif	// WITH_STREAMLINE

void UStreamlineLibraryDeepDVC::SetDeepDVCMode(UStreamlineDeepDVCMode DeepDVCMode)
{
#if WITH_STREAMLINE
	if (!TryInitDeepDVCLibrary())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("SetDeepDVCMode should not be called before PostEngineInit"));
		return;
	}

	const UEnum* Enum = StaticEnum<UStreamlineDeepDVCMode>();

	// UEnums are strongly typed, but then one can also cast a byte to an UEnum ...
	if(Enum->IsValidEnumValue(int64(DeepDVCMode)) && (Enum->GetMaxEnumValue() != int64(DeepDVCMode)))
	{
		static auto CVarDeepDVCEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DeepDVC.Enable"));
		if (CVarDeepDVCEnable)
		{
			CVarDeepDVCEnable->SetWithCurrentPriority(DeepDVCModeIntCvarFromEnum(DeepDVCMode));
		}
		
		if (DeepDVCMode != UStreamlineDeepDVCMode::Off)
		{
#if !UE_BUILD_SHIPPING
			check(IsInGameThread());
			DLSSErrorState.bIsDeepDVCModeUnsupported = !IsDeepDVCModeSupported(DeepDVCMode);
			DLSSErrorState.InvalidDeepDVCMode = DeepDVCMode;
#endif 
		}
	}
	else
	{
#if !UE_BUILD_SHIPPING
		FFrame::KismetExecutionMessage(*FString::Printf(
			TEXT("SetDeepDVCMode should not be called with an invalid DeepDVCMode enum value (%d) \"%s\""), 
			int64(DeepDVCMode), *StaticEnum<UStreamlineDeepDVCMode>()->GetDisplayNameTextByValue(int64(DeepDVCMode)).ToString()),
			ELogVerbosity::Error, SetDeepDVCModeInvalidEnumValueError);
#endif 
	}
#endif	// WITH_STREAMLINE
}

UStreamlineDeepDVCMode UStreamlineLibraryDeepDVC::GetDeepDVCMode()
{
#if WITH_STREAMLINE
	if (!TryInitDeepDVCLibrary())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("GetDeepDVCMode should not be called before PostEngineInit"));
		return UStreamlineDeepDVCMode::Off;
	}

	static const auto CVarDeepDVCEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DeepDVC.Enable"));
	if (CVarDeepDVCEnable != nullptr)
	{
		return DeepDVCModeEnumFromIntCvar(CVarDeepDVCEnable->GetInt());
	}
#endif
	return UStreamlineDeepDVCMode::Off;
}

UStreamlineDeepDVCMode UStreamlineLibraryDeepDVC::GetDefaultDeepDVCMode()
{
#if WITH_STREAMLINE
	if (!TryInitDeepDVCLibrary())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("GetDefaultDeepDVCMode should not be called before PostEngineInit"));
		return UStreamlineDeepDVCMode::Off;
	}
#endif
	if (UStreamlineLibraryDeepDVC::IsDeepDVCSupported())
	{
		return UStreamlineDeepDVCMode::Off;
	}
	else
	{
		return UStreamlineDeepDVCMode::Off;
	}
}

STREAMLINEDEEPDVCBLUEPRINT_API void UStreamlineLibraryDeepDVC::SetDeepDVCIntensity(float Intensity)
{
#if WITH_STREAMLINE
	if (!TryInitDeepDVCLibrary())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("SetDeepDVCIntensity should not be called before PostEngineInit"));
		return ;
	}

	static const auto CVarDeepDVCIntensity = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DeepDVC.Intensity"));
	if (CVarDeepDVCIntensity)
	{
		// Quantize here so we can have snap the value to 0, which downstream is used to turn off the DeepDVC implicitely
		// CVarDeepDVCSaturationBoost->Set(..., ECVF_SetByCommandline) internally uses	Set(*FString::Printf(TEXT("%g"), InValue),...); which doesn't snap to 0
		CVarDeepDVCIntensity->Set(*FString::Printf(TEXT("%2.2f"), Intensity), ECVF_SetByCommandline);
	}

#endif

}

STREAMLINEDEEPDVCBLUEPRINT_API float UStreamlineLibraryDeepDVC::GetDeepDVCIntensity()
{
#if WITH_STREAMLINE
	if (!TryInitDeepDVCLibrary())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("GetDeepDVCIntensity should not be called before PostEngineInit"));
		return 0.0f;
	}

	static const auto CVarDeepDVCIntensity = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DeepDVC.Intensity"));

	if (CVarDeepDVCIntensity)
	{
		return CVarDeepDVCIntensity->GetFloat();
	}

#endif
	return 0.0f;
}

STREAMLINEDEEPDVCBLUEPRINT_API void UStreamlineLibraryDeepDVC::SetDeepDVCSaturationBoost(float SaturationBoost)
{
#if WITH_STREAMLINE
	if (!TryInitDeepDVCLibrary())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("SetDeepDVCSaturationBoost should not be called before PostEngineInit"));
		return;
	}

	static const auto CVarDeepDVCSaturationBoost = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DeepDVC.SaturationBoost"));
	if (CVarDeepDVCSaturationBoost)
	{
		// Quantize here so we can have snap the value to 0, which is nice because hitting 0 is useful
		// CVarDeepDVCSaturationBoost->Set(..., ECVF_SetByCommandline) internally uses	Set(*FString::Printf(TEXT("%g"), InValue),...); which doesn't snap to 0
		CVarDeepDVCSaturationBoost->Set(*FString::Printf(TEXT("%2.2f"), SaturationBoost), ECVF_SetByCommandline);
	}
#endif
}

STREAMLINEDEEPDVCBLUEPRINT_API float UStreamlineLibraryDeepDVC::GetDeepDVCSaturationBoost()
{
#if WITH_STREAMLINE
	if (!TryInitDeepDVCLibrary())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Error, TEXT("GetDeepDVCSaturationBoost should not be called before PostEngineInit"));
		return 0.0f;
	}

	static const auto CVarDeepDVCSaturationBoost = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DeepDVC.SaturationBoost"));

	if (CVarDeepDVCSaturationBoost)
	{
		return CVarDeepDVCSaturationBoost->GetFloat();
	}

#endif
	return  0.0f;
}

#if WITH_STREAMLINE

// Delayed initialization, which allows this module to be available early so blueprints can be loaded before DLSS is available in PostEngineInit
bool UStreamlineLibraryDeepDVC::TryInitDeepDVCLibrary()
{
	if (bDeepDVCLibraryInitialized)
	{
		// TODO
		return true;
	}

	// Register this before we bail out so we can show error messages
#if !UE_BUILD_SHIPPING
	if (!DeepDVCOnScreenMessagesDelegateHandle.IsValid())
	{
		DeepDVCOnScreenMessagesDelegateHandle = FCoreDelegates::OnGetOnScreenMessages.AddStatic(&GetDeepDVCOnScreenMessages);
	}
#endif

	

	if (IsStreamlineSupported())
	{
		if (GetPlatformStreamlineRHI()->IsDeepDVCSupportedByRHI())
		{

			DeepDVCSupport = ToUStreamlineFeatureSupport(QueryStreamlineDeepDVCSupport());
		}
		else
		{
			DeepDVCSupport = UStreamlineFeatureSupport::NotSupportedByRHI;
		}
	}
	else
	{
		if (GetPlatformStreamlineSupport() == EStreamlineSupport::NotSupportedIncompatibleRHI)
		{
			DeepDVCSupport = UStreamlineFeatureSupport::NotSupportedByRHI;
		}
		else
		{
			DeepDVCSupport = UStreamlineFeatureSupport::NotSupported;
		}
	}

	bDeepDVCLibraryInitialized = true;

	return true;
}
#endif // WITH_STREAMLINE


void UStreamlineLibraryDeepDVC::Startup()
{
#if WITH_STREAMLINE
	// This initialization will likely not succeed unless this module has been moved to PostEngineInit, and that's ok
	UStreamlineLibraryDeepDVC::TryInitDeepDVCLibrary();
	UStreamlineLibrary::RegisterFeatureSupport(UStreamlineFeature::DeepDVC, UStreamlineLibraryDeepDVC::QueryDeepDVCSupport());
#else
	UE_LOG(LogStreamlineDeepDVCBlueprint, Log, TEXT("Streamline is not supported on this platform at build time. The Streamline Blueprint library however is supported and stubbed out to ignore any calls to enable DLSS-G and will always return UStreamlineDeepDVCSupport::NotSupportedByPlatformAtBuildTime, regardless of the underlying hardware. This can be used to e.g. to turn off DLSS-G related UI elements."));
	UStreamlineLibraryDeepDVC::DeepDVCSupport = UStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime;
#endif
}
void UStreamlineLibraryDeepDVC::Shutdown()
{
#if WITH_STREAMLINE && !UE_BUILD_SHIPPING
	if (UStreamlineLibraryDeepDVC::DeepDVCOnScreenMessagesDelegateHandle.IsValid())
	{
		FCoreDelegates::OnGetOnScreenMessages.Remove(UStreamlineLibraryDeepDVC::DeepDVCOnScreenMessagesDelegateHandle);
		UStreamlineLibraryDeepDVC::DeepDVCOnScreenMessagesDelegateHandle.Reset();
	}
#endif
}




void FStreamlineLibraryDeepDVCBlueprintModule::StartupModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool())
	{
		UE_LOG(LogStreamlineDeepDVCBlueprint, Log, TEXT("Initialization of StreamlineBlueprint is disabled."));
		return;
	}

	UStreamlineLibraryDeepDVC::Startup();
}

void FStreamlineLibraryDeepDVCBlueprintModule::ShutdownModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool())
	{
		return;
	}

	UStreamlineLibraryDeepDVC::Shutdown();
}

IMPLEMENT_MODULE(FStreamlineLibraryDeepDVCBlueprintModule, StreamlineDeepDVCBlueprint)

#undef LOCTEXT_NAMESPACE