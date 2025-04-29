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

#include "XeFGBlueprintLibrary.generated.h"

class FXeFGModule;
class FXeFGRHI;

UENUM(BlueprintType)
enum class EXeFGMode : uint8
{
	Off		UMETA(DisplayName = "Off"),
	On		UMETA(DisplayName = "On"),
};

UCLASS()
class UXeFGBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void Init(FXeFGModule* InXeFGModule);
	static void Deinit();

	/** Check if Intel XeSS-FG is supported on the current platform */
	UFUNCTION(BlueprintPure, Category = "XeSS-FG", meta = (DisplayName = "Is Intel(R) XeSS-FG Supported"))
	static XEFGBLUEPRINT_API bool IsXeFGSupported();

	/** List all available Intel XeSS-FG modes */
	UFUNCTION(BlueprintCallable, Category = "XeSS-FG", meta = (DisplayName = "Get Supported Intel(R) XeSS-FG Modes"))
	static XEFGBLUEPRINT_API TArray<EXeFGMode> GetSupportedXeFGModes();

	/** Get the current Intel XeSS-FG mode */
	UFUNCTION(BlueprintPure, Category = "XeSS-FG", meta = (DisplayName = "Get Current Intel(R) XeSS-FG Mode"))
	static XEFGBLUEPRINT_API EXeFGMode GetXeFGMode();

	/** Set the selected Intel XeSS-FG mode */
	UFUNCTION(BlueprintCallable, Category = "XeSS-FG", meta = (DisplayName = "Set Intel(R) XeSS-FG Mode"))
	static XEFGBLUEPRINT_API void SetXeFGMode(EXeFGMode Mode);

	/** If relaunch is required by Intel XeSS-FG */
	UFUNCTION(BlueprintPure, Category = "XeSS-FG", meta = (DisplayName = "If Relaunch is Required by XeSS-FG"))
	static XEFGBLUEPRINT_API bool IfRelaunchRequiredByXeFG();

private:
	static bool bInitialized;
	static bool bIsXeFGSupported;
	static FXeFGRHI* XeFGRHI;
};
