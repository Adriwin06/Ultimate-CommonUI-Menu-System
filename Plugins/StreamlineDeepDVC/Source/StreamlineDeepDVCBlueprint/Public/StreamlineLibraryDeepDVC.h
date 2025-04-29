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

#include "StreamlineLibraryDeepDVC.generated.h"



class FDelegateHandle;

#ifdef __INTELLISENSE__
#define WITH_STREAMLINE 1
#endif

UENUM(BlueprintType)
enum class EStreamlineDeepDVCMode : uint8
{
	Off UMETA(DisplayName = "Off"),
	On UMETA(DisplayName = "On"),
};


UCLASS(MinimalAPI)
class  UStreamlineLibraryDeepDVC : public UBlueprintFunctionLibrary
{
	friend class FStreamlineBlueprintModule;
	GENERATED_BODY()
public:

	/** Checks whether DeepDVC is supported by the current GPU. Further details can be retrieved via QueryDeepDVCSupport*/
	UFUNCTION(BlueprintPure, Category = "Streamline|DeepDVC", meta = (DisplayName = "Is NVIDIA DeepDVC Supported"))
	static STREAMLINEDEEPDVCBLUEPRINT_API bool IsDeepDVCSupported();

	/** Checks whether DeepDVC is supported by the current GPU	*/
	UFUNCTION(BlueprintPure, Category = "Streamline|DeepDVC", meta = (DisplayName = "Query NVIDIA DeepDVC Support"))
	static STREAMLINEDEEPDVCBLUEPRINT_API EStreamlineFeatureSupport QueryDeepDVCSupport();

	/** Checks whether a DeepDVC mode is supported */
	UFUNCTION(BlueprintPure, Category = "Streamline|DeepDVC", meta = (DisplayName = "Is DeepDVC Mode Supported"))
	static STREAMLINEDEEPDVCBLUEPRINT_API bool IsDeepDVCModeSupported(EStreamlineDeepDVCMode DeepDVCMode);

	/** Retrieves all supported DeepDVC modes. Can be used to populate UI */
	UFUNCTION(BlueprintPure, Category = "Streamline|DeepDVC", meta = (DisplayName = "Get Supported DeepDVC Modes"))
	static STREAMLINEDEEPDVCBLUEPRINT_API TArray<EStreamlineDeepDVCMode> GetSupportedDeepDVCModes();

	/**
	 * Sets the console variables to enable/disable DeepDVC
	 * Off = DeepDVC disabled
	 * On = DeepDVC always enabled
	 */
	UFUNCTION(BlueprintCallable, Category = "Streamline|DeepDVC", meta = (DisplayName = "Set DeepDVC Mode"))
	static STREAMLINEDEEPDVCBLUEPRINT_API void SetDeepDVCMode(EStreamlineDeepDVCMode DeepDVCMode);

	/* Reads the console variables to infer the current DeepDVC mode*/
	UFUNCTION(BlueprintPure, Category = "Streamline|DeepDVC", meta = (DisplayName = "Get DeepDVC Mode"))
	static STREAMLINEDEEPDVCBLUEPRINT_API EStreamlineDeepDVCMode GetDeepDVCMode();

	/* Find a reasonable default DeepDVC mode based on current hardware */
	UFUNCTION(BlueprintPure, Category = "Streamline|DeepDVC", meta = (DisplayName = "Get Default DeepDVC Mode"))
	static STREAMLINEDEEPDVCBLUEPRINT_API EStreamlineDeepDVCMode GetDefaultDeepDVCMode();


	/* Set the console variable to controls how strong or subtle the DeepDVC filter effect will be on an image. A low intensity will keep the images closer to the original, while a high intensity will make the filter effect more pronounced. */
	UFUNCTION(BlueprintCallable, Category = "Streamline|DeepDVC", meta = (DisplayName = "Set DeepDVC Intensity"))
	static STREAMLINEDEEPDVCBLUEPRINT_API void SetDeepDVCIntensity(float Intensity);

	/* Read the console variables to infer the current DeepDVC intensity ("r.Streamline.DeepDVC.Intensity) */
	UFUNCTION(BlueprintPure, Category = "Streamline|DeepDVC", meta = (DisplayName = "Get DeepDVC Intensity"))
	static STREAMLINEDEEPDVCBLUEPRINT_API float GetDeepDVCIntensity();

	/* Set the console variable that enhances the colors in them image, making them more vibrant and eye-catching. This setting will only be active if r.Streamline.DeepDVC.Intensity is relatively high. Once active, colors pop up more, making the image look more lively. */
	UFUNCTION(BlueprintCallable, Category = "Streamline|DeepDVC", meta = (DisplayName = "Set DeepDVC  Saturation Boost"))
	static STREAMLINEDEEPDVCBLUEPRINT_API void SetDeepDVCSaturationBoost(float Intensity);

	/* Read the console variables to infer the current DeepDVC saturation boost ("r.Streamline.DeepDVC.SaturationBoost) */
	UFUNCTION(BlueprintPure, Category = "Streamline|DeepDVC", meta = (DisplayName = "Get DeepDVC Saturation Boost"))
	static STREAMLINEDEEPDVCBLUEPRINT_API float GetDeepDVCSaturationBoost();




	static void Startup();
	static void Shutdown();
private:
	static EStreamlineFeatureSupport DeepDVCSupport;

#if WITH_STREAMLINE


	static bool bDeepDVCLibraryInitialized;



	static bool TryInitDeepDVCLibrary();

#if !UE_BUILD_SHIPPING
	struct FDLSSErrorState
	{
		bool bIsDeepDVCModeUnsupported = false;
		EStreamlineDeepDVCMode InvalidDeepDVCMode = EStreamlineDeepDVCMode::Off;
	};

	static FDLSSErrorState DLSSErrorState;

	static void GetDeepDVCOnScreenMessages(TMultiMap<FCoreDelegates::EOnScreenMessageSeverity, FText>& OutMessages);
	static FDelegateHandle DeepDVCOnScreenMessagesDelegateHandle;
#endif


#endif



};


class FStreamlineLibraryDeepDVCBlueprintModule final : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};


