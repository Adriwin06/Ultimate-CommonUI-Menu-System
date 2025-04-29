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

#include "XeSSSettings.h"

#include "XeSSCommonMacros.h"

#include "HAL/IConsoleManager.h"
#include "XeSSUnrealCore.h"

#define LOCTEXT_NAMESPACE "XeSS"

UXeSSSettings::UXeSSSettings()
	: bEnableXeSSInEditorViewports(true)
{}

void UXeSSSettings::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	// Apply settings from ini file, this will update the console variables and project settings
	// Using ECVF_SetByGameSetting to be able to update CVar later, once setting is changed in project menu
	XeSSUnreal::ApplyCVarSettingsFromIni(TEXT("/Script/XeSSPlugin.XeSSSettings"), *GEngineIni, ECVF_SetByGameSetting);
	if (IsTemplate())
	{
		ImportConsoleVariableValues();
	}
#endif
}

FName UXeSSSettings::GetContainerName() const
{
	return FName("Project");
}
FName UXeSSSettings::GetCategoryName() const
{
	return FName("Plugins");
}

#if WITH_EDITOR
FText UXeSSSettings::GetSectionText() const
{
	return NSLOCTEXT("IntelXeSS", "IntelXeSSSettingsName", "Intel XeSS");
}

FText UXeSSSettings::GetSectionDescription() const
{
	return NSLOCTEXT("IntelXeSS", "IntelXeSSSettingsDescription", "Configure the Intel XeSS Plugin");
}

void UXeSSSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		ExportValuesToConsoleVariables(PropertyChangedEvent.Property);
	}
}
#endif

#undef LOCTEXT_NAMESPACE
