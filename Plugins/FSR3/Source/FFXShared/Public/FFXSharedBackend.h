// This file is part of the FidelityFX Super Resolution 3.0 Unreal Engine Plugin.
//
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

#pragma once

#include "FFXShared.h"

#include "Modules/ModuleManager.h"
#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "RHIFwd.h"
#else
#include "RHI.h"
#endif

class FRDGTexture;
enum EPixelFormat : uint8;
#if UE_VERSION_AT_LEAST(5, 2, 0)
enum class ERHIAccess : uint32;
#endif
typedef struct FfxCreateResourceDescription FfxCreateResourceDescription;

namespace FFXStrings
{
	static constexpr auto D3D12 = TEXT("D3D12");
}

enum class EFFXBackendAPI : uint8
{
	D3D12,
	Unreal,
	Unsupported,
	Unknown
};

struct FFXSharedResource
{
	FfxResource Resource;
	void* Data;
};

class IFFXSharedBackend
{
public:
	virtual void Init() = 0;
	virtual EFFXBackendAPI GetAPI() const = 0;
	virtual void SetFeatureLevel(FfxInterface& OutInterface, ERHIFeatureLevel::Type FeatureLevel) = 0;
	virtual size_t GetGetScratchMemorySize() = 0;
	virtual FfxErrorCode CreateInterface(FfxInterface& OutInterface, uint32 MaxContexts) = 0;
	virtual FfxDevice GetDevice(void* device) = 0;
	virtual FfxCommandList GetCommandList(void* list) = 0;
	virtual FfxResource GetResource(void* resource, wchar_t* name, FfxResourceStates state, uint32 shaderComponentMapping) = 0;
	virtual FfxCommandQueue GetCommandQueue(void* cmdQueue) = 0;
	virtual FfxSwapchain GetSwapchain(void* swapChain) = 0;
	virtual FfxDevice GetNativeDevice() = 0;
    virtual FfxResource GetNativeResource(FRHITexture* Texture, FfxResourceStates State) = 0;
    virtual FfxResource GetNativeResource(FRDGTexture* Texture, FfxResourceStates State) = 0;
	virtual FfxCommandList GetNativeCommandBuffer(FRHICommandListImmediate& RHICmdList) = 0;
	virtual uint32 GetNativeTextureFormat(FRHITexture* Texture) = 0;
	virtual FfxShaderModel GetSupportedShaderModel() = 0;
	virtual bool IsFloat16Supported() = 0;
	virtual void ForceUAVTransition(FRHICommandListImmediate& RHICmdList, FRHITexture* OutputTexture, ERHIAccess Access) = 0;
	virtual void UpdateSwapChain(FfxInterface& Interface, void* SwapChain, bool mode, bool allowAsyncWorkloads, bool showDebugView) = 0;
	virtual FfxResource GetInterpolationOutput(FfxSwapchain SwapChain) = 0;
	virtual FfxCommandList GetInterpolationCommandList(FfxSwapchain SwapChain) = 0;
	virtual void BindUITexture(FfxSwapchain gameSwapChain, FfxResource uiResource) = 0;
	virtual FFXSharedResource CreateResource(FfxInterface& Interface, const FfxCreateResourceDescription* desc) = 0;
	virtual FfxErrorCode ReleaseResource(FfxInterface& Interface, FFXSharedResource Resource) = 0;
	virtual void RegisterFrameResources(FRHIResource* FIResources, IRefCountedObject* FSR3Resources) = 0;
	virtual bool GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS) = 0;
	virtual void CopySubRect(FfxCommandList CmdList, FfxResource Src, FfxResource Dst, FIntPoint OutputExtents, FIntPoint OutputPoint) = 0;
};

extern FFXSHARED_API EPixelFormat GetUEFormat(FfxSurfaceFormat Format);
extern FFXSHARED_API FfxSurfaceFormat GetFFXFormat(EPixelFormat UEFormat, bool bSRGB);
extern FFXSHARED_API ERHIAccess GetUEAccessState(FfxResourceStates State);

class IFFXSharedBackendModule : public IModuleInterface
{
public:
	virtual IFFXSharedBackend* GetBackend() = 0;
};
