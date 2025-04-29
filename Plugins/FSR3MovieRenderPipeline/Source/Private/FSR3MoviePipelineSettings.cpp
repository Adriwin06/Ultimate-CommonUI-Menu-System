// This file is part of the FidelityFX Super Resolution 3.1 Unreal Engine Plugin.
//
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "FSR3MoviePipelineSettings.h"

#include "FFXFSR3TemporalUpscaling.h"

#include "MovieRenderPipelineDataTypes.h"
#include "SceneView.h"
#include "Templates/SharedPointer.h"

#define LOCTEXT_NAMESPACE "FSR3MoviePipelineSettings"

UFSR3MoviePipelineSettings::UFSR3MoviePipelineSettings()
: FSR3Quality(EFSR3MoviePipelineQuality::Quality)
{

}

void UFSR3MoviePipelineSettings::ValidateStateImpl()
{
    Super::ValidateStateImpl();

	IFFXFSR3TemporalUpscalingModule& FSR3ModuleInterface = FModuleManager::GetModuleChecked<IFFXFSR3TemporalUpscalingModule>(TEXT("FFXFSR3TemporalUpscaling"));
	if (!FSR3ModuleInterface.IsPlatformSupported(GMaxRHIShaderPlatform))
	{	
		ValidationResults.Add(FText::FromString(TEXT("FidelityFX Super Resolution 3 is unsupported on this platform.")));
		ValidationState = EMoviePipelineValidationState::Warnings;
	}
}

void UFSR3MoviePipelineSettings::GetFormatArguments(FMoviePipelineFormatArgs& InOutFormatArgs) const
{
    Super::GetFormatArguments(InOutFormatArgs);

	IFFXFSR3TemporalUpscalingModule& FSR3ModuleInterface = FModuleManager::GetModuleChecked<IFFXFSR3TemporalUpscalingModule>(TEXT("FFXFSR3TemporalUpscaling"));
	if (FSR3ModuleInterface.IsPlatformSupported(GMaxRHIShaderPlatform))
	{
		InOutFormatArgs.FileMetadata.Add(TEXT("amd/fidelityFxFSR3QualityMode"), StaticEnum<EFSR3MoviePipelineQuality>()->GetDisplayNameTextByIndex((int32)FSR3Quality).ToString());
		InOutFormatArgs.FilenameArguments.Add(TEXT("fidelityFxFSR3QualityMode"), StaticEnum<EFSR3MoviePipelineQuality>()->GetDisplayNameTextByIndex((int32)FSR3Quality).ToString());
	}
	else
	{
		InOutFormatArgs.FileMetadata.Add(TEXT("amd/fidelityFxFSR3QualityMode"), TEXT("Unsupported"));
		InOutFormatArgs.FilenameArguments.Add(TEXT("fidelityFxFSR3QualityMode"), TEXT("Unsupported"));
	}
}

void UFSR3MoviePipelineSettings::SetupViewFamily(FSceneViewFamily& ViewFamily)
{
	static IConsoleVariable* CVarScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	static IConsoleVariable* CVarFSR3Enabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR3.Enabled"));
	IFFXFSR3TemporalUpscalingModule& FSR3ModuleInterface = FModuleManager::GetModuleChecked<IFFXFSR3TemporalUpscalingModule>(TEXT("FFXFSR3TemporalUpscaling"));
	if (ViewFamily.ViewMode == EViewModeIndex::VMI_Lit && CVarFSR3Enabled->GetInt())
	{
		float ScreenPercentage = FSR3Quality == EFSR3MoviePipelineQuality::Native ? 100.f : FSR3ModuleInterface.GetResolutionFraction((uint32)FSR3Quality) * 100.0f;
		if (CVarScreenPercentage)
		{
			CVarScreenPercentage->Set(ScreenPercentage, ECVF_SetByCode);
		}
	}
}
