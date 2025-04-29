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

#include "StreamlineLibraryReflex.h"
#include "StreamlineLibrary.h"


#if WITH_STREAMLINE
#include "StreamlineReflex.h"
#include "StreamlineCore.h"
#include "StreamlineRHI.h"

#endif

#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FStreamlineReflexBlueprintModule"
DEFINE_LOG_CATEGORY_STATIC(LogStreamlineReflexBlueprint, Log, All);

#if WITH_STREAMLINE

#define TRY_INIT_STREAMLINE_REFLEX_LIBRARY_AND_RETURN(ReturnValueOrEmptyOrVoidPreFiveThree) \
if (!TryInitLibrary()) \
{ \
	UE_LOG(LogStreamlineReflexBlueprint, Error, TEXT("%s should not be called before PostEngineInit"), ANSI_TO_TCHAR(__FUNCTION__)); \
	return ReturnValueOrEmptyOrVoidPreFiveThree; \
}

#else

#define TRY_INIT_STREAMLINE_REFLEX_LIBRARY_AND_RETURN(ReturnValueWhichCanBeEmpty) 

#endif


EStreamlineFeatureSupport FStreamlineLibraryImplementationBase::FeatureSupport = EStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime;

#if WITH_STREAMLINE

bool FStreamlineLibraryImplementationBase::bIsLibraryInitialized = false;
bool FStreamlineLibraryImplementationBase::TryInitLibrary()
{
	if (bIsLibraryInitialized)
	{
		return true;
	}

	if (IsStreamlineSupported())
	{
		if (GetPlatformStreamlineRHI()->IsReflexSupportedByRHI())
		{

			FeatureSupport = ToUStreamlineFeatureSupport(/* TODO generalize this when we use this class elsewhere */QueryStreamlineReflexSupport());
		}
		else
		{
			FeatureSupport = EStreamlineFeatureSupport::NotSupportedByRHI;
		}
	}
	else
	{
		if (GetPlatformStreamlineSupport() == EStreamlineSupport::NotSupportedIncompatibleRHI)
		{
			FeatureSupport = EStreamlineFeatureSupport::NotSupportedByRHI;
		}
		else
		{
			FeatureSupport = EStreamlineFeatureSupport::NotSupported;
		}
	}

	bIsLibraryInitialized = true;

	return true;
}
#endif

bool UStreamlineLibraryReflex::IsReflexSupported()
{

	TRY_INIT_STREAMLINE_REFLEX_LIBRARY_AND_RETURN(false);

#if WITH_STREAMLINE
	return QueryReflexSupport() == EStreamlineFeatureSupport::Supported;
#else
	return false;
#endif
}

EStreamlineFeatureSupport UStreamlineLibraryReflex::QueryReflexSupport()
{
#if WITH_STREAMLINE
	
	bool bIsMaxTickRateHandlerEnabled = false;
	
	TArray<IMaxTickRateHandlerModule*> MaxTickRateHandlerModules = IModularFeatures::Get()
		.GetModularFeatureImplementations<IMaxTickRateHandlerModule>(IMaxTickRateHandlerModule::GetModularFeatureName());
	for (IMaxTickRateHandlerModule* MaxTickRateHandler : MaxTickRateHandlerModules)
	{
		if (MaxTickRateHandler != GetStreamlineReflexMaxTickRateHandler())
		{
			continue;
		}

		bIsMaxTickRateHandlerEnabled = bIsMaxTickRateHandlerEnabled || MaxTickRateHandler->GetAvailable();
	}
	
	bool bIsLatencyMarkerModuleEnabled = false;
	
	TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get()
		.GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
	for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
	{
		if (LatencyMarkerModule != GetStreamlineReflexLatencyMarkerModule())
		{
			continue;
		}

		bIsLatencyMarkerModuleEnabled = bIsLatencyMarkerModuleEnabled || LatencyMarkerModule->GetAvailable();
	}
	
	return bIsMaxTickRateHandlerEnabled && bIsLatencyMarkerModuleEnabled ? EStreamlineFeatureSupport::Supported : EStreamlineFeatureSupport::NotSupported;
#endif
	return EStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime;
}

#if WITH_STREAMLINE
static EStreamlineReflexMode ReflexModeEnumFromFlags(int32 ReflexFlags)
{

	static_assert(uint32(FStreamlineMaxTickRateHandler::ReflexFlags::ReflexOff) == uint32(EStreamlineReflexMode::Off), "enum value mismatch. Dear NVIDIA Streamline plugin developer, please update this code!");
	static_assert(uint32(FStreamlineMaxTickRateHandler::ReflexFlags::ReflexOn) == uint32(EStreamlineReflexMode::Enabled), "enum value mismatch. Dear NVIDIA Streamline plugin developer, please update this code!");
	static_assert(uint32(FStreamlineMaxTickRateHandler::ReflexFlags::ReflexBoost) == uint32(EStreamlineReflexMode::Boost), "enum value mismatch. Dear NVIDIA Streamline plugin developer, please update this code!");

	if ((uint32(FStreamlineMaxTickRateHandler::ReflexFlags::AllBits) & ReflexFlags) != uint32(FStreamlineMaxTickRateHandler::ReflexFlags::ReflexInvalidFlags))
	{
		return static_cast<EStreamlineReflexMode>(ReflexFlags);
	}
	else
	{
		UE_LOG(LogStreamlineReflexBlueprint, Error, TEXT("Invalid t.Streamline.Reflex.Mode and t.Streamline.Reflex.Auto vars yielding invalid GetFlags %d. turning Reflex off"), ReflexFlags);
		return EStreamlineReflexMode::Off;
	}

}
#endif

void UStreamlineLibraryReflex::SetReflexMode(const EStreamlineReflexMode Mode)
{
#if WITH_STREAMLINE
	if (ValidateEnumValue(Mode, __FUNCTION__))
	{
		TArray<IMaxTickRateHandlerModule*> MaxTickRateHandlerModules = IModularFeatures::Get()
			.GetModularFeatureImplementations<IMaxTickRateHandlerModule>(IMaxTickRateHandlerModule::GetModularFeatureName());
		for (IMaxTickRateHandlerModule* MaxTickRateHandler : MaxTickRateHandlerModules)
		{
			if (MaxTickRateHandler != GetStreamlineReflexMaxTickRateHandler())
			{
				continue;
			}

			MaxTickRateHandler->SetEnabled(Mode != EStreamlineReflexMode::Off);
			MaxTickRateHandler->SetFlags(static_cast<uint32>(Mode));
		}
	}
#endif
}

EStreamlineReflexMode UStreamlineLibraryReflex::GetReflexMode()
{
#if WITH_STREAMLINE
	TArray<IMaxTickRateHandlerModule*> MaxTickRateHandlerModules = IModularFeatures::Get()
		.GetModularFeatureImplementations<IMaxTickRateHandlerModule>(IMaxTickRateHandlerModule::GetModularFeatureName());
	for (IMaxTickRateHandlerModule* MaxTickRateHandler : MaxTickRateHandlerModules)
	{
		if (MaxTickRateHandler != GetStreamlineReflexMaxTickRateHandler())
		{
			continue;
		}

		if (MaxTickRateHandler->GetEnabled())
		{
			return ReflexModeEnumFromFlags(MaxTickRateHandler->GetFlags());
		}
	}
#endif
	return EStreamlineReflexMode::Off;
}


bool UStreamlineLibraryReflex::IsReflexModeSupported(EStreamlineReflexMode ReflexMode)
{
	TRY_INIT_STREAMLINE_REFLEX_LIBRARY_AND_RETURN(false);

	if (ValidateEnumValue(ReflexMode, __FUNCTION__))
	{
		if (ReflexMode == EStreamlineReflexMode::Off)
		{
			return true;
		}

		if (!IsReflexSupported()) // that returns false if WITH_STREAMLINE is false 
		{
			return false;
		}

		return true; // TODO, right now Enabled and Boost
	}

	return false;
}

TArray<EStreamlineReflexMode> UStreamlineLibraryReflex::GetSupportedReflexModes()
{
	TArray<EStreamlineReflexMode> SupportedQualityModes;

	TRY_INIT_STREAMLINE_REFLEX_LIBRARY_AND_RETURN(SupportedQualityModes);

	const UEnum* Enum = StaticEnum<EStreamlineReflexMode>();
	for (int32 EnumIndex = 0; EnumIndex < Enum->NumEnums() - 1 /* avoid _MAX */; ++EnumIndex)
	{
		const int64 EnumValue = Enum->GetValueByIndex(EnumIndex);
		const EStreamlineReflexMode QualityMode = EStreamlineReflexMode(EnumValue);
		if (IsReflexModeSupported(QualityMode))
		{
			SupportedQualityModes.Add(QualityMode);
		}
	}

	return SupportedQualityModes;
}

EStreamlineReflexMode UStreamlineLibraryReflex::GetDefaultReflexMode()
{
	TRY_INIT_STREAMLINE_REFLEX_LIBRARY_AND_RETURN(EStreamlineReflexMode::Off);

	if (UStreamlineLibraryReflex::IsReflexSupported())
	{
		return EStreamlineReflexMode::Enabled;
	}
	else
	{
		return EStreamlineReflexMode::Off;
	}
}

float UStreamlineLibraryReflex::GetGameToRenderLatencyInMs()
{
#if WITH_STREAMLINE
	TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get()
		.GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
	for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
	{
		if (LatencyMarkerModule != GetStreamlineReflexLatencyMarkerModule())
		{
			continue;
		}

		return LatencyMarkerModule->GetTotalLatencyInMs();
	}
#endif
	return 0.f;
}

float UStreamlineLibraryReflex::GetGameLatencyInMs()
{
#if WITH_STREAMLINE
	TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get()
		.GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
	for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
	{
		if (LatencyMarkerModule != GetStreamlineReflexLatencyMarkerModule())
		{
			continue;
		}

		return LatencyMarkerModule->GetGameLatencyInMs();
	}
#endif
	return 0.f;
}

float UStreamlineLibraryReflex::GetRenderLatencyInMs()
{
#if WITH_STREAMLINE
	TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get()
		.GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
	for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
	{
		if (LatencyMarkerModule != GetStreamlineReflexLatencyMarkerModule())
		{
				continue;
		}

		return LatencyMarkerModule->GetRenderLatencyInMs();
	}
#endif
	return 0.f;
}

void UStreamlineLibraryReflex::Startup()
{
#if WITH_STREAMLINE
	UStreamlineLibrary::RegisterFeatureSupport(EStreamlineFeature::Reflex, UStreamlineLibraryReflex::QueryReflexSupport());
#endif
}

void UStreamlineLibraryReflex::Shutdown()
{
#if WITH_STREAMLINE

#endif
}

void FStreamlineLibraryReflexBlueprintModule::StartupModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool())
	{
		UE_LOG(LogStreamlineReflexBlueprint, Log, TEXT("Initialization of StreamlineBlueprint is disabled."));
		return;
	}

	UStreamlineLibraryReflex::Startup();
}

void FStreamlineLibraryReflexBlueprintModule::ShutdownModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool())
	{
		return;
	}

	UStreamlineLibraryReflex::Shutdown();
}

IMPLEMENT_MODULE(FStreamlineLibraryReflexBlueprintModule, StreamlineReflexBlueprint)

#undef LOCTEXT_NAMESPACE