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

#pragma once

#include "Engine/DeveloperSettings.h"

#include "XeSSSettings.generated.h"

UCLASS(config = Engine, defaultconfig, meta = (DisplayName = "Intel XeSS"))
class XESSPLUGIN_API UXeSSSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:

	UXeSSSettings();

	/** This enables XeSS in editor viewports */
	UPROPERTY(config, EditAnywhere, Category = "Editor", meta = (
		DisplayName = "Allow Intel XeSS to be turned on in Editor viewports",
		ToolTip = "Disabling will only allow to enable XeSS when running standalone game"))
	bool bEnableXeSSInEditorViewports;

	UPROPERTY(config, EditAnywhere, Category = "Debug", meta = (
		ConsoleVariable = "r.XeSS.FrameDump.Path",
		DisplayName = "Intel XeSS debug data save location",
		ToolTip = "Directory that will be used for debug images and data when r.XeSS.FrameDump.Start is called, can be changed with r.XeSS.FrameDump.Path"))
	FString DebugDataDumpPath;

	virtual void PostInitProperties() override;
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
