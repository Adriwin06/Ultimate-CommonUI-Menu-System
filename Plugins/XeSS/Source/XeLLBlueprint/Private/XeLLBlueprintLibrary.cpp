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

#include "XeLLBlueprintLibrary.h"

#include "XeSSCommonMacros.h"

#define XELL_ENABLE_IN_BP (XESS_ENGINE_WITH_XEFG_API && WITH_XELL)

#if XELL_ENABLE_IN_BP
#include "XeLLLatencyMarkers.h"
#include "XeLLMaxTickRateHandler.h"
#include "XeLLModule.h"
#endif

#if WITH_XELL
#define XELL_CHECK_BLUEPRINT_FUNCTION_CALL(FunctionName) \
	checkf(bInitialized, TEXT("%s can't be called before module XeLLCore loaded."), TEXT(#FunctionName))
#else
#define XELL_CHECK_BLUEPRINT_FUNCTION_CALL(FunctionName)
#endif

bool UXeLLBlueprintLibrary::bInitialized = false;
FXeLLModule* UXeLLBlueprintLibrary::XeLLModule = nullptr;

void UXeLLBlueprintLibrary::Init(FXeLLModule* InXeLLModule)
{
#if WITH_XELL
	check(InXeLLModule);
	XeLLModule = InXeLLModule;
	bInitialized = true;
#endif
}

void UXeLLBlueprintLibrary::Deinit()
{
#if WITH_XELL
	XeLLModule = nullptr;
	bInitialized = false;
#endif
}

bool UXeLLBlueprintLibrary::IsXeLLSupported()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(IsXeLLSupported);

#if XELL_ENABLE_IN_BP
	return XeLLModule->IsXeLLSupported();
#else
	return false;
#endif
}

TArray<EXeLLMode> UXeLLBlueprintLibrary::GetSupportedXeLLModes()
{
	TArray<EXeLLMode> SupportedXeLLModes;
	const UEnum* XeLLModeEnum = StaticEnum<EXeLLMode>();

	for (int32 EnumIndex = 0; EnumIndex < XeLLModeEnum->NumEnums() - 1; ++EnumIndex)
	{
		SupportedXeLLModes.Add(EXeLLMode(XeLLModeEnum->GetValueByIndex(EnumIndex)));
	}
	return SupportedXeLLModes;
}

void UXeLLBlueprintLibrary::SetXeLLMode(const EXeLLMode Mode)
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(SetXeLLMode);

#if XELL_ENABLE_IN_BP
	XeLLModule->SetXeLLMode(static_cast<uint32>(Mode));
#endif
}

EXeLLMode UXeLLBlueprintLibrary::GetXeLLMode()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(GetXeLLMode);

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetMaxTickRateHandler())
	{
		return static_cast<EXeLLMode>(XeLLModule->GetMaxTickRateHandler()->GetFlags());
	}
	return EXeLLMode::Off;
#else
	return EXeLLMode::Off;
#endif
}

void UXeLLBlueprintLibrary::SetFlashIndicatorEnabled(const bool bEnabled)
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(SetFlashIndicatorEnabled);

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetLatencyMarker())
	{
		XeLLModule->GetLatencyMarker()->SetFlashIndicatorEnabled(bEnabled);
	}
#endif
}

bool UXeLLBlueprintLibrary::GetFlashIndicatorEnabled()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(GetFlashIndicatorEnabled);

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetLatencyMarker())
	{
		return XeLLModule->GetLatencyMarker()->GetFlashIndicatorEnabled();
	}
	return false;
#else
	return false;
#endif
}

float UXeLLBlueprintLibrary::GetGameToRenderLatencyInMs()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(GetGameToRenderLatencyInMs)

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetLatencyMarker())
	{
		return XeLLModule->GetLatencyMarker()->GetTotalLatencyInMs();
	}
	return 0.0f;
#else
	return 0.0f;
#endif
}

float UXeLLBlueprintLibrary::GetGameLatencyInMs()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(GetGameLatencyInMs)

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetLatencyMarker())
	{
		return XeLLModule->GetLatencyMarker()->GetGameLatencyInMs();
	}
	return 0.0f;
#else
	return 0.0f;
#endif
}

float UXeLLBlueprintLibrary::GetRenderLatencyInMs()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(GetRenderLatencyInMs)

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetLatencyMarker())
	{
		return XeLLModule->GetLatencyMarker()->GetRenderLatencyInMs();
	}
	return 0.0f;
#else
	return 0.0f;
#endif
}

float UXeLLBlueprintLibrary::GetSimulationLatencyInMs()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(GetSimulationLatencyInMs)

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetLatencyMarker())
	{
		return XeLLModule->GetLatencyMarker()->GetSimulationLatencyInMs();
	}
	return 0.0f;
#else
	return 0.0f;
#endif
}

float UXeLLBlueprintLibrary::GetRenderSubmitLatencyInMs()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(GetRenderSubmitLatencyInMs)

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetLatencyMarker())
	{
		return XeLLModule->GetLatencyMarker()->GetRenderSubmitLatencyInMs();
	}
	return 0.0f;
#else
	return 0.0f;
#endif
}

float UXeLLBlueprintLibrary::GetPresentLatencyInMs()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(GetPresentLatencyInMs)

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetLatencyMarker())
	{
		return XeLLModule->GetLatencyMarker()->GetPresentLatencyInMs();
	}
	return 0.0f;
#else
	return 0.0f;
#endif
}

float UXeLLBlueprintLibrary::GetInputLatencyInMs()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(GetInputLatencyInMs)

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetLatencyMarker())
	{
		return XeLLModule->GetLatencyMarker()->GetInputLatencyInMs();
	}
	return 0.0f;
#else
	return 0.0f;
#endif
}

bool UXeLLBlueprintLibrary::GetLatencyMarkEnabled()
{
	XELL_CHECK_BLUEPRINT_FUNCTION_CALL(GetLatencyMarkEnabled)

#if XELL_ENABLE_IN_BP
	if (XeLLModule->GetLatencyMarker())
	{
		return XeLLModule->GetLatencyMarker()->GetEnabled();
	}
	return false;
#else
	return false;
#endif
}
