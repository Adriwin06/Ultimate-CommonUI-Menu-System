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

#include "Modules/ModuleManager.h"

#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "StreamlineLibrary.h"


#include "StreamlineLibraryReflex.generated.h"


#ifdef __INTELLISENSE__
#define WITH_STREAMLINE 1
#endif

UENUM(BlueprintType)
enum class EStreamlineReflexMode : uint8
{
	Off = 0 UMETA(DisplayName = "Off"),
	Enabled = 1 UMETA(DisplayName = "Enabled"),
	Boost = 3 UMETA(DisplayName = "Boost")
};

// TODO, eventually also use on the other BP libraries
class FStreamlineLibraryImplementationBase
{
	protected:

	static EStreamlineFeatureSupport FeatureSupport;


#if WITH_STREAMLINE
	static bool bIsLibraryInitialized;
	static bool TryInitLibrary();
#endif
};


UCLASS(MinimalAPI)
class UStreamlineLibraryReflex : public UBlueprintFunctionLibrary, 
	public FStreamlineLibraryImplementationBase
{
public:
	GENERATED_BODY()


	UFUNCTION(BlueprintPure, Category = "Streamline|Reflex", meta = (DisplayName = "Is NVIDIA Reflex Supported"))
	static STREAMLINEREFLEXBLUEPRINT_API bool IsReflexSupported();

	UFUNCTION(BlueprintPure, Category = "Streamline|Reflex", meta = (DisplayName = "Query NVIDIA Reflex Support"))
	static STREAMLINEREFLEXBLUEPRINT_API EStreamlineFeatureSupport QueryReflexSupport();


	UFUNCTION(BlueprintCallable, Category = "Streamline|Reflex", meta = (DisplayName = "Set Reflex mode"))
	static STREAMLINEREFLEXBLUEPRINT_API void SetReflexMode(const EStreamlineReflexMode Mode);
	
	UFUNCTION(BlueprintPure, Category = "Streamline|Reflex", meta = (DisplayName = "Get Reflex mode"))
	static STREAMLINEREFLEXBLUEPRINT_API EStreamlineReflexMode GetReflexMode();

	/** Checks whether a Reflex mode is supported */
	UFUNCTION(BlueprintPure, Category = "Streamline|Reflex", meta = (DisplayName = "Is Reflex Mode Supported"))
	static STREAMLINEREFLEXBLUEPRINT_API bool IsReflexModeSupported(EStreamlineReflexMode ReflexMode);
	/** Retrieves all supported Reflex modes. Can be used to populate UI */
	UFUNCTION(BlueprintPure, Category = "Streamline|Reflex", meta = (DisplayName = "Get Supported Reflex Modes"))
	static STREAMLINEREFLEXBLUEPRINT_API TArray<EStreamlineReflexMode> GetSupportedReflexModes();

	UFUNCTION(BlueprintPure, Category = "Streamline|Reflex", meta = (DisplayName = "Get default Reflex mode"))
	static STREAMLINEREFLEXBLUEPRINT_API EStreamlineReflexMode GetDefaultReflexMode();

	UFUNCTION(BlueprintPure, Category = "Streamline|Reflex", meta = (DisplayName = "Get Reflex Game To Render Latency (ms)"))
	static STREAMLINEREFLEXBLUEPRINT_API float GetGameToRenderLatencyInMs();
	UFUNCTION(BlueprintPure, Category = "Streamline|Reflex", meta = (DisplayName = "Get Reflex Game Latency (ms)"))
	static STREAMLINEREFLEXBLUEPRINT_API float GetGameLatencyInMs();
	UFUNCTION(BlueprintPure, Category = "Streamline|Reflex", meta = (DisplayName = "Get Reflex Render Latency (ms)"))
	static STREAMLINEREFLEXBLUEPRINT_API float GetRenderLatencyInMs();


	static void Startup();
	static void Shutdown();
};

class FStreamlineLibraryReflexBlueprintModule final : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};
