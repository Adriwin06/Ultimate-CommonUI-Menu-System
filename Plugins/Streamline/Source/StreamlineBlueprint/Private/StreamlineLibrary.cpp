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
#include "StreamlineLibrary.h"
#include "StreamlineLibraryPrivate.h"
#include "StreamlineLibraryReflex.h"
#include "StreamlineLibraryDLSSG.h"
//#include "StreamlineLibraryDeepDVC.h"

#include "HAL/IConsoleManager.h"

#if WITH_STREAMLINE

#include "StreamlineCore.h"

#include "StreamlineDLSSG.h"
#include "StreamlineReflex.h"
#include "StreamlineDeepDVC.h"

#include "StreamlineRHI.h"
#include "StreamlineAPI.h"

#include "sl.h"
#include "sl_dlss_g.h"
#include "sl_reflex.h"
#include "sl_deepdvc.h"
#endif

#define LOCTEXT_NAMESPACE "FStreamlineBlueprintModule"
DEFINE_LOG_CATEGORY(LogStreamlineBlueprint);

TStaticArray<FStreamlineFeatureRequirements, static_cast<uint8>(UStreamlineFeature::Count)> UStreamlineLibrary::Features;
bool UStreamlineLibrary::bStreamlineLibraryInitialized = false;


#if WITH_STREAMLINE
UStreamlineFeatureSupport ToUStreamlineFeatureSupport(EStreamlineFeatureSupport Support)
{
	static_assert(int32(EStreamlineFeatureSupport::NumValues) == 7, "dear NVIDIA plugin developer, please update this code to handle the new enum values ");

	switch (Support)
	{
		case EStreamlineFeatureSupport::Supported: return UStreamlineFeatureSupport::Supported;

		default:
			/* Gotta catch them all*/
		case EStreamlineFeatureSupport::NotSupported: return UStreamlineFeatureSupport::NotSupported;

		case EStreamlineFeatureSupport::NotSupportedIncompatibleHardware: return UStreamlineFeatureSupport::NotSupportedIncompatibleHardware;
		case EStreamlineFeatureSupport::NotSupportedDriverOutOfDate: return UStreamlineFeatureSupport::NotSupportedDriverOutOfDate;
		case EStreamlineFeatureSupport::NotSupportedOperatingSystemOutOfDate: return UStreamlineFeatureSupport::NotSupportedOperatingSystemOutOfDate;
		case EStreamlineFeatureSupport::NotSupportedHardwareSchedulingDisabled: return UStreamlineFeatureSupport::NotSupportedHardewareSchedulingDisabled;
		case EStreamlineFeatureSupport::NotSupportedIncompatibleRHI: return UStreamlineFeatureSupport::NotSupportedByRHI;
	}
}

namespace
{

	FStreamlineVersion FromStreamlineVersion(const sl::Version& SLVersion)
	{
		return FStreamlineVersion{ static_cast<int32>(SLVersion.major), static_cast<int32>(SLVersion.minor), static_cast<int32>(SLVersion.build) };
	}

	uint32 FromUStreamlineFeature(UStreamlineFeature InFeature)
	{
		static_assert(int32(UStreamlineFeature::Count) == 3, "dear NVIDIA plugin developer, please update this code to handle the new enum values ");

		switch (InFeature)
		{
		case UStreamlineFeature::DLSSG: return sl::kFeatureDLSS_G;
		case UStreamlineFeature::Reflex: return sl::kFeatureReflex;
		case UStreamlineFeature::DeepDVC: return sl::kFeatureDeepDVC;
		default:

			return 0;
		}
	}
}
#endif

int32 UStreamlineLibrary::ValidateAndConvertToIndex(UStreamlineFeature Feature)
{
	const int32 FeatureInt = static_cast<int32>(Feature);

	if (FeatureInt < UStreamlineLibrary::Features.Num())
	{
		return FeatureInt;
	}

	else
	{
		return 0;
	}
}


void UStreamlineLibrary::BreakStreamlineFeatureRequirements(UStreamlineFeatureRequirementsFlags Requirements, bool& D3D11Supported, bool& D3D12Supported, bool& VulkanSupported, bool& VSyncOffRequired, bool& HardwareSchedulingRequired)
{
	if (ValidateEnumValue(Requirements, __FUNCTION__))
	{
		D3D11Supported = EnumHasAllFlags(Requirements, UStreamlineFeatureRequirementsFlags::D3D11Supported);
		D3D12Supported = EnumHasAllFlags(Requirements, UStreamlineFeatureRequirementsFlags::D3D12Supported);
		VulkanSupported = EnumHasAllFlags(Requirements, UStreamlineFeatureRequirementsFlags::VulkanSupported);
		VSyncOffRequired = EnumHasAllFlags(Requirements, UStreamlineFeatureRequirementsFlags::VSyncOffRequired);
		HardwareSchedulingRequired = EnumHasAllFlags(Requirements, UStreamlineFeatureRequirementsFlags::HardwareSchedulingRequired);
	}
}

FStreamlineFeatureRequirements UStreamlineLibrary::GetStreamlineFeatureInformation(UStreamlineFeature Feature)
{
	if (ValidateEnumValue(Feature, __FUNCTION__))
	{
		return Features[ValidateAndConvertToIndex(Feature)];
	}

	return FStreamlineFeatureRequirements();
}

bool UStreamlineLibrary::IsStreamlineFeatureSupported(UStreamlineFeature Feature)
{
	TRY_INIT_STREAMLINE_LIBRARY_AND_RETURN(false)

	if (ValidateEnumValue(Feature, __FUNCTION__))
	{
		return QueryStreamlineFeatureSupport(Feature) == UStreamlineFeatureSupport::Supported;
	}

	return false;
}


UStreamlineFeatureSupport UStreamlineLibrary::QueryStreamlineFeatureSupport(UStreamlineFeature Feature)
{
	TRY_INIT_STREAMLINE_LIBRARY_AND_RETURN(UStreamlineFeatureSupport::NotSupported)

	if (ValidateEnumValue(Feature, __FUNCTION__))
	{
		return Features[ValidateAndConvertToIndex(Feature)].Support;
	}

	return UStreamlineFeatureSupport::NotSupported;
}

void UStreamlineLibrary::Startup()
{
#if WITH_STREAMLINE
	// This initialization will likely not succeed unless this module has been moved to PostEngineInit, and that's ok
	TryInitStreamlineLibrary();
#else
	UE_LOG(LogStreamlineBlueprint, Log, TEXT("Streamline is not supported on this platform at build time. The Streamline Blueprint library however is supported and stubbed out to ignore any calls to enable Streamline features and will always return UStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime, regardless of the underlying hardware. This can be used to e.g. to turn off related UI elements."));
#endif
}
void UStreamlineLibrary::Shutdown()
{
#if WITH_STREAMLINE && !UE_BUILD_SHIPPING
	
#endif
}

void UStreamlineLibrary::RegisterFeatureSupport(UStreamlineFeature InFeature, UStreamlineFeatureSupport InSupport)
{
#if WITH_STREAMLINE
	sl::Feature SLFeature = FromUStreamlineFeature(InFeature);
	FStreamlineFeatureRequirements& Requirements = Features[ValidateAndConvertToIndex(InFeature)];
	if (IsStreamlineSupported())
	{

		sl::FeatureRequirements SLRequirements;
		SLgetFeatureRequirements(SLFeature, SLRequirements);

		Requirements.RequiredDriverVersion = FromStreamlineVersion(SLRequirements.driverVersionRequired);
		Requirements.DetectedDriverVersion = FromStreamlineVersion(SLRequirements.driverVersionDetected);
		Requirements.RequiredOperatingSystemVersion = FromStreamlineVersion(SLRequirements.osVersionRequired);
		Requirements.DetectedOperatingSystemVersion = FromStreamlineVersion(SLRequirements.osVersionDetected);

		// static_assert and static_cast are best friends
#define UE_SL_ENUM_CHECK(A,B) static_assert(uint32(sl::FeatureRequirementFlags::A) == uint32(UStreamlineFeatureRequirementsFlags::B), "sl::FeatureRequirementFlags vs UStreamlineFeatureRequirementsFlags enum mismatch");
		UE_SL_ENUM_CHECK(eD3D11Supported, D3D11Supported)
			UE_SL_ENUM_CHECK(eD3D12Supported, D3D12Supported)
			UE_SL_ENUM_CHECK(eVulkanSupported, VulkanSupported)
			UE_SL_ENUM_CHECK(eVSyncOffRequired, VSyncOffRequired)
			UE_SL_ENUM_CHECK(eHardwareSchedulingRequired, HardwareSchedulingRequired)
#undef UE_SL_ENUM_CHECK

		// strip the API support bits for those that are not implemented, but keep the other flags intact
		const sl::FeatureRequirementFlags ImplementedAPIFlags = PlatformGetAllImplementedStreamlineRHIs();
		const sl::FeatureRequirementFlags AllAPIFlags = sl::FeatureRequirementFlags::eD3D11Supported | sl::FeatureRequirementFlags::eD3D12Supported | sl::FeatureRequirementFlags::eVulkanSupported;
		const sl::FeatureRequirementFlags SLRequirementFlags = sl::FeatureRequirementFlags(SLBitwiseAnd(SLRequirements.flags, ImplementedAPIFlags) | SLBitwiseAnd(SLRequirements.flags, ~AllAPIFlags));

		Requirements.Requirements = static_cast<UStreamlineFeatureRequirementsFlags>(SLRequirementFlags);
		Requirements.Support = InSupport;

	}
#endif

}

#if WITH_STREAMLINE

// Delayed initialization, which allows this module to be available early so blueprints can be loaded before DLSS is available in PostEngineInit
bool UStreamlineLibrary::TryInitStreamlineLibrary()
{
	if (bStreamlineLibraryInitialized)
	{
		// TODO
		return true;
	}

//	// Register this before we bail out so we can show error messages
//#if !UE_BUILD_SHIPPING
//	if (!DLSSOnScreenMessagesDelegateHandle.IsValid())
//	{
//		DLSSOnScreenMessagesDelegateHandle = FCoreDelegates::OnGetOnScreenMessages.AddStatic(&GetDLSSOnScreenMessages);
//	}
//#endif


	


	bStreamlineLibraryInitialized = true;

	return true;
}
#endif // WITH_STREAMLINE

void FStreamlineBlueprintModule::StartupModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool())
	{
		UE_LOG(LogStreamlineBlueprint, Log, TEXT("Initialization of StreamlineBlueprint is disabled."));
		return;
	}

	UStreamlineLibrary::Startup();
	UStreamlineLibraryReflex::Startup();
	UStreamlineLibraryDLSSG::Startup();
}

void FStreamlineBlueprintModule::ShutdownModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool())
	{
		return;
	}

	UStreamlineLibrary::Shutdown();
	UStreamlineLibraryDLSSG::Shutdown();
	UStreamlineLibraryReflex::Shutdown();
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStreamlineBlueprintModule, StreamlineBlueprint)


