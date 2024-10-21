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

#include "StreamlineLibraryDLSSG.h"

#if WITH_STREAMLINE
#include "StreamlineCore.h"
#include "StreamlineRHI.h"
#include "StreamlineReflex.h"
#include "StreamlineDLSSG.h"
#include "StreamlineAPI.h"
#include "sl.h"
#include "sl_dlss_g.h"
#endif

#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FStreammlineBlueprintModule"
DEFINE_LOG_CATEGORY_STATIC(LogStreamlineDLSSGBlueprint, Log, All);

// that eventually should get moved into a separate DLSS-FG BP library plugin
#if WITH_STREAMLINE

#define TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(ReturnValueOrEmptyOrVoidPreFiveThree) \
if (!TryInitDLSSGLibrary()) \
{ \
	UE_LOG(LogStreamlineDLSSGBlueprint, Error, TEXT("%s should not be called before PostEngineInit"), ANSI_TO_TCHAR(__FUNCTION__)); \
	return ReturnValueOrEmptyOrVoidPreFiveThree; \
}

#else

#define TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(ReturnValueWhichCanBeEmpty) 

#endif

UStreamlineFeatureSupport UStreamlineLibraryDLSSG::DLSSGSupport = UStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime;

#if WITH_STREAMLINE

bool UStreamlineLibraryDLSSG::bDLSSGLibraryInitialized = false;

static bool ShowDLSSSDebugOnScreenMessages()
{
	return true;
	//if (GetDefault<UDLSSOverrideSettings>()->ShowDLSSSDebugOnScreenMessages == EDLSSSettingOverride::UseProjectSettings)
	//{
	//	return GetDefault<UDLSSSettings>()->bLogStreamlineBlueprint;
	//}
	//else
	//{
	//	return GetDefault<UDLSSOverrideSettings>()->ShowDLSSSDebugOnScreenMessages == EDLSSSettingOverride::Enabled;
	//}
}


#if !UE_BUILD_SHIPPING

UStreamlineLibraryDLSSG::FDLSSErrorState UStreamlineLibraryDLSSG::DLSSErrorState;
FDelegateHandle UStreamlineLibraryDLSSG::DLSSOnScreenMessagesDelegateHandle;
void UStreamlineLibraryDLSSG::GetDLSSOnScreenMessages(TMultiMap<FCoreDelegates::EOnScreenMessageSeverity, FText>& OutMessages)
{
	check(IsInGameThread());

	// We need a valid DLSSSupport, so calling this here in case other UStreamlineLibraryDLSSG functions which call TryInitStreamlineLibrary() haven't been called
	if (!TryInitDLSSGLibrary())
	{
		return;
	}

	// TODO
	//if(ShowDLSSSDebugOnScreenMessages())
	//{
	//
	//}
}
#endif

#endif


bool UStreamlineLibraryDLSSG::IsDLSSGModeSupported(UStreamlineDLSSGMode DLSSGMode)
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(false);

	const UEnum* Enum = StaticEnum<UStreamlineDLSSGMode>();

	if (ValidateEnumValue(DLSSGMode, __FUNCTION__))
	{
		if (DLSSGMode == UStreamlineDLSSGMode::Off)
		{
			return true;
		}

		if (!IsDLSSGSupported()) // that returns false if WITH_STREAMLINE is false 
		{
			return false;
		}

		return true; // TODO, right now On and Auto are always supported
	}
	
	return false;
}

TArray<UStreamlineDLSSGMode> UStreamlineLibraryDLSSG::GetSupportedDLSSGModes()
{
	TArray<UStreamlineDLSSGMode> SupportedQualityModes;

	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(SupportedQualityModes);

	const UEnum* Enum = StaticEnum<UStreamlineDLSSGMode>();
	for (int32 EnumIndex = 0; EnumIndex < Enum->NumEnums(); ++EnumIndex)
	{
		const int64 EnumValue = Enum->GetValueByIndex(EnumIndex);
		if (EnumValue != Enum->GetMaxEnumValue())
		{
			const UStreamlineDLSSGMode QualityMode = UStreamlineDLSSGMode(EnumValue);
			if (IsDLSSGModeSupported(QualityMode))
			{
				SupportedQualityModes.Add(QualityMode);
			}
		}
	}
	return SupportedQualityModes;
}

bool UStreamlineLibraryDLSSG::IsDLSSGSupported()
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(false);

#if WITH_STREAMLINE
	return QueryDLSSGSupport() == UStreamlineFeatureSupport::Supported;
#else
	return false;
#endif
}

UStreamlineFeatureSupport UStreamlineLibraryDLSSG::QueryDLSSGSupport()
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(UStreamlineFeatureSupport::NotSupported);

	return DLSSGSupport;
}

#if WITH_STREAMLINE
static int32 DLSSGModeIntCvarFromEnum(UStreamlineDLSSGMode DLSSGMode)
{
	switch (DLSSGMode)
	{
	case UStreamlineDLSSGMode::Off:
		return 0;
	case UStreamlineDLSSGMode::On:
		return 1;
	case UStreamlineDLSSGMode::Auto:
		return 2;
	default:
		checkf(false, TEXT("dear DLSS-FG plugin developer, please support new enum type!"));
		return 0;
	}
}

static UStreamlineDLSSGMode DLSSGModeEnumFromIntCvar(int32 DLSSGMode)
{
	switch (DLSSGMode)
	{
	case 0:
		return UStreamlineDLSSGMode::Off;
	case 1:
		return UStreamlineDLSSGMode::On;
	case 2:
		return UStreamlineDLSSGMode::Auto;
	default:
		UE_LOG(LogStreamlineDLSSGBlueprint, Error, TEXT("Invalid r.Streamline.DLSSG.Enable value %d"), DLSSGMode);
		return UStreamlineDLSSGMode::Off;
	}
}
#endif	// WITH_STREAMLINE

void UStreamlineLibraryDLSSG::SetDLSSGMode(UStreamlineDLSSGMode DLSSGMode)
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(void());

#if WITH_STREAMLINE
	const UEnum* Enum = StaticEnum<UStreamlineDLSSGMode>();

	// UEnums are strongly typed, but then one can also cast a byte to an UEnum ...
	if (ValidateEnumValue(DLSSGMode, __FUNCTION__))
	{
		static auto CVarDLSSGEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable"));
		if (CVarDLSSGEnable)
		{
			CVarDLSSGEnable->SetWithCurrentPriority(DLSSGModeIntCvarFromEnum(DLSSGMode));
		}
		
		if (DLSSGMode != UStreamlineDLSSGMode::Off)
		{
#if !UE_BUILD_SHIPPING
			check(IsInGameThread());
			DLSSErrorState.bIsDLSSGModeUnsupported = !IsDLSSGModeSupported(DLSSGMode);
			DLSSErrorState.InvalidDLSSGMode = DLSSGMode;
#endif 
		}
	}
#endif	// WITH_STREAMLINE
}

STREAMLINEBLUEPRINT_API void UStreamlineLibraryDLSSG::GetDLSSGFrameTiming(float& FrameRateInHertz, int32& FramesPresented)
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(void());

#if WITH_STREAMLINE
	GetStreamlineDLSSGFrameTiming(FrameRateInHertz, FramesPresented);
#endif
}

UStreamlineDLSSGMode UStreamlineLibraryDLSSG::GetDLSSGMode()
{

	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(UStreamlineDLSSGMode::Off);

#if WITH_STREAMLINE
	static const auto CVarDLSSGEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable"));
	if (CVarDLSSGEnable != nullptr)
	{
		return DLSSGModeEnumFromIntCvar(CVarDLSSGEnable->GetInt());
	}
#endif
	return UStreamlineDLSSGMode::Off;
}

UStreamlineDLSSGMode UStreamlineLibraryDLSSG::GetDefaultDLSSGMode()
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(UStreamlineDLSSGMode::Off);

	if (UStreamlineLibraryDLSSG::IsDLSSGSupported())
	{
		return UStreamlineDLSSGMode::Off;
	}
	else
	{
		return UStreamlineDLSSGMode::Off;
	}
}


#if WITH_STREAMLINE

// Delayed initialization, which allows this module to be available early so blueprints can be loaded before DLSS is available in PostEngineInit
bool UStreamlineLibraryDLSSG::TryInitDLSSGLibrary()
{
	if (bDLSSGLibraryInitialized)
	{
		// TODO
		return true;
	}

	// Register this before we bail out so we can show error messages
#if !UE_BUILD_SHIPPING
	if (!DLSSOnScreenMessagesDelegateHandle.IsValid())
	{
		DLSSOnScreenMessagesDelegateHandle = FCoreDelegates::OnGetOnScreenMessages.AddStatic(&GetDLSSOnScreenMessages);
	}
#endif

	

	if (IsStreamlineSupported())
	{
		if (GetPlatformStreamlineRHI()->IsDLSSGSupportedByRHI())
		{

			DLSSGSupport = ToUStreamlineFeatureSupport(QueryStreamlineDLSSGSupport());
		}
		else
		{
			DLSSGSupport = UStreamlineFeatureSupport::NotSupportedByRHI;
		}
	}
	else
	{
		if (GetPlatformStreamlineSupport() == EStreamlineSupport::NotSupportedIncompatibleRHI)
		{
			DLSSGSupport = UStreamlineFeatureSupport::NotSupportedByRHI;
		}
		else
		{
			DLSSGSupport = UStreamlineFeatureSupport::NotSupported;
		}
	}

	bDLSSGLibraryInitialized = true;

	return true;
}
#endif // WITH_STREAMLINE


void UStreamlineLibraryDLSSG::Startup()
{
#if WITH_STREAMLINE
	// This initialization will likely not succeed unless this module has been moved to PostEngineInit, and that's ok
	UStreamlineLibraryDLSSG::TryInitDLSSGLibrary();

	UStreamlineLibrary::RegisterFeatureSupport(UStreamlineFeature::DLSSG, UStreamlineLibraryDLSSG::QueryDLSSGSupport());
#else
	UE_LOG(LogStreamlineDLSSGBlueprint, Log, TEXT("Streamline is not supported on this platform at build time. The Streamline Blueprint library however is supported and stubbed out to ignore any calls to enable DLSS-G and will always return UStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime, regardless of the underlying hardware. This can be used to e.g. to turn off DLSS-G related UI elements."));
	UStreamlineLibraryDLSSG::DLSSGSupport = UStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime;
#endif
}
void UStreamlineLibraryDLSSG::Shutdown()
{
#if WITH_STREAMLINE && !UE_BUILD_SHIPPING
	if (UStreamlineLibraryDLSSG::DLSSOnScreenMessagesDelegateHandle.IsValid())
	{
		FCoreDelegates::OnGetOnScreenMessages.Remove(UStreamlineLibraryDLSSG::DLSSOnScreenMessagesDelegateHandle);
		UStreamlineLibraryDLSSG::DLSSOnScreenMessagesDelegateHandle.Reset();
	}
#endif
}



#undef LOCTEXT_NAMESPACE