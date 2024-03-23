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

#include "XeSSMacros.h"

#include "CoreMinimal.h"
#include "xess.h"

DECLARE_LOG_CATEGORY_EXTERN(LogXeSS, Verbose, All);

class FD3D12DynamicRHI;
class FDynamicRHI;
class FRHICommandList;
class FRHICommandListImmediate;
class FRHIBuffer;
class FRHITexture;
class IConsoleVariable;
struct ID3D12Device;
struct ID3D12DynamicRHI;

#if ENGINE_MAJOR_VERSION < 5
class FRHIStructuredBuffer;
#endif // ENGINE_MAJOR_VERSION < 5

struct XESSPLUGIN_API FXeSSInitArguments
{
	uint32 OutputWidth = 0;
	uint32 OutputHeight = 0;
	int32 QualitySetting = 0;
	uint32 InitFlags = 0;
};

struct XESSPLUGIN_API FXeSSExecuteArguments
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

class XESSPLUGIN_API FXeSSRHI
{
	
public:
	FXeSSRHI(FDynamicRHI* DynamicRHI);
	virtual ~FXeSSRHI();

	void RHIInitializeXeSS(const FXeSSInitArguments& InArguments);
	bool EffectRecreationIsRequired(const FXeSSInitArguments& NewArgs) const;
	void RHIExecuteXeSS(const FXeSSExecuteArguments& InArguments);
	float GetMinSupportedResolutionFraction();
	float GetMaxSupportedResolutionFraction();
	float GetOptimalResolutionFraction();
	float GetOptimalResolutionFraction(const xess_quality_settings_t InQualitySetting);
	uint32 GetXeSSInitFlags();

#if ENGINE_MAJOR_VERSION >= 5
	void TriggerResourceTransitions(FRHICommandListImmediate& RHICmdList, FRHIBuffer* DummyBuffer) const;
#else // ENGINE_MAJOR_VERSION >= 5
	void TriggerResourceTransitions(FRHICommandListImmediate& RHICmdList, FRHIStructuredBuffer* DummyBuffer) const;
#endif // ENGINE_MAJOR_VERSION >= 5

	bool IsXeSSInitialized() { return bXeSSInitialized; }
	void HandleXeSSEnabledSet(IConsoleVariable* Variable);
	void HandleXeSSQualitySet(IConsoleVariable* Variable);
private:
	void InitResolutionFractions();
	void TriggerFrameCapture(int FrameCount) const;

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	ID3D12DynamicRHI* D3D12RHI = nullptr;
#else // XESS_ENGINE_VERSION_GEQ(5, 1)
	FD3D12DynamicRHI* D3D12RHI = nullptr;
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)
	FXeSSInitArguments InitArgs;

	xess_context_handle_t XeSSContext = nullptr;
	// Used by `r.XeSS.Enabled` console variable `OnChanged` callback
	bool bCurrentXeSSEnabled = false;
	bool bXeSSInitialized = false;

	// TODO: remove it
	xess_quality_settings_t QualitySetting = XESS_QUALITY_SETTING_BALANCED;
};
