/*******************************************************************************
 * Copyright 2021 Intel Corporation
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

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "XeSSBlueprintLibrary.generated.h"

class FXeSSModule;
class FXeSSRHI;
class FXeSSUpscaler;

// QUALITY EDIT:
UENUM(BlueprintType)
enum class EXeSSQualityMode : uint8
{
	Off					UMETA(DisplayName = "Off"),
	UltraPerformance	UMETA(DisplayName = "Ultra Performance"),
	Performance			UMETA(DisplayName = "Performance"),
	Balanced			UMETA(DisplayName = "Balanced"),
	Quality				UMETA(DisplayName = "Quality"),
	UltraQuality		UMETA(DisplayName = "Ultra Quality"),
	UltraQualityPlus	UMETA(DisplayName = "Ultra Quality Plus"),
	AntiAliasing		UMETA(DisplayName = "Anti-Aliasing"),
};

UCLASS()
class  UXeSSBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void Init(FXeSSModule* InXeSSModule);
	static void Deinit();

	/** Check if Intel XeSS-SR is supported on the current Platform */
	UFUNCTION(BlueprintPure, Category = "XeSS-SR", meta = (DisplayName = "Is Intel(R) XeSS-SR Supported"))
	static XESSBLUEPRINT_API bool IsXeSSSupported();

	/** List all available Intel XeSS-SR quality modes */
	UFUNCTION(BlueprintPure, Category = "XeSS-SR", meta = (DisplayName = "Get Supported Intel(R) XeSS-SR Quality Modes"))
	static XESSBLUEPRINT_API TArray<EXeSSQualityMode> GetSupportedXeSSQualityModes();

	/** Get the current Intel XeSS-SR quality mode */
	UFUNCTION(BlueprintPure, Category = "XeSS-SR", meta = (DisplayName = "Get Current Intel(R) XeSS-SR Quality Mode"))
	static XESSBLUEPRINT_API EXeSSQualityMode GetXeSSQualityMode();

	/** Get the default Intel XeSS-SR quality mode for the given screen resolution */
	UFUNCTION(BlueprintPure, Category = "XeSS-SR", meta = (DisplayName = "Get Default Intel(R) XeSS-SR Quality Mode"))
	static XESSBLUEPRINT_API EXeSSQualityMode GetDefaultXeSSQualityMode(FIntPoint ScreenResolution);

	/** Set the selected Intel XeSS-SR quality mode */
	UFUNCTION(BlueprintCallable, Category = "XeSS-SR", meta = (DisplayName = "Set Intel(R) XeSS-SR Quality Mode"))
	static XESSBLUEPRINT_API void SetXeSSQualityMode(EXeSSQualityMode QualityMode);

	/** Get Intel XeSS-SR quality mode information, e.g., screen percentage */
	UFUNCTION(BlueprintCallable, Category = "XeSS-SR", meta = (DisplayName = "Get Intel(R) XeSS-SR Quality Mode Information"))
	static XESSBLUEPRINT_API bool GetXeSSQualityModeInformation(EXeSSQualityMode QualityMode, float& ScreenPercentage);

private:
	static bool bInitialized;
	static bool bIsXeSSSupported;
	static FXeSSRHI* XeSSRHI;
	static FXeSSUpscaler* XeSSUpscaler;
};
