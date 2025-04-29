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

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "xess/xess.h"

class FRHICommandListBase;
class FRHITexture;
class FXeSSSDKWrapperBase;
class IConsoleVariable;

struct FXeSSInitArguments
{
	uint32 OutputWidth = 0;
	uint32 OutputHeight = 0;
	int32 QualitySetting = 0;
	uint32 InitFlags = 0;
};

struct FXeSSExecuteArguments
{
	FRHITexture* ColorTexture = nullptr;
	FRHITexture* VelocityTexture = nullptr;
	FRHITexture* OutputTexture = nullptr;

	float JitterOffsetX = 0.0f;
	float JitterOffsetY = 0.0f;

	FIntRect SrcViewRect = FIntRect(FIntPoint::ZeroValue, FIntPoint::ZeroValue);
	FIntRect DstViewRect = FIntRect(FIntPoint::ZeroValue, FIntPoint::ZeroValue);

	uint32 bCameraCut = 0;
};

class XESSCORE_API FXeSSRHI
{
public:
	FXeSSRHI();
	virtual ~FXeSSRHI();
	bool Initialize();

	virtual void OnXeSSSDKLoaded() = 0;
	virtual void RHIInitializeXeSS(const FXeSSInitArguments& InArguments) = 0;
	virtual void RHIExecuteXeSS(const FXeSSExecuteArguments& InArguments, FRHICommandListBase& ExecutingCmdList) = 0;
	virtual xess_result_t CreateXeSSContext(xess_context_handle_t* OutXeSSContext) = 0;
	virtual xess_result_t BuildXeSSPipelines(xess_context_handle_t InXeSSContext, uint32_t InInitFlags) = 0;

	bool EffectRecreationIsRequired(const FXeSSInitArguments& NewArgs) const;
	float GetMinSupportedResolutionFraction();
	float GetMaxSupportedResolutionFraction();
	float GetOptimalResolutionFraction();
	float GetOptimalResolutionFraction(xess_quality_settings_t InQualitySetting);
	uint32 GetXeSSInitFlags();
	bool IsXeSSInitialized() { return bXeSSInitialized; }
	void HandleXeSSEnabledSet(IConsoleVariable* Variable);
	void HandleXeSSQualitySet(IConsoleVariable* Variable);
protected:
	FXeSSInitArguments InitArgs;
	FXeSSSDKWrapperBase* XeSSSDKWrapper = nullptr;
	xess_context_handle_t XeSSContext = nullptr;
private:
	void InitResolutionFractions();
	// Used by `r.XeSS.Enabled` console variable `OnChanged` callback
	bool bCurrentXeSSEnabled = false;
	bool bXeSSInitialized = false;
};

class IXeSSRHIModule : public IModuleInterface
{
public:
	virtual FXeSSRHI* CreateXeSSRHI() = 0;
};
