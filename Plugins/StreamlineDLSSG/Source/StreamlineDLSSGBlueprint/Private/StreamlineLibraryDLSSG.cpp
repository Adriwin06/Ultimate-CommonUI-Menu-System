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

#include "StreamlineLibraryDLSSG.h"
#include "StreamlineLibrary.h"

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
#include "HAL/IConsoleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FStreamlineDLSSGBlueprintModule"
DEFINE_LOG_CATEGORY_STATIC(LogStreamlineDLSSGBlueprint, Log, All);

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

EStreamlineFeatureSupport UStreamlineLibraryDLSSG::DLSSGSupport = EStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime;

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


bool UStreamlineLibraryDLSSG::IsDLSSGModeSupported(EStreamlineDLSSGMode DLSSGMode)
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(false);

	if (ValidateEnumValue(DLSSGMode, __FUNCTION__))
	{
		if (DLSSGMode == EStreamlineDLSSGMode::Off)
		{
			return true;
		}

		if (!IsDLSSGSupported()) // that returns false if WITH_STREAMLINE is false 
		{
			return false;
		}

#if WITH_STREAMLINE
		int32 MinNumGeneratedFrames = 0;
		int32 MaxNumGeneratedFrames = 0;

		GetStreamlineDLSSGMinMaxGeneratedFrames(MinNumGeneratedFrames, MaxNumGeneratedFrames);

		switch (DLSSGMode)
		{
			case EStreamlineDLSSGMode::Auto: /* fall through*/
			case EStreamlineDLSSGMode::On2X: return MinNumGeneratedFrames <= 1 && 1 <= MaxNumGeneratedFrames;
			case EStreamlineDLSSGMode::On3X: return MinNumGeneratedFrames <= 2 && 2 <= MaxNumGeneratedFrames;
			case EStreamlineDLSSGMode::On4X: return MinNumGeneratedFrames <= 3 && 3 <= MaxNumGeneratedFrames;
		}
#endif
		return false; 
	}
	
	return false;
}

TArray<EStreamlineDLSSGMode> UStreamlineLibraryDLSSG::GetSupportedDLSSGModes()
{
	TArray<EStreamlineDLSSGMode> SupportedQualityModes;

	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(SupportedQualityModes);

	const UEnum* Enum = StaticEnum<EStreamlineDLSSGMode>();
	for (int32 EnumIndex = 0; EnumIndex < Enum->NumEnums() - 1 /* avoid _MAX */; ++EnumIndex)
	{
		const int64 EnumValue = Enum->GetValueByIndex(EnumIndex);
		const EStreamlineDLSSGMode QualityMode = EStreamlineDLSSGMode(EnumValue);
		if (IsDLSSGModeSupported(QualityMode))
		{
			SupportedQualityModes.Add(QualityMode);
		}
	}
#if 0
	// This could be removed if we change the enum values to be in order of their UI appearance
	
	auto IfSupportedMoveToIndex = [&SupportedQualityModes](EStreamlineDLSSGMode EnumValue, int32 TargetIndexInArray)
	{
		int32 CurrentEnumValueIndex = INDEX_NONE;
		SupportedQualityModes.Find(EnumValue, CurrentEnumValueIndex);

		if (CurrentEnumValueIndex != INDEX_NONE && CurrentEnumValueIndex != TargetIndexInArray)
		{
			SupportedQualityModes.Swap(CurrentEnumValueIndex, TargetIndexInArray);
		}
	};

	IfSupportedMoveToIndex(EStreamlineDLSSGMode::Off, 0);
	IfSupportedMoveToIndex(EStreamlineDLSSGMode::Auto, 1);
	IfSupportedMoveToIndex(EStreamlineDLSSGMode::On1X, 2);
	IfSupportedMoveToIndex(EStreamlineDLSSGMode::On2X, 3);
	IfSupportedMoveToIndex(EStreamlineDLSSGMode::On3X, 4);
#endif 
	return SupportedQualityModes;
}

bool UStreamlineLibraryDLSSG::IsDLSSGSupported()
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(false);

#if WITH_STREAMLINE
	return QueryDLSSGSupport() == EStreamlineFeatureSupport::Supported;
#else
	return false;
#endif
}

EStreamlineFeatureSupport UStreamlineLibraryDLSSG::QueryDLSSGSupport()
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(EStreamlineFeatureSupport::NotSupported);

	return DLSSGSupport;
}

#if WITH_STREAMLINE
static int32 DLSSGModeIntCvarFromEnum(EStreamlineDLSSGMode DLSSGMode)
{
	switch (DLSSGMode)
	{
		case EStreamlineDLSSGMode::Off:
			return 0;	
		case EStreamlineDLSSGMode::On2X:
		case EStreamlineDLSSGMode::On3X:
		case EStreamlineDLSSGMode::On4X:
			return 1;
		case EStreamlineDLSSGMode::Auto:
			return 2;
		default:
			checkf(false, TEXT("dear DLSS-FG plugin developer, please support new enum type!"));
			return 0;
	}
}

static EStreamlineDLSSGMode DLSSGModeEnumFromIntCvar(int32 DLSSGMode, int FramesToGenerate)
{
	switch (DLSSGMode)
	{
		case 0:
			return EStreamlineDLSSGMode::Off;
		case 2:
			return EStreamlineDLSSGMode::Auto;
		// we intentionally fall through here and catch any weird state later		
	}

	switch (FramesToGenerate)
	{
		case 1:
			return EStreamlineDLSSGMode::On2X;
		case 2:
			return EStreamlineDLSSGMode::On3X;
		case 3: 
			return EStreamlineDLSSGMode::On4X;
		default:
			UE_LOG(LogStreamlineDLSSGBlueprint, Error, TEXT("Invalid r.Streamline.DLSSG.Enable value %d and/or r.Streamline.DLSSG.FramesToGenerate %d "), DLSSGMode, FramesToGenerate);
			return EStreamlineDLSSGMode::Off;
	}

}

static int32 DLSSGFramesToGenerateCvarFromEnum(EStreamlineDLSSGMode DLSSGMode)
{
	switch (DLSSGMode)
	{
		case EStreamlineDLSSGMode::Off:
		case EStreamlineDLSSGMode::Auto:
		case EStreamlineDLSSGMode::On2X:
			return 1;

		case EStreamlineDLSSGMode::On3X:
			return 2;
		case EStreamlineDLSSGMode::On4X:
			return 3;
		default:
			checkf(false, TEXT("dear DLSS-FG plugin developer, please support new enum type!"));
			return 1;
	}
}


#endif	// WITH_STREAMLINE

void UStreamlineLibraryDLSSG::SetDLSSGMode(EStreamlineDLSSGMode DLSSGMode)
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(void());

#if WITH_STREAMLINE
	// UEnums are strongly typed, but then one can also cast a byte to an UEnum ...
	if (ValidateEnumValue(DLSSGMode, __FUNCTION__))
	{
		static auto CVarEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable"));
		if (CVarEnable)
		{
			CVarEnable->SetWithCurrentPriority(DLSSGModeIntCvarFromEnum(DLSSGMode));
		}

		static auto CVarFramesToGenerate = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.FramesToGenerate"));
		if (CVarFramesToGenerate)
		{
			CVarFramesToGenerate->SetWithCurrentPriority(DLSSGFramesToGenerateCvarFromEnum(DLSSGMode));
		}
		
		if (DLSSGMode != EStreamlineDLSSGMode::Off)
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

STREAMLINEDLSSGBLUEPRINT_API void UStreamlineLibraryDLSSG::GetDLSSGFrameTiming(float& FrameRateInHertz, int32& FramesPresented)
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(void());

#if WITH_STREAMLINE
	GetStreamlineDLSSGFrameTiming(FrameRateInHertz, FramesPresented);
#endif
}

EStreamlineDLSSGMode UStreamlineLibraryDLSSG::GetDLSSGMode()
{

	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(EStreamlineDLSSGMode::Off);

#if WITH_STREAMLINE
	static const auto CVarEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable"));
	static auto CVarFramesToGenerate = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.FramesToGenerate"));

	if (CVarEnable != nullptr && CVarFramesToGenerate != nullptr)
	{
		return DLSSGModeEnumFromIntCvar(CVarEnable->GetInt(), CVarFramesToGenerate->GetInt());
	}
#endif
	return EStreamlineDLSSGMode::Off;
}

EStreamlineDLSSGMode UStreamlineLibraryDLSSG::GetDefaultDLSSGMode()
{
	TRY_INIT_STREAMLINE_DLSSG_LIBRARY_AND_RETURN(EStreamlineDLSSGMode::Off);


	if (UStreamlineLibraryDLSSG::IsDLSSGSupported())
	{
		if (IsDLSSGModeSupported(EStreamlineDLSSGMode::Auto))
		{
			return EStreamlineDLSSGMode::Auto;
		}
		else
		{
			return EStreamlineDLSSGMode::Off;
		}

	}
	else

	{
		return EStreamlineDLSSGMode::Off;
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
			DLSSGSupport = EStreamlineFeatureSupport::NotSupportedByRHI;
		}
	}
	else
	{
		if (GetPlatformStreamlineSupport() == EStreamlineSupport::NotSupportedIncompatibleRHI)
		{
			DLSSGSupport = EStreamlineFeatureSupport::NotSupportedByRHI;
		}
		else
		{
			DLSSGSupport = EStreamlineFeatureSupport::NotSupported;
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

	UStreamlineLibrary::RegisterFeatureSupport(EStreamlineFeature::DLSSG, UStreamlineLibraryDLSSG::QueryDLSSGSupport());
#else
	UE_LOG(LogStreamlineDLSSGBlueprint, Log, TEXT("Streamline is not supported on this platform at build time. The Streamline Blueprint library however is supported and stubbed out to ignore any calls to enable DLSS-G and will always return UStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime, regardless of the underlying hardware. This can be used to e.g. to turn off DLSS-G related UI elements."));
	UStreamlineLibraryDLSSG::DLSSGSupport = EStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime;
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

void FStreamlineLibraryDLSSGBlueprintModule::StartupModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool())
	{
		UE_LOG(LogStreamlineDLSSGBlueprint, Log, TEXT("Initialization of StreamlineBlueprint is disabled."));
		return;
	}

	UStreamlineLibraryDLSSG::Startup();
}


void FStreamlineLibraryDLSSGBlueprintModule::ShutdownModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool())
	{
		return;
	}

	UStreamlineLibraryDLSSG::Shutdown();
}

IMPLEMENT_MODULE(FStreamlineLibraryDLSSGBlueprintModule, StreamlineDLSSGBlueprint)

#undef LOCTEXT_NAMESPACE

