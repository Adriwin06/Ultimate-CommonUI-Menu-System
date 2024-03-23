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

#include "Windows/AllowWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_START
#include "FidelityFX/host/ffx_interface.h"
THIRD_PARTY_INCLUDES_END
#include "Windows/HideWindowsPlatformTypes.h"
#include "FFXSharedBackend.h"
#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "RHIResources.h"
#else
#include "RHI.h"
#endif
#include "RendererInterface.h"
#include "RenderGraphDefinitions.h"
#include "RenderGraphBuilder.h"

//-------------------------------------------------------------------------------------
// The maximum number of resources that can be allocated.
//-------------------------------------------------------------------------------------
#define FFX_RHI_MAX_RESOURCE_COUNT (256)
#define FFX_MAX_BLOCK_RESOURCE_COUNT (64)
#define FFX_MAX_BLOCK_COUNT (4)
#define FFX_MAX_JOB_COUNT (128)

//-------------------------------------------------------------------------------------
// State data for the FFX SDK backend that manages mapping resources between UE & FFX SDK.
//-------------------------------------------------------------------------------------
struct FFXRHIBACKEND_API FFXBackendState
{
	struct Resource
	{
		uint32 EffectId;
		FRHIResource* Resource;
		FfxResourceDescription Desc;
		TRefCountPtr<IPooledRenderTarget>* RT;
		FRDGTexture* RDG;
		TRefCountPtr<FRDGPooledBuffer>* PooledBuffer;
	} Resources[FFX_RHI_MAX_RESOURCE_COUNT];

	struct Block
	{
		uint64 ResourceMask;
		uint64 DynamicMask;
	} Blocks[FFX_MAX_BLOCK_COUNT];

	FfxGpuJobDescription Jobs[FFX_MAX_JOB_COUNT];
	uint32 NumJobs;
	ERHIFeatureLevel::Type FeatureLevel;
	FfxDevice device;
	uint32 EffectIndex;

	uint32 AllocEffect();
	uint32 GetEffectId(uint32 Index);
	void SetEffectId(uint32 Index, uint32 EffectId);

	uint32 AllocIndex();
	void MarkDynamic(uint32 Index);
	uint32 GetDynamicIndex();
	bool IsValidIndex(uint32 Index);
	void FreeIndex(uint32 Index);

	uint32 AddResource(FRHIResource* Resource, FfxResourceType Type, TRefCountPtr<IPooledRenderTarget>* RT, FRDGTexture* RDG, TRefCountPtr<FRDGPooledBuffer>* PooledBuffer);

	FRHIResource* GetResource(uint32 Index);

	FRDGTextureRef GetOrRegisterExternalTexture(FRDGBuilder& GraphBuilder, uint32 Index);

	FRDGTexture* GetRDGTexture(FRDGBuilder& GraphBuilder, uint32 Index);

	FRDGBufferRef GetRDGBuffer(FRDGBuilder& GraphBuilder, uint32 Index);

	TRefCountPtr<IPooledRenderTarget> GetPooledRT(uint32 Index);

	FfxResourceType GetType(uint32 Index);

	void RemoveResource(uint32 Index);
};

//-------------------------------------------------------------------------------------
// FFX-style functions for the RHI backend to help setup the FSR2 library.
//-------------------------------------------------------------------------------------
extern FfxErrorCode ffxGetInterfaceUE(FfxInterface* outInterface, void* scratchBuffer, size_t scratchBufferSize);
extern size_t ffxGetScratchMemorySizeUE();
extern FfxResource ffxGetResourceFromUEResource(FfxInterface* backendInterface, FRDGTexture* rdgRes, FfxResourceStates state = FFX_RESOURCE_STATE_COMPUTE_READ);

class FFXRHIBACKEND_API FFXRHIBackend : public IFFXSharedBackend
{
public:
	FFXRHIBackend();
	virtual ~FFXRHIBackend();

	void OnViewportCreatedHandler_SetCustomPresent();
	void OnBeginDrawHandler();

	void Init() final;
	EFFXBackendAPI GetAPI() const final;
	void SetFeatureLevel(FfxInterface& OutInterface, ERHIFeatureLevel::Type FeatureLevel) final;
	size_t GetGetScratchMemorySize() final;
	FfxErrorCode CreateInterface(FfxInterface& OutInterface, uint32 MaxContexts) final;
	FfxDevice GetDevice(void* device) final;
	FfxCommandList GetCommandList(void* list) final;
	FfxResource GetResource(void* resource, wchar_t* name, FfxResourceStates state, uint32 shaderComponentMapping) final;
	FfxCommandQueue GetCommandQueue(void* cmdQueue) final;
	FfxSwapchain GetSwapchain(void* swapChain) final;
	FfxDevice GetNativeDevice() final;
	FfxResource GetNativeResource(FRHITexture* Texture, FfxResourceStates State) final;
    FfxResource GetNativeResource(FRDGTexture* Texture, FfxResourceStates State) final;
	FfxCommandList GetNativeCommandBuffer(FRHICommandListImmediate& RHICmdList) final;
	uint32 GetNativeTextureFormat(FRHITexture* Texture) final;
	FfxShaderModel GetSupportedShaderModel() final;
	bool IsFloat16Supported() final;
	void ForceUAVTransition(FRHICommandListImmediate& RHICmdList, FRHITexture* OutputTexture, ERHIAccess Access) final;
	void UpdateSwapChain(FfxInterface& Interface, void* SwapChain, bool mode, bool allowAsyncWorkloads, bool showDebugView) final;
	FfxResource GetInterpolationOutput(FfxSwapchain SwapChain) final;
	FfxCommandList GetInterpolationCommandList(FfxSwapchain SwapChain) final;
	void BindUITexture(FfxSwapchain gameSwapChain, FfxResource uiResource) final;
	FFXSharedResource CreateResource(FfxInterface& Interface, const FfxCreateResourceDescription* desc) final;
	FfxErrorCode ReleaseResource(FfxInterface& Interface, FFXSharedResource Resource) final;
	void RegisterFrameResources(FRHIResource* FIResources, IRefCountedObject* FSR3Resources) final;
	bool GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS) final;
	void CopySubRect(FfxCommandList CmdList, FfxResource Src, FfxResource Dst, FIntPoint OutputExtents, FIntPoint OutputPoint) final;
};
