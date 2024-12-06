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

UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "XeSS Blueprint Library"))
class  UXeSSBlueprintLibrary : public UBlueprintFunctionLibrary
{
	friend class FXeSSBlueprint;
	GENERATED_BODY()

public:
	/** Checks if Intel XeSS is supported on the current GPU */
	UFUNCTION(BlueprintPure, Category = "XeSS", meta = (DisplayName = "Is Intel(R) XeSS Supported"))
	static XESSBLUEPRINT_API bool IsXeSSSupported();

	/** Lists all available Intel XeSS quality modes*/
	UFUNCTION(BlueprintPure, Category = "XeSS", meta = (DisplayName = "Get Supported Intel(R) XeSS Quality Modes"))
	static XESSBLUEPRINT_API TArray<EXeSSQualityMode> GetSupportedXeSSQualityModes();

	/** Gets current Intel XeSS quality mode*/
	UFUNCTION(BlueprintPure, Category = "XeSS", meta = (DisplayName = "Get Current Intel(R) XeSS Quality Mode"))
	static XESSBLUEPRINT_API EXeSSQualityMode GetXeSSQualityMode();

	/** Gets the default Intel XeSS quality mode for the given Screen Resolution*/
	UFUNCTION(BlueprintPure, Category = "XeSS", meta = (DisplayName = "Get Default Intel(R) XeSS Quality Mode"))
	static XESSBLUEPRINT_API EXeSSQualityMode GetDefaultXeSSQualityMode(FIntPoint ScreenResolution);

	/** Sets the selected Intel XeSS quality mode*/
	UFUNCTION(BlueprintCallable, Category = "XeSS", meta = (DisplayName = "Set Intel(R) XeSS Quality Mode"))
	static XESSBLUEPRINT_API void SetXeSSQualityMode(EXeSSQualityMode QualityMode);

	/** Gets Intel XeSS quality mode information*/
	UFUNCTION(BlueprintCallable, Category = "XeSS", meta = (DisplayName = "Get Intel(R) XeSS Quality Mode Information"))
	static XESSBLUEPRINT_API bool GetXeSSQualityModeInformation(EXeSSQualityMode QualityMode, float& ScreenPercentage);

private:
	static bool bXeSSSupported;

#if USE_XESS
	static FXeSSRHI* XeSSRHI;
	static FXeSSUpscaler* XeSSUpscaler;
#endif
};
