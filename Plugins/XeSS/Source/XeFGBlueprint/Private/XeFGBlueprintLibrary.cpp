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

#include "XeFGBlueprintLibrary.h"

#include "HAL/IConsoleManager.h"
#include "XeSSCommonMacros.h"

#if WITH_XEFG
#include "XeFGModule.h"
#include "XeFGRHIModule.h"
#include "XeFGRHI.h"
#endif

#define XEFG_ENABLE_IN_BP (XESS_ENGINE_WITH_XEFG_API && WITH_XEFG)

// Use this macro only with Blueprint functions that depend on module XeFGCore.
// Note: only check if WITH_XESS to simplify code
#if WITH_XEFG
#define XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(FunctionName) \
	checkf(bInitialized, TEXT("%s can't be called before module XeFGCore loaded."), TEXT(#FunctionName))
#else
#define XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(FunctionName)
#endif

bool UXeFGBlueprintLibrary::bInitialized = false;
bool UXeFGBlueprintLibrary::bIsXeFGSupported = false;
FXeFGRHI* UXeFGBlueprintLibrary::XeFGRHI = nullptr;

static FString GetDisplayName(EXeFGMode Mode)
{
	static UEnum* Enum = StaticEnum<EXeFGMode>();
	return Enum->GetDisplayNameTextByValue(static_cast<int64>(Mode)).ToString();
}

static bool IsValidEnumValue(int32 CVarInt)
{
	static UEnum* Enum = StaticEnum<EXeFGMode>();
	return Enum->IsValidEnumValue(CVarInt) && CVarInt != Enum->GetMaxEnumValue();
}

static bool IsValidEnumValue(EXeFGMode Mode)
{
	return IsValidEnumValue(static_cast<int32>(Mode));
}

static EXeFGMode ToXeFGMode(int32 CVarInt)
{
	static UEnum* Enum = StaticEnum<EXeFGMode>();
	if (IsValidEnumValue(CVarInt))
	{
		return EXeFGMode(CVarInt);
	}
	FFrame::KismetExecutionMessage(*FString::Printf(TEXT("ToXeFGMode called with invalid value (%d)"),
		CVarInt), ELogVerbosity::Warning);
	return EXeFGMode::Off;
}

void UXeFGBlueprintLibrary::Init(FXeFGModule* InXeFGModule)
{
#if WITH_XEFG
	check(InXeFGModule);
	bIsXeFGSupported = InXeFGModule->IsXeFGSupported();
	XeFGRHI = InXeFGModule->GetXeFGRHI();
	bInitialized = true;
#endif
}

void UXeFGBlueprintLibrary::Deinit()
{
#if WITH_XEFG
	bIsXeFGSupported = false;
	XeFGRHI = nullptr;
	bInitialized = false;
#endif
}

bool UXeFGBlueprintLibrary::IsXeFGSupported()
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(IsXeSSSupported);

	return UXeFGBlueprintLibrary::bIsXeFGSupported;
}

TArray<EXeFGMode> UXeFGBlueprintLibrary::GetSupportedXeFGModes()
{
	TArray<EXeFGMode> SupportedModes;
	const UEnum* ModeEnum = StaticEnum<EXeFGMode>();

	for (int32 EnumIndex = 0; EnumIndex < ModeEnum->NumEnums() - 1; ++EnumIndex)
	{
		SupportedModes.Add(EXeFGMode(ModeEnum->GetValueByIndex(EnumIndex)));
	}
	return SupportedModes;
}

EXeFGMode UXeFGBlueprintLibrary::GetXeFGMode()
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(GetXeFGMode);

#if XEFG_ENABLE_IN_BP
	static const auto CVarXeFGMode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.Enabled"));
	return ToXeFGMode(CVarXeFGMode->GetInt());
#else
	return EXeFGMode::Off;
#endif
}

void UXeFGBlueprintLibrary::SetXeFGMode(EXeFGMode Mode)
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(SetXeFGMode);

#if XEFG_ENABLE_IN_BP
	if (!IsValidEnumValue(Mode))
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("SetXeFGMode called with invalid enum value (%d) %s"),
			Mode, *GetDisplayName(Mode)), ELogVerbosity::Warning);
		return;
	}
	static const auto CVarXeFGMode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.Enabled"));
	CVarXeFGMode->SetWithCurrentPriority(static_cast<int32>(Mode));
#endif
}

XEFGBLUEPRINT_API bool UXeFGBlueprintLibrary::IfRelaunchRequiredByXeFG()
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(IfRelaunchRequiredByXeFG);

#if XEFG_ENABLE_IN_BP
	return GetXeFGMode() == EXeFGMode::On && !XeFGRHI->IfGameViewportUsesXeFGSwapChain();
#else
	return false;
#endif
}
