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

#include "FFXSharedBackend.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#pragma warning( push )
#pragma warning( disable : 4191 )
#else
#define _countof(a) (sizeof(a)/sizeof(*(a)))
#define strcpy_s(a, b) strcpy(a, b)
#define FFX_GCC 1
#endif
THIRD_PARTY_INCLUDES_START
#if UE_VERSION_AT_LEAST(5, 3, 0)
#include <bit>
#endif
#include "FidelityFX/host/ffx_interface.h"
THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
#pragma warning( pop )
#include "Windows/HideWindowsPlatformTypes.h"
#else
#undef _countof
#undef strcpy_s
#undef FFX_GCC
#endif
#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "RHIResources.h"
#else
#include "RHI.h"
#endif
#include "RendererInterface.h"
#include "RenderGraphDefinitions.h"
#include "RenderGraphBuilder.h"

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_RHI 0x0000004u

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

	uint8 StagingRingBuffer[FFX_ALIGN_UP(FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE, sizeof(uint32_t))];
	uint32 StagingRingBufferBase;

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
// FFX-style functions for the RHI backend to help setup the FSR3 library.
//-------------------------------------------------------------------------------------
extern FfxErrorCode ffxGetInterfaceUE(FfxInterface* outInterface, void* scratchBuffer, size_t scratchBufferSize);
extern size_t ffxGetScratchMemorySizeUE();
extern FfxResource ffxGetResourceFromUEResource(FfxInterface* backendInterface, FRDGTexture* rdgRes, FfxResourceStates state = FFX_RESOURCE_STATE_COMPUTE_READ);

class FFXRHIBACKEND_API FFXRHIBackend : public IFFXSharedBackend
{
	FFXSharedAllocCallbacks AllocCbs;
public:
	FFXRHIBackend();
	virtual ~FFXRHIBackend();

	void OnViewportCreatedHandler_SetCustomPresent();
	void OnBeginDrawHandler();

	ffxReturnCode_t ffxCreateContext(ffxContext* context, ffxCreateContextDescHeader* desc) final;
	ffxReturnCode_t ffxDestroyContext(ffxContext* context) final;
	ffxReturnCode_t ffxConfigure(ffxContext* context, const ffxConfigureDescHeader* desc) final;
	ffxReturnCode_t ffxQuery(ffxContext* context, ffxQueryDescHeader* desc) final;
	ffxReturnCode_t ffxDispatch(ffxContext* context, const ffxDispatchDescHeader* desc) final;

	void Init() final;
	EFFXBackendAPI GetAPI() const final;
	void SetFeatureLevel(ffxContext* context, ERHIFeatureLevel::Type FeatureLevel) final;
	FfxSwapchain GetSwapchain(void* swapChain) final;
	FfxApiResource GetNativeResource(FRHITexture* Texture, FfxApiResourceState State) final;
	FfxApiResource GetNativeResource(FRDGTexture* Texture, FfxApiResourceState State) final;
	FfxCommandList GetNativeCommandBuffer(FRHICommandListImmediate& RHICmdList, FRHITexture* Texture) final;
	FfxShaderModel GetSupportedShaderModel() final;
	bool IsFloat16Supported() final;
	void ForceUAVTransition(FRHICommandListImmediate& RHICmdList, FRHITexture* OutputTexture, ERHIAccess Access) final;
	void UpdateSwapChain(ffxContext* Context, ffxConfigureDescFrameGeneration& Desc) final;
	void UpdateSwapChain(ffxContext* Context, ffxConfigureDescFrameGeneration& Desc, ffxConfigureDescFrameGenerationRegisterDistortionFieldResource& DescDistortion) final;
	FfxApiResource GetInterpolationOutput(FfxSwapchain SwapChain) final;
	void* GetInterpolationCommandList(FfxSwapchain SwapChain) final;
	void RegisterFrameResources(FRHIResource* FIResources, uint64 FrameID) final;
	bool GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS) final;
	void CopySubRect(FfxCommandList CmdList, FfxApiResource Src, FfxApiResource Dst, FIntPoint OutputExtents, FIntPoint OutputPoint) final;
	void Flush(FRHITexture* Tex, FRHICommandListImmediate& RHICmdList) final;
};
