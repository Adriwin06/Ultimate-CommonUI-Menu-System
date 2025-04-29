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

#pragma once

#include "FFXShared.h"

#include "Modules/ModuleManager.h"
#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "RHIFwd.h"
#else
#include "RHI.h"
#endif

class FRDGTexture;
#if UE_VERSION_AT_LEAST(5, 1, 0)
enum EPixelFormat : uint8;
#else
enum EPixelFormat;
#endif
#if UE_VERSION_AT_LEAST(5, 2, 0)
enum class ERHIAccess : uint32;
#endif
typedef struct FfxCreateResourceDescription FfxCreateResourceDescription;
typedef struct ffxConfigureDescFrameGeneration ffxConfigureDescFrameGeneration;
typedef struct ffxConfigureDescFrameGenerationRegisterDistortionFieldResource ffxConfigureDescFrameGenerationRegisterDistortionFieldResource;

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

struct FFXSharedAllocCallbacks
{
	static void* ffxAlloc(void* pUserData, uint64_t size)
	{
		return FMemory::Malloc(size);
	}

	static void ffxDealloc(void* pUserData, void* pMem)
	{
		return FMemory::Free(pMem);
	}

	ffxAllocationCallbacks Cbs;

	FFXSharedAllocCallbacks()
	{
		Cbs.pUserData = nullptr;
		Cbs.alloc = &ffxAlloc;
		Cbs.dealloc = &ffxDealloc;
	}
};

class FRDGBuilder;

class IFFXSharedBackend
{
public:
	virtual ffxReturnCode_t ffxCreateContext(ffxContext* context, ffxCreateContextDescHeader* desc) = 0;
	virtual ffxReturnCode_t ffxDestroyContext(ffxContext* context) = 0;
	virtual ffxReturnCode_t ffxConfigure(ffxContext* context, const ffxConfigureDescHeader* desc) = 0;
	virtual ffxReturnCode_t ffxQuery(ffxContext* context, ffxQueryDescHeader* desc) = 0;
	virtual ffxReturnCode_t ffxDispatch(ffxContext* context, const ffxDispatchDescHeader* desc) = 0;

	virtual void Init() = 0;
	virtual EFFXBackendAPI GetAPI() const = 0;
	virtual void SetFeatureLevel(ffxContext* context, ERHIFeatureLevel::Type FeatureLevel) = 0;

	virtual FfxSwapchain GetSwapchain(void* swapChain) = 0;
    virtual FfxApiResource GetNativeResource(FRHITexture* Texture, FfxApiResourceState State) = 0;
    virtual FfxApiResource GetNativeResource(FRDGTexture* Texture, FfxApiResourceState State) = 0;
	virtual FfxCommandList GetNativeCommandBuffer(FRHICommandListImmediate& RHICmdList, FRHITexture* Texture) = 0;
	virtual FfxShaderModel GetSupportedShaderModel() = 0;
	virtual bool IsFloat16Supported() = 0;
	virtual void ForceUAVTransition(FRHICommandListImmediate& RHICmdList, FRHITexture* OutputTexture, ERHIAccess Access) = 0;
	virtual void UpdateSwapChain(ffxContext* Context, ffxConfigureDescFrameGeneration& Desc) = 0;
	virtual void UpdateSwapChain(ffxContext* Context, ffxConfigureDescFrameGeneration& Desc, ffxConfigureDescFrameGenerationRegisterDistortionFieldResource& DescDistortion) = 0;

	virtual FfxApiResource GetInterpolationOutput(FfxSwapchain SwapChain) = 0;
	virtual void* GetInterpolationCommandList(FfxSwapchain SwapChain) = 0;
	virtual void RegisterFrameResources(FRHIResource* FIResources, uint64 FrameID) = 0;
	virtual bool GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS) = 0;
	virtual void CopySubRect(FfxCommandList CmdList, FfxApiResource Src, FfxApiResource Dst, FIntPoint OutputExtents, FIntPoint OutputPoint) = 0;
	virtual void Flush(FRHITexture* Tex, FRHICommandListImmediate& RHICmdList) = 0;

#if UE_VERSION_OLDER_THAN(5, 0, 0)
	static FRDGBuilder* GetGraphBuilder() { return GraphBuilder; }
	static void SetGraphBuilder(FRDGBuilder* InGraphBuilder) { GraphBuilder = InGraphBuilder; }
private:
	static FFXSHARED_API FRDGBuilder* GraphBuilder;
#endif
};

extern FFXSHARED_API FfxApiSurfaceFormat GetFFXApiFormat(EPixelFormat UEFormat, bool bSRGB);
extern FFXSHARED_API ERHIAccess GetUEAccessState(FfxResourceStates State);

class IFFXSharedBackendModule : public IModuleInterface
{
public:
	virtual IFFXSharedBackend* GetBackend() = 0;
};
