/*******************************************************************************
 * Copyright 2024 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "XeLLBlueprintLibrary.generated.h"

class FXeLLModule;

UENUM(BlueprintType)
enum class EXeLLMode : uint8
{
	Off = 0				UMETA(DisplayName = "Off"),
	On = 1				UMETA(DisplayName = "On"),
};

UCLASS()
class UXeLLBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void Init(FXeLLModule* InXeLLModule);
	static void Deinit();

	/** Check if Intel XeLL is supported on the current platform  */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Is Intel(R) XeLL Supported"))
	static XELLBLUEPRINT_API bool IsXeLLSupported();
	/** List all available Intel XeLL modes */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Supported Intel(R) XeLL Modes"))
	static XELLBLUEPRINT_API TArray<EXeLLMode> GetSupportedXeLLModes();

	/** Set the selected Intel XeLL mode */
	UFUNCTION(BlueprintCallable, Category = "XeLL", meta = (DisplayName = "Set Intel(R) XeLL Mode"))
	static XELLBLUEPRINT_API void SetXeLLMode(const EXeLLMode Mode);
	/** Get the current Intel XeLL mode */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Current Intel(R) XeLL Mode"))
	static XELLBLUEPRINT_API EXeLLMode GetXeLLMode();

	/** Enable or disable the flash indicator */
	UFUNCTION(BlueprintCallable, Category = "XeLL", meta = (DisplayName = "Set Flash Indicator Enabled"))
	static XELLBLUEPRINT_API void SetFlashIndicatorEnabled(const bool bEnabled);
	/** GetCheck if the flash indicator is enabled */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Flash Indicator Enabled"))
	static XELLBLUEPRINT_API bool GetFlashIndicatorEnabled();

	/** Get game to render latency in milliseconds */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Game to Render Latency (ms)"))
	static XELLBLUEPRINT_API float GetGameToRenderLatencyInMs();
	/** Get game latency in milliseconds */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Game Latency (ms)"))
	static XELLBLUEPRINT_API float GetGameLatencyInMs();
	/** Get render latency in milliseconds */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Render Latency (ms)"))
	static XELLBLUEPRINT_API float GetRenderLatencyInMs();

	/** Get simulation latency in milliseconds */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Simulation Latency (ms)"))
	static XELLBLUEPRINT_API float GetSimulationLatencyInMs();
	/** Get render submit latency in milliseconds */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Render Submit Latency (ms)"))
	static XELLBLUEPRINT_API float GetRenderSubmitLatencyInMs();
	/** Get present latency in milliseconds */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Present Latency (ms)"))
	static XELLBLUEPRINT_API float GetPresentLatencyInMs();

	/** Get input latency in milliseconds */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Input Latency (ms)"))
	static XELLBLUEPRINT_API float GetInputLatencyInMs();

	/** Get Latency Mark Enabled */
	UFUNCTION(BlueprintPure, Category = "XeLL", meta = (DisplayName = "Get Latency Mark Enabled"))
	static XELLBLUEPRINT_API bool GetLatencyMarkEnabled();

private:
	static bool bInitialized;
	static FXeLLModule* XeLLModule;
};
