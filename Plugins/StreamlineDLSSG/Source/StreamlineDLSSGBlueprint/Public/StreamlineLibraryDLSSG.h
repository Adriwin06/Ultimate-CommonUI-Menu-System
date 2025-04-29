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
enum class EStreamlineDLSSGMode : uint8
{
	
	Off  = 0   UMETA(DisplayName = "Off"),
	Auto = 251 UMETA(DisplayName = "Auto"),
	On2X = 17  UMETA(DisplayName = "2X"),
	On3X = 23  UMETA(DisplayName = "3X"),
	On4X = 31  UMETA(DisplayName = "4X"),
};


UCLASS(MinimalAPI)
class  UStreamlineLibraryDLSSG : public UBlueprintFunctionLibrary
{
	friend class FStreamlineBlueprintModule;
	GENERATED_BODY()
public:

	/** Checks whether DLSS-FG is supported by the current GPU. Further details can be retrieved via QueryDLSSGSupport*/
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Is NVIDIA DLSS-FG Supported"))
	static STREAMLINEDLSSGBLUEPRINT_API bool IsDLSSGSupported();

	/** Checks whether DLSS-FG is supported by the current GPU	*/
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Query NVIDIA DLSS-FG Support"))
	static STREAMLINEDLSSGBLUEPRINT_API EStreamlineFeatureSupport QueryDLSSGSupport();

	/** Checks whether a DLSS-FG mode is supported */
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Is DLSS-FG Mode Supported"))
	static STREAMLINEDLSSGBLUEPRINT_API bool IsDLSSGModeSupported(EStreamlineDLSSGMode DLSSGMode);

	/** Retrieves all supported DLSS-FG modes. Can be used to populate UI */
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Get Supported DLSS-FG Modes"))
	static STREAMLINEDLSSGBLUEPRINT_API TArray<EStreamlineDLSSGMode> GetSupportedDLSSGModes();

	/**
	 * Sets the console variables to enable/disable DLSS-FG as well as how many frames are generated. The latter depends on the hardware
	 * Off = DLSS-FG disabled
	 * Auto = DLSS-FG may be temporarily disabled if it could hurt frame rate
	 * On2x = DLSS-FG always enabled, generate 1 frame for each rendered frame
	 * On3x = DLSS-FG always enabled, generate 2 frames for each rendered frame
	 * On4x = DLSS-FG always enabled, generate 3 frames for each rendered frame
	 */
	UFUNCTION(BlueprintCallable, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Set DLSS-FG Mode"))
	static STREAMLINEDLSSGBLUEPRINT_API void SetDLSSGMode(EStreamlineDLSSGMode DLSSGMode);

	/* Reads the console variables to infer the current DLSS-FG mode*/
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Get DLSS-FG Mode"))
	static STREAMLINEDLSSGBLUEPRINT_API EStreamlineDLSSGMode GetDLSSGMode();

	/* Find a reasonable default DLSS-FG mode based on current hardware */
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Get Default DLSS-FG Mode"))
	static STREAMLINEDLSSGBLUEPRINT_API EStreamlineDLSSGMode GetDefaultDLSSGMode();

	/* Returns the actual framerate and number of frames presented, whether DLSS-FG is active or not */
	UFUNCTION(BlueprintPure, Category = "Streamline|DLSS-FG", meta = (DisplayName = "Get DLSS-FG  frame rate and presented frames"))
	static STREAMLINEDLSSGBLUEPRINT_API void GetDLSSGFrameTiming(float& FrameRateInHertz, int32& FramesPresented);

	static void Startup();
	static void Shutdown();
private:
	static EStreamlineFeatureSupport DLSSGSupport;

#if WITH_STREAMLINE


	static bool bDLSSGLibraryInitialized;



	static bool TryInitDLSSGLibrary();

#if !UE_BUILD_SHIPPING
	struct FDLSSErrorState
	{
		bool bIsDLSSGModeUnsupported = false;
		EStreamlineDLSSGMode InvalidDLSSGMode = EStreamlineDLSSGMode::Off;
	};

	static FDLSSErrorState DLSSErrorState;

	static void GetDLSSOnScreenMessages(TMultiMap<FCoreDelegates::EOnScreenMessageSeverity, FText>& OutMessages);
	static FDelegateHandle DLSSOnScreenMessagesDelegateHandle;
#endif




#endif
};

class FStreamlineLibraryDLSSGBlueprintModule final : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};