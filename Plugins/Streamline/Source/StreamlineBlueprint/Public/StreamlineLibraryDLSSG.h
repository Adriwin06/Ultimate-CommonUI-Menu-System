/*
* Copyright (c) 2022 - 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#include "Modules/ModuleManager.h"

#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/CoreDelegates.h"

#include "StreamlineLibrary.h"

#include "StreamlineLibraryDLSSG.generated.h"



class FDelegateHandle;

#ifdef __INTELLISENSE__
#define WITH_STREAMLINE 1
#endif

UENUM(BlueprintType)
enum class UStreamlineDLSSGMode : uint8
{
	Off UMETA(DisplayName = "Off"),
	On UMETA(DisplayName = "On"),
	Auto UMETA(DisplayName = "Auto"),
};


UCLASS(MinimalAPI)
class  UStreamlineLibraryDLSSG : public UBlueprintFunctionLibrary
{
	friend class FStreamlineBlueprintModule;
	GENERATED_BODY()
public:

	/** Checks whether DLSS-FG is supported by the current GPU. Further details can be retrieved via QueryDLSSGSupport*/
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Is NVIDIA DLSS-FG Supported"))
	static STREAMLINEBLUEPRINT_API bool IsDLSSGSupported();

	/** Checks whether DLSS-FG is supported by the current GPU	*/
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Query NVIDIA DLSS-FG Support"))
	static STREAMLINEBLUEPRINT_API UStreamlineFeatureSupport QueryDLSSGSupport();

	/** Checks whether a DLSS-FG mode is supported */
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Is DLSS-FG Mode Supported"))
	static STREAMLINEBLUEPRINT_API bool IsDLSSGModeSupported(UStreamlineDLSSGMode DLSSGMode);

	/** Retrieves all supported DLSS-FG modes. Can be used to populate UI */
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Get Supported DLSS-FG Modes"))
	static STREAMLINEBLUEPRINT_API TArray<UStreamlineDLSSGMode> GetSupportedDLSSGModes();

	/**
	 * Sets the console variables to enable/disable DLSS-FG
	 * Off = DLSS-FG disabled
	 * On = DLSS-FG always enabled
	 * Auto = DLSS-FG may be temporarily disabled if it could hurt frame rate
	 */
	UFUNCTION(BlueprintCallable, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Set DLSS-FG Mode"))
	static STREAMLINEBLUEPRINT_API void SetDLSSGMode(UStreamlineDLSSGMode DLSSGMode);

	/* Reads the console variables to infer the current DLSS-FG mode*/
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Get DLSS-FG Mode"))
	static STREAMLINEBLUEPRINT_API UStreamlineDLSSGMode GetDLSSGMode();

	/* Find a reasonable default DLSS-FG mode based on current hardware */
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Get Default DLSS-FG Mode"))
	static STREAMLINEBLUEPRINT_API UStreamlineDLSSGMode GetDefaultDLSSGMode();

	/* Returns the actual framerate and number of frames presented, whether DLSS-FG is active or not */
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Get DLSS-FG frame frame rate and presented frames"))
	static STREAMLINEBLUEPRINT_API void GetDLSSGFrameTiming(float& FrameRateInHertz, int32& FramesPresented);


	static void Startup();
	static void Shutdown();
private:
	static UStreamlineFeatureSupport DLSSGSupport;

#if WITH_STREAMLINE


	static bool bDLSSGLibraryInitialized;



	static bool TryInitDLSSGLibrary();

#if !UE_BUILD_SHIPPING
	struct FDLSSErrorState
	{
		bool bIsDLSSGModeUnsupported = false;
		UStreamlineDLSSGMode InvalidDLSSGMode = UStreamlineDLSSGMode::Off;
	};

	static FDLSSErrorState DLSSErrorState;

	static void GetDLSSOnScreenMessages(TMultiMap<FCoreDelegates::EOnScreenMessageSeverity, FText>& OutMessages);
	static FDelegateHandle DLSSOnScreenMessagesDelegateHandle;
#endif




#endif
};

