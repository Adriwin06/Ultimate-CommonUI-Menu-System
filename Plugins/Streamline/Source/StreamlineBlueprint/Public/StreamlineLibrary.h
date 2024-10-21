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
#pragma once
#include "CoreMinimal.h"
#include "Containers/StaticArray.h"
#include "Modules/ModuleManager.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StreamlineLibrary.generated.h"

//namespace sl
//{
//	struct Version;
//}

// That should be updated if new BP libraries are added for new featurew
#define STREAMLINE_LIBARY_KEYWORDS "DLSS-G, Reflex, DeepDVC, Streamline"
UENUM(BlueprintType)
enum class UStreamlineFeature : uint8
{
	DLSSG UMETA(DisplayName = "DLSS Frame Generation"),
	Reflex UMETA(DisplayName = "Reflex"),
	DeepDVC UMETA(DisplayName = "DeepDVC"),
	Count UMETA(Hidden)
};


UENUM(BlueprintType)
enum class UStreamlineFeatureSupport : uint8
{
	Supported UMETA(DisplayName = "Supported"),

	NotSupported UMETA(DisplayName = "Not Supported"),
	NotSupportedIncompatibleHardware UMETA(DisplayName = "Incompatible Hardware", ToolTip = "This feature requires an NVIDIA RTX GPU"),
	NotSupportedDriverOutOfDate UMETA(DisplayName = "Driver Out of Date", ToolTip = "The driver is outdated. Also see GetStreamlineFeatureGMinimumDriverVersion"),
	NotSupportedOperatingSystemOutOfDate UMETA(DisplayName = "Operating System Out of Date", ToolTip = "The Operating System is outdated. Also see GetStreamlineFeatureMinimumOperatingSystemVersion"),
	NotSupportedHardewareSchedulingDisabled UMETA(DisplayName = "Hardware Scheduling Disabled", ToolTip = "This feature requires Windows Hardware Scheduling to be Enabled"),
	NotSupportedByRHI UMETA(DisplayName = "Not supported by RHI", ToolTip = "This RHI doesn't not support this feature run time."),
	NotSupportedByPlatformAtBuildTime UMETA(DisplayName = "Platform Not Supported At Build Time", ToolTip = "This platform doesn't not support this feature at build time. Currently this feature is only supported on Windows 64"),
	NotSupportedIncompatibleAPICaptureToolActive UMETA(DisplayName = "Incompatible API Capture Tool Active", ToolTip = "This feature is not compatible with an active API capture tool such as RenderDoc.")
};



UENUM(BlueprintType, meta = (Bitflags))
enum class UStreamlineFeatureRequirementsFlags : uint8
{
	None = 0,
	
	D3D11Supported = 1 << 0,
	D3D12Supported = 1 << 1,
	VulkanSupported = 1 << 2,

	VSyncOffRequired = 1 << 3,
	HardwareSchedulingRequired = 1 << 4
};
ENUM_CLASS_FLAGS(UStreamlineFeatureRequirementsFlags)

USTRUCT(BlueprintType)
struct FStreamlineVersion
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, Category = "Streamline")
	int32 Major = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Streamline")
	int32 Minor = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Streamline")
	int32 Build = 0;

};

static_assert(uint8(UStreamlineFeature::Count) == 3u, "dear NVIDIA plugin developer, please update the Keywords below handle the new enum values");

USTRUCT(BlueprintType)
struct FStreamlineFeatureRequirements 
{
	GENERATED_BODY()
public:
	
	UPROPERTY(BlueprintReadWrite, Category = "Streamline")
	UStreamlineFeatureSupport Support = UStreamlineFeatureSupport::NotSupportedByPlatformAtBuildTime;
	
	UPROPERTY(BlueprintReadWrite, Category = "Streamline")
	UStreamlineFeatureRequirementsFlags Requirements = UStreamlineFeatureRequirementsFlags::None;

	UPROPERTY(BlueprintReadWrite, Category = "Streamline")
	FStreamlineVersion RequiredOperatingSystemVersion;
	UPROPERTY(BlueprintReadWrite, Category = "Streamline")
	FStreamlineVersion DetectedOperatingSystemVersion;

	UPROPERTY(BlueprintReadWrite, Category = "Streamline")
	FStreamlineVersion RequiredDriverVersion;
	UPROPERTY(BlueprintReadWrite, Category = "Streamline")
	FStreamlineVersion DetectedDriverVersion;
};


UCLASS(MinimalAPI)
class  UStreamlineLibrary : public UBlueprintFunctionLibrary
{
	friend class FStreamlineBlueprintModule;
	GENERATED_BODY()
public:

		/** Checks whether a Streamline feature is supported by the current GPU. Further details can be retrieved via QueryStreamlineFeatureSupport*/
	UFUNCTION(BlueprintPure, Category = "Streamline", meta = (DisplayName = "Get NVIDIA Streamline Feature information", Keywords = "Reflex, DLSS-G, DeepDVC"))
	static STREAMLINEBLUEPRINT_API FStreamlineFeatureRequirements GetStreamlineFeatureInformation(UStreamlineFeature Feature);

	UFUNCTION(BlueprintPure, Category = "Streamline", meta = (/*DisplayName = "Get Streamline Feature Requirements", */Keywords = "Reflex, DLSS-G, DeepDVC"))
	static STREAMLINEBLUEPRINT_API void BreakStreamlineFeatureRequirements(UStreamlineFeatureRequirementsFlags Requirements, bool& D3D11Supported, bool& D3D12Supported, bool& VulkanSupported, bool& VSyncOffRequired, bool& HardwareSchedulingRequired);

	/** Checks whether a Streamline feature is supported by the current GPU. Further details can be retrieved via QueryStreamlineFeatureSupport*/
	UFUNCTION(BlueprintPure, Category = "Streamline", meta = (DisplayName = "Is NVIDIA Streamline Feature Supported", Keywords = "Reflex, DLSS-G, DeepDVC" ))
	static STREAMLINEBLUEPRINT_API bool IsStreamlineFeatureSupported(UStreamlineFeature Feature);

	/** Checks whether Streamline feature  is supported by the current GPU	*/
	UFUNCTION(BlueprintPure, Category = "Streamline", meta = (DisplayName = "Query NVIDIA Streamline Feature Support", Keywords = "Reflex, DLSS-G, DeepDVC"))
	static STREAMLINEBLUEPRINT_API UStreamlineFeatureSupport QueryStreamlineFeatureSupport(UStreamlineFeature Feature);


	static STREAMLINEBLUEPRINT_API void RegisterFeatureSupport(UStreamlineFeature Feature, UStreamlineFeatureSupport Support);

protected:

	static void Startup();
	static void Shutdown();

private:

	
	static TStaticArray<FStreamlineFeatureRequirements, static_cast<uint8>(UStreamlineFeature::Count)> Features;
	
	static int32 ValidateAndConvertToIndex(UStreamlineFeature Feature);


	static bool bStreamlineLibraryInitialized;
	static bool TryInitStreamlineLibrary();
};


template <typename UE>
bool ValidateEnumValue(UE Value, const char* CallSite)
{
	// UEnums are strongly typed, but then one can also cast a byte to an UEnum ...
	const UEnum* Enum = StaticEnum<UE>();
	const bool bIsValid = Enum->IsValidEnumValue(int64(Value)) && (Enum->GetMaxEnumValue() != int64(Value));

#if !UE_BUILD_SHIPPING
	if (!bIsValid)
	{
		FFrame::KismetExecutionMessage(*FString::Printf(
			TEXT("%s should not be called with an invalid enum value (%d) \"%s\""),
			ANSI_TO_TCHAR(CallSite), int64(Value), *Enum->GetDisplayNameTextByValue(int64(Value)).ToString()),
			ELogVerbosity::Error);
	}
#endif 
	return bIsValid;
}

// TODO maybe move inter SL plugin stuff into a separate header?
#if WITH_STREAMLINE
#include "StreamlineCore.h"
STREAMLINEBLUEPRINT_API UStreamlineFeatureSupport ToUStreamlineFeatureSupport(EStreamlineFeatureSupport Support);
#endif


class FStreamlineBlueprintModule final : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};
