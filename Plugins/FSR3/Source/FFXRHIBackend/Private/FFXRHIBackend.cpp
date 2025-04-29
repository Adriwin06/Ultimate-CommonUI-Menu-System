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

#include "FFXRHIBackend.h"
#include "FFXRHIBackendSubPass.h"
#include "../../FFXFrameInterpolation/Public/FFXFrameInterpolationModule.h"
#include "../../FFXFrameInterpolation/Public/IFFXFrameInterpolation.h"
#include "RenderGraphUtils.h"
#include "Engine/RendererSettings.h"
#include "Containers/ResourceArray.h"
#include "Containers/DynamicRHIResourceArray.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#if UE_VERSION_OLDER_THAN(5, 0, 0)
#include "RenderGraphBuilder.h"
#endif

#include "FFXShared.h"
#include "FFXFSR3.h"
#include "FFXOpticalFlowApi.h"
#include "FFXFrameInterpolationApi.h"
#include "FFXFSR3Settings.h"

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

#include "ffx_provider.h"
#if UE_VERSION_OLDER_THAN(5, 0, 0)
#undef TRY
#define TRY(_expr) \
	{ ffxReturnCode_t _rc = (_expr); if (_rc != FFX_API_RETURN_OK) return _rc; }
#endif
#include "ffx_provider_framegeneration.h"
#include "ffx_provider_fsr3upscale.h"

THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
#pragma warning( pop )
#include "Windows/HideWindowsPlatformTypes.h"
#else
#undef _countof
#undef strcpy_s
#undef FFX_GCC
#endif

struct FFXTextureBulkData final : public FResourceBulkDataInterface
{
	FFXTextureBulkData()
	: Data(nullptr)
	, DataSize(0)
	{
	}

	FFXTextureBulkData(const void* InData, uint32 InDataSize)
	: Data(InData)
	, DataSize(InDataSize)
	{
	}

	const void* GetResourceBulkData() const { return Data; }
	uint32 GetResourceBulkDataSize() const { return DataSize; }
	
	void Discard() {}
	
	const void* Data = nullptr;
	uint32 DataSize = 0;
};

static EPixelFormat GetUEFormat(FfxSurfaceFormat Format)
{
	EPixelFormat UEFormat = PF_Unknown;
	switch (Format)
	{
	case FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS:
		UEFormat = PF_R32G32B32A32_UINT;
		break;
	case FFX_SURFACE_FORMAT_R32G32B32A32_UINT:
		UEFormat = PF_R32G32B32A32_UINT;
		break;
	case FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT:
		UEFormat = PF_A32B32G32R32F;
		break;
	case FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT:
		UEFormat = PF_FloatRGBA;
		break;
	case FFX_SURFACE_FORMAT_R10G10B10A2_UNORM:
		UEFormat = PF_A2B10G10R10;
		break;
	case FFX_SURFACE_FORMAT_R32G32_FLOAT:
		UEFormat = PF_G32R32F;
		break;
	case FFX_SURFACE_FORMAT_R32_UINT:
		UEFormat = PF_R32_UINT;
		break;
	case FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS:
		UEFormat = PF_R8G8B8A8_UINT;
		break;
	case FFX_SURFACE_FORMAT_R8G8B8A8_UNORM:
		UEFormat = PF_R8G8B8A8;
		break;
	case FFX_SURFACE_FORMAT_R8G8B8A8_SRGB:
		UEFormat = PF_R8G8B8A8;
		break;
	case FFX_SURFACE_FORMAT_R11G11B10_FLOAT:
		UEFormat = PF_FloatR11G11B10;
		break;
	case FFX_SURFACE_FORMAT_R16G16_FLOAT:
		UEFormat = PF_G16R16F;
		break;
	case FFX_SURFACE_FORMAT_R16G16_UINT:
		UEFormat = PF_R16G16_UINT;
		break;
	case FFX_SURFACE_FORMAT_R16_FLOAT:
		UEFormat = PF_R16F;
		break;
	case FFX_SURFACE_FORMAT_R16_UINT:
		UEFormat = PF_R16_UINT;
		break;
	case FFX_SURFACE_FORMAT_R16_UNORM:
		UEFormat = PF_G16;
		break;
	case FFX_SURFACE_FORMAT_R16_SNORM:
		UEFormat = PF_R16G16B16A16_SNORM;
		break;
	case FFX_SURFACE_FORMAT_R8_UNORM:
		UEFormat = PF_R8;
		break;
	case FFX_SURFACE_FORMAT_R8_UINT:
		UEFormat = PF_R8_UINT;
		break;
	case FFX_SURFACE_FORMAT_R32_FLOAT:
		UEFormat = PF_R32_FLOAT;
		break;
	case FFX_SURFACE_FORMAT_R8G8_UNORM:
		UEFormat = PF_R8G8;
		break;
	case FFX_SURFACE_FORMAT_R16G16_SINT:
		UEFormat = PF_R16G16B16A16_SINT;
		break;
	default:
		check(false);
		break;
	}
	return UEFormat;
}

static FfxErrorCode CreateResource_UE(FfxInterface* backendInterface, const FfxCreateResourceDescription* desc, FfxUInt32 effectContextId, FfxResourceInternal* outTexture)
{
	FfxErrorCode Result = FFX_OK;
	FFXBackendState* Context = (FFXBackendState*)backendInterface->scratchBuffer;
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	FRDGBuilder* GraphBuilder = FFXRHIBackend::GetGraphBuilder();
#endif

	if (Context)
	{
		ETextureCreateFlags Flags = TexCreate_None;
		Flags |= (desc->resourceDescription.usage & FFX_RESOURCE_USAGE_READ_ONLY) ? TexCreate_ShaderResource : TexCreate_None;
		Flags |= (desc->resourceDescription.usage & FFX_RESOURCE_USAGE_RENDERTARGET) ? TexCreate_RenderTargetable | TexCreate_UAV | TexCreate_ShaderResource : TexCreate_None;
		Flags |= (desc->resourceDescription.usage & FFX_RESOURCE_USAGE_UAV) ? TexCreate_UAV | TexCreate_ShaderResource : TexCreate_None;
		Flags |= desc->resourceDescription.format == FFX_SURFACE_FORMAT_R8G8B8A8_SRGB ? TexCreate_SRGB : TexCreate_None;

		size_t Size = desc->resourceDescription.width;
		void* InitData = desc->initData.buffer;
		size_t InitDataSize = desc->initData.size;
		if (desc->resourceDescription.format == FFX_SURFACE_FORMAT_R16_SNORM && desc->initData.buffer)
		{
			int16* Data = (int16*)FMemory::Malloc(desc->initData.size * 4);
			for (uint32 i = 0; i < (desc->initData.size / sizeof(int16)); i++)
			{
				Data[i * 4] = ((int16*)desc->initData.buffer)[i];
				Data[i * 4 + 1] = 0;
				Data[i * 4 + 2] = 0;
				Data[i * 4 + 3] = 0;
			}

			InitData = Data;
			InitDataSize = desc->initData.size * 4;
			Size = desc->resourceDescription.width * 4;
		}
		else if (desc->resourceDescription.format == FFX_SURFACE_FORMAT_R16G16_SINT && desc->initData.buffer)
		{
			int16* Data = (int16*)FMemory::Malloc(desc->initData.size * 2);
			for (uint32 i = 0; i < (desc->initData.size / (sizeof(int16) * 2)); i+=2)
			{
				Data[i * 2] = ((int16*)desc->initData.buffer)[i];
				Data[i * 2 + 1] = ((int16*)desc->initData.buffer)[i+1];
				Data[i * 2 + 2] = 0;
				Data[i * 2 + 3] = 0;
			}

			InitData = Data;
			InitDataSize = desc->initData.size * 2;
			Size = desc->resourceDescription.width * 2;
		}

		auto Type = desc->resourceDescription.type;

		bool bInitData = InitData && InitDataSize;
	
		switch (Type)
		{
			case FFX_RESOURCE_TYPE_BUFFER:
			{
#if UE_VERSION_AT_LEAST(5, 0, 0)
				FRHIResourceCreateInfo Info(WCHAR_TO_TCHAR(desc->name));
				TResourceArray<uint8> InitDataResourceArray;
				if (bInitData)
				{
					InitDataResourceArray.AddUninitialized(InitDataSize);
					FMemory::Memcpy(InitDataResourceArray.GetData(), InitData, InitDataSize);
					Info.ResourceArray = &InitDataResourceArray;
				}
				FRDGBufferDesc Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), Size);
#if UE_VERSION_AT_LEAST(5, 3, 0)
				FBufferRHIRef VB = FRHICommandListExecutor::GetImmediateCommandList().CreateBuffer(Size * sizeof(uint32), Desc.Usage, sizeof(uint32), bInitData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initialState), Info);
#else
				FBufferRHIRef VB = RHICreateBuffer(Size * sizeof(uint32), Desc.Usage, sizeof(uint32), Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initialState), Info);
#endif
				check(VB.GetReference());
				TRefCountPtr<FRDGPooledBuffer>* PooledBuffer = new TRefCountPtr<FRDGPooledBuffer>;
				*PooledBuffer = new FRDGPooledBuffer(VB, Desc, desc->resourceDescription.width, WCHAR_TO_TCHAR(desc->name));
#else
				FRDGBufferRef BufferRef = GraphBuilder->CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), Size), WCHAR_TO_TCHAR(desc->name));
				TRefCountPtr<FRDGPooledBuffer>* PooledBuffer = new TRefCountPtr<FRDGPooledBuffer>;
				ConvertToExternalBuffer(*GraphBuilder, BufferRef, *PooledBuffer);
				FVertexBufferRHIRef VB = (*PooledBuffer)->GetVertexBufferRHI();
				check(VB.GetReference());

				if (bInitData)
				{
					void* Dest = RHILockVertexBuffer(VB, 0, desc->resourceDescription.width, EResourceLockMode::RLM_WriteOnly);
					FMemory::Memcpy(Dest, BulkData.Data, FMath::Min(Size, desc->initData.size));
					RHIUnlockVertexBuffer(VB);
				}
#endif
				outTexture->internalIndex = Context->AddResource(VB.GetReference(), desc->resourceDescription.type, nullptr, nullptr, PooledBuffer);
				Context->Resources[outTexture->internalIndex].Desc = desc->resourceDescription;
				Context->Resources[outTexture->internalIndex].Desc.type = Type;
				Context->SetEffectId(outTexture->internalIndex, effectContextId);
				break;
			}
			case FFX_RESOURCE_TYPE_TEXTURE2D:
			{
				
				uint32 NumMips = desc->resourceDescription.mipCount > 0 ? desc->resourceDescription.mipCount : FMath::FloorToInt(FMath::Log2((float)FMath::Max(desc->resourceDescription.width, desc->resourceDescription.height)));
#if UE_VERSION_AT_LEAST(5, 1, 0)
				FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(WCHAR_TO_TCHAR(desc->name), desc->resourceDescription.width, desc->resourceDescription.height, GetUEFormat(desc->resourceDescription.format));
				FFXTextureBulkData BulkData(InitData, InitDataSize);
				if (bInitData) {
					Desc.SetBulkData(&BulkData);
				}
				Desc.SetNumMips(NumMips);
				Desc.SetInitialState(bInitData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initialState));
				Desc.SetNumSamples(1);
				Desc.SetFlags(Flags);
				FTextureRHIRef Texture = RHICreateTexture(Desc);
#else
				FTexture2DRHIRef Texture = RHICreateTexture2D(desc->resourceDescription.width, desc->resourceDescription.height, GetUEFormat(desc->resourceDescription.format), NumMips, 1, Flags, Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initialState), Info);
				Texture->SetName(FName(WCHAR_TO_TCHAR(desc->name)));
#endif

				TRefCountPtr<IPooledRenderTarget>* PooledRT = new TRefCountPtr<IPooledRenderTarget>;
				*PooledRT = CreateRenderTarget(Texture.GetReference(),WCHAR_TO_TCHAR( desc->name));
				outTexture->internalIndex = Context->AddResource(Texture.GetReference(), desc->resourceDescription.type, PooledRT, nullptr, nullptr);
				Context->Resources[outTexture->internalIndex].Desc = desc->resourceDescription;
				Context->Resources[outTexture->internalIndex].Desc.mipCount = NumMips;
				Context->SetEffectId(outTexture->internalIndex, effectContextId);
				break;
			}
			case FFX_RESOURCE_TYPE_TEXTURE3D:
			{
				uint32 NumMips = desc->resourceDescription.mipCount > 0 ? desc->resourceDescription.mipCount : FMath::FloorToInt(FMath::Log2((float)FMath::Max(FMath::Max(desc->resourceDescription.width, desc->resourceDescription.height), desc->resourceDescription.depth)));
#if UE_VERSION_AT_LEAST(5, 1, 0)
				FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create3D(WCHAR_TO_TCHAR(desc->name), desc->resourceDescription.width, desc->resourceDescription.height, desc->resourceDescription.depth, GetUEFormat(desc->resourceDescription.format));
				FFXTextureBulkData BulkData(InitData, InitDataSize);
				if (bInitData) {
					Desc.SetBulkData(&BulkData);
				}
				Desc.SetNumMips(NumMips);
				Desc.SetInitialState(bInitData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initialState));
				Desc.SetNumSamples(1);
				Desc.SetFlags(Flags);
				FTextureRHIRef Texture = RHICreateTexture(Desc);
#else
				FTexture3DRHIRef Texture = RHICreateTexture3D(desc->resourceDescription.width, desc->resourceDescription.height, desc->resourceDescription.depth, GetUEFormat(desc->resourceDescription.format), NumMips, Flags, Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initialState), Info);
				Texture->SetName(FName(WCHAR_TO_TCHAR(desc->name)));
#endif

				TRefCountPtr<IPooledRenderTarget>* PooledRT = new TRefCountPtr<IPooledRenderTarget>;
				*PooledRT = CreateRenderTarget(Texture.GetReference(), WCHAR_TO_TCHAR(desc->name));
				outTexture->internalIndex = Context->AddResource(Texture.GetReference(), desc->resourceDescription.type, PooledRT, nullptr, nullptr);
				Context->Resources[outTexture->internalIndex].Desc = desc->resourceDescription;
				Context->Resources[outTexture->internalIndex].Desc.mipCount = NumMips;
				Context->SetEffectId(outTexture->internalIndex, effectContextId);
				break;
			}
			case FFX_RESOURCE_TYPE_TEXTURE1D:
			default:
			{
				Result = FFX_ERROR_INVALID_ENUM;
				break;
			}
		}

		if (bInitData && (desc->resourceDescription.format == FFX_SURFACE_FORMAT_R16_SNORM || desc->resourceDescription.format == FFX_SURFACE_FORMAT_R16G16_SINT))
		{
			FMemory::Free(InitData);
		}
	}
	else
	{
		Result = FFX_ERROR_INVALID_ARGUMENT;
	}

	return Result;
}

static FfxResourceDescription GetResourceDesc_UE(FfxInterface* backendInterface, FfxResourceInternal resource)
{
	FFXBackendState* backendContext = (FFXBackendState*)backendInterface->scratchBuffer;

	FfxResourceDescription desc = backendContext->Resources[resource.internalIndex].Desc;
	return desc;
}

static FfxErrorCode GetDeviceCapabilities_UE(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities)
{
#if UE_VERSION_AT_LEAST(5, 0, 0)
	if (GetFeatureLevelShaderPlatform(ERHIFeatureLevel::SM6) != SP_NumPlatforms)
	{
		deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_0;
	}
	else
#endif
	{
		deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
	}

	// We are just going to assume no FP16 support and let the compiler do what is needs to
	deviceCapabilities->fp16Supported = false;

	// Only DX12 can tell us what the min & max wave sizes are properly
	if (IsRHIDeviceAMD())
	{
		deviceCapabilities->waveLaneCountMin = 64;
		deviceCapabilities->waveLaneCountMax = 64;
	}
	else
	{
		deviceCapabilities->waveLaneCountMin = 32;
		deviceCapabilities->waveLaneCountMax = 32;
	}

	FString RHIName = GDynamicRHI->GetName();
	if (RHIName == FFXStrings::D3D12)
	{
		deviceCapabilities->waveLaneCountMin = GRHIMinimumWaveSize;
		deviceCapabilities->waveLaneCountMax = GRHIMaximumWaveSize;
		IFFXSharedBackendModule* DX12Backend = FModuleManager::GetModulePtr<IFFXSharedBackendModule>(TEXT("FFXD3D12Backend"));
		if (DX12Backend)
		{
			auto* ApiAccessor = DX12Backend->GetBackend();
			if (ApiAccessor)
			{
				deviceCapabilities->maximumSupportedShaderModel = (FfxShaderModel)ApiAccessor->GetSupportedShaderModel();
				deviceCapabilities->fp16Supported = ApiAccessor->IsFloat16Supported();
			}
		}
	}
	
	// We can rely on the RHI telling us if raytracing is supported
	deviceCapabilities->raytracingSupported = GRHISupportsRayTracing;
	return FFX_OK;
}

static FfxErrorCode CreateDevice_UE(FfxInterface* backendInterface, FfxEffect effect, FfxEffectBindlessConfig* bindlessConfig, FfxUInt32* effectContextId)
{
	FFXBackendState* backendContext = (FFXBackendState*)backendInterface->scratchBuffer;
	if (backendContext->device != backendInterface->device)
	{
		FMemory::Memzero(backendInterface->scratchBuffer, backendInterface->scratchBufferSize);
		for (uint32 i = 0; i < FFX_MAX_BLOCK_COUNT; i++)
		{
			backendContext->Blocks[i].ResourceMask = 0xffffffffffffffff;
		}
		backendContext->device = backendInterface->device;
	}
	if (effectContextId)
	{
		*effectContextId = backendContext->AllocEffect();
	}

	return FFX_OK;
}

static FfxErrorCode ReleaseDevice_UE(FfxInterface* backendInterface, FfxUInt32 effectContextId)
{
	FFXBackendState* backendContext = (FFXBackendState*)backendInterface->scratchBuffer;
	for (int i = 0; i < FFX_RHI_MAX_RESOURCE_COUNT; ++i)
	{
		if (backendContext->IsValidIndex(i) && backendContext->GetEffectId(i) == effectContextId)
		{
			backendContext->RemoveResource(i);
		}
	}
	return FFX_OK;
}

static FfxErrorCode CreatePipeline_UE(FfxInterface* backendInterface, FfxEffect effect, FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* pipelineDescription, FfxUInt32 effectContextId,  FfxPipelineState* outPipeline)
{
	FfxErrorCode Result = FFX_ERROR_INVALID_ARGUMENT;
	FFXBackendState* Context = backendInterface ? (FFXBackendState*)backendInterface->scratchBuffer : nullptr;
	if (Context && pipelineDescription && outPipeline)
	{
		FfxDeviceCapabilities deviceCapabilities;
		GetDeviceCapabilities_UE(backendInterface, &deviceCapabilities);

		bool const bPreferWave64 = (deviceCapabilities.maximumSupportedShaderModel >= FFX_SHADER_MODEL_6_6 && deviceCapabilities.waveLaneCountMin == 32 && deviceCapabilities.waveLaneCountMax == 64);
		outPipeline->pipeline = (FfxPipeline*)GetFFXPass(effect, pass, permutationOptions, pipelineDescription, outPipeline, deviceCapabilities.fp16Supported, bPreferWave64);
		if (outPipeline->pipeline)
		{
			Result = FFX_OK;
		}
	}
	return Result;
}

static FfxErrorCode ScheduleRenderJob_UE(FfxInterface* backendInterface, const FfxGpuJobDescription* job)
{
	FFXBackendState* backendContext = (FFXBackendState*)backendInterface->scratchBuffer;
	backendContext->Jobs[backendContext->NumJobs] = *job;
	if (job->jobType == FFX_GPU_JOB_COMPUTE)
	{
		// needs to copy SRVs and UAVs in case they are on the stack only
		FfxComputeJobDescription* computeJob = &backendContext->Jobs[backendContext->NumJobs].computeJobDescriptor;
		const uint32_t numConstBuffers = job->computeJobDescriptor.pipeline.constCount;
		for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < numConstBuffers; ++currentRootConstantIndex)
		{
			computeJob->cbs[currentRootConstantIndex].num32BitEntries = job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries;
			memcpy(computeJob->cbs[currentRootConstantIndex].data, job->computeJobDescriptor.cbs[currentRootConstantIndex].data, computeJob->cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t));
		}
	}
	backendContext->NumJobs++;

	return FFX_OK;
}

#if UE_VERSION_OLDER_THAN(5, 0, 0)
static bool IsFloatFormat(EPixelFormat Format)
{
	switch (Format)
	{
	case PF_A32B32G32R32F:
	case PF_FloatRGB:
	case PF_FloatRGBA:
	case PF_R32_FLOAT:
	case PF_G16R16F:
	case PF_G16R16F_FILTER:
	case PF_G32R32F:
	case PF_R16F:
	case PF_R16F_FILTER:
	case PF_FloatR11G11B10:
		return true;

	default:
		break;
	}
	return false;
}
#endif

static FfxErrorCode FlushRenderJobs_UE(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId)
{
	FfxErrorCode Result = FFX_OK;
	FFXBackendState* Context = backendInterface ? (FFXBackendState*)backendInterface->scratchBuffer : nullptr;
	FRDGBuilder* GraphBuilder = (FRDGBuilder*)commandList;
	if (Context && GraphBuilder)
	{
		for (uint32 i = 0; i < Context->NumJobs; i++)
		{
			FfxGpuJobDescription* job = &Context->Jobs[i];
			switch (job->jobType)
			{
				case FFX_GPU_JOB_CLEAR_FLOAT:
				{
					FRDGTexture* RdgTex = Context->GetRDGTexture(*GraphBuilder, job->clearJobDescriptor.target.internalIndex);
					if (RdgTex)
					{
						if (IsFloatFormat(RdgTex->Desc.Format))
						{
							for (uint8 MipLevel = 0; MipLevel < RdgTex->Desc.NumMips; MipLevel++)
							{
								FRDGTextureUAVDesc Desc(RdgTex, MipLevel);
								FRDGTextureUAVRef UAV = GraphBuilder->CreateUAV(Desc);
								AddClearUAVPass(*GraphBuilder, UAV, job->clearJobDescriptor.color);
							}
						}
						else
						{
							uint32 UintVector[4];
							FMemory::Memcpy(UintVector, job->clearJobDescriptor.color, sizeof(uint32) * 4);
							for (uint8 MipLevel = 0; MipLevel < RdgTex->Desc.NumMips; MipLevel++)
							{
								FRDGTextureUAVDesc Desc(RdgTex, MipLevel);
								FRDGTextureUAVRef UAV = GraphBuilder->CreateUAV(Desc);
								AddClearUAVPass(*GraphBuilder, UAV, UintVector);
							}
						}
					}
					else
					{
						FRDGBufferUAVRef UAV = GraphBuilder->CreateUAV(Context->GetRDGBuffer(*GraphBuilder, job->clearJobDescriptor.target.internalIndex), PF_R32_FLOAT);
						AddClearUAVFloatPass(*GraphBuilder, UAV, job->clearJobDescriptor.color[0]);
					}
					break;
				}
				case FFX_GPU_JOB_COPY:
				{
					if ((Context->GetType(job->copyJobDescriptor.src.internalIndex) == FFX_RESOURCE_TYPE_BUFFER) && (Context->GetType(job->copyJobDescriptor.dst.internalIndex) == FFX_RESOURCE_TYPE_BUFFER))
					{
						check(false);
					}
					else
					{
						FRDGTexture* SrcRDG = Context->GetRDGTexture(*GraphBuilder, job->copyJobDescriptor.src.internalIndex);
						FRDGTexture* DstRDG = Context->GetRDGTexture(*GraphBuilder, job->copyJobDescriptor.dst.internalIndex);

						FRHICopyTextureInfo Info;
						Info.NumMips = FMath::Min(SrcRDG->Desc.NumMips, DstRDG->Desc.NumMips);
						check(SrcRDG->Desc.Extent.X <= DstRDG->Desc.Extent.X && SrcRDG->Desc.Extent.Y <= DstRDG->Desc.Extent.Y);
						AddCopyTexturePass(*GraphBuilder, SrcRDG, DstRDG, Info);
					}

					break;
				}
				case FFX_GPU_JOB_COMPUTE:
				{
					IFFXRHIBackendSubPass* Pipeline = (IFFXRHIBackendSubPass*)job->computeJobDescriptor.pipeline.pipeline;
					check(Pipeline);
					Pipeline->Dispatch(*GraphBuilder, Context, job);
					break;
				}
				case FFX_GPU_JOB_BARRIER:
				{
					break;
				}
				default:
				{
					Result = FFX_ERROR_INVALID_ENUM;
					break;
				}
			}
		}

		Context->NumJobs = 0;
	}
	else
	{
		Result = FFX_ERROR_INVALID_ARGUMENT;
	}

	return Result;
}

static FfxErrorCode DestroyPipeline_UE(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId)
{
	FfxErrorCode Result = FFX_OK;

	if (pipeline && pipeline->pipeline)
	{
		delete (IFFXRHIBackendSubPass*)pipeline->pipeline;
	}

	return Result;
}

static FfxErrorCode DestroyResource_UE(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId)
{
	FfxErrorCode Result = FFX_OK;
	FFXBackendState* Context = backendInterface ? (FFXBackendState*)backendInterface->scratchBuffer : nullptr;
	if (Context)
	{
		if (Context->IsValidIndex(resource.internalIndex) && Context->GetEffectId(resource.internalIndex) == effectContextId)
		{
			Context->RemoveResource(resource.internalIndex);
		}
		else
		{
			Result = FFX_ERROR_OUT_OF_RANGE;
		}
	}
	else
	{
		Result = FFX_ERROR_INVALID_ARGUMENT;
	}

	return Result;
}

static FfxSurfaceFormat GetFFXFormat(EPixelFormat UEFormat, bool bSRGB)
{
	FfxSurfaceFormat Format = FFX_SURFACE_FORMAT_UNKNOWN;
	switch (UEFormat)
	{
	case PF_R32G32B32A32_UINT:
		Format = FFX_SURFACE_FORMAT_R32G32B32A32_UINT;
		break;
	case PF_A32B32G32R32F:
		Format = FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
		break;
	case PF_FloatRGBA:
		Format = FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
		break;
	case PF_A2B10G10R10:
		Format = FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;
		break;
	case PF_G32R32F:
		Format = FFX_SURFACE_FORMAT_R32G32_FLOAT;
		break;
	case PF_R32_UINT:
		Format = FFX_SURFACE_FORMAT_R32_UINT;
		break;
	case PF_R8G8B8A8_UINT:
		Format = FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
		break;
	case PF_R8G8B8A8:
		if (bSRGB)
		{
			Format = FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
			break;
		}
	case PF_B8G8R8A8:
		Format = FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
		break;
	case PF_FloatR11G11B10:
	case PF_FloatRGB:
		Format = FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
		break;
	case PF_G16R16F:
		Format = FFX_SURFACE_FORMAT_R16G16_FLOAT;
		break;
	case PF_R16G16_UINT:
		Format = FFX_SURFACE_FORMAT_R16G16_UINT;
		break;
	case PF_R16F:
		Format = FFX_SURFACE_FORMAT_R16_FLOAT;
		break;
	case PF_R16_UINT:
		Format = FFX_SURFACE_FORMAT_R16_UINT;
		break;
	case PF_G16:
		Format = FFX_SURFACE_FORMAT_R16_UNORM;
		break;
	case PF_R16G16B16A16_SNORM:
		Format = FFX_SURFACE_FORMAT_R16_SNORM;
		break;
	case PF_R8:
		Format = FFX_SURFACE_FORMAT_R8_UNORM;
		break;
	case PF_R32_FLOAT:
		Format = FFX_SURFACE_FORMAT_R32_FLOAT;
		break;
	case PF_DepthStencil:
		Format = FFX_SURFACE_FORMAT_R32_FLOAT;
		break;
	case PF_R8G8:
		Format = FFX_SURFACE_FORMAT_R8G8_UNORM;
		break;
	case PF_R8_UINT:
		Format = FFX_SURFACE_FORMAT_R8_UINT;
		break;
	case PF_R16G16B16A16_SINT:
		Format = FFX_SURFACE_FORMAT_R16G16_SINT;
		break;
	case PF_A16B16G16R16:
		Format = FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
		break;
	default:
		check(false);
		break;
	}
	return Format;
}

static FfxErrorCode RegisterResource_UE(FfxInterface* backendInterface, const FfxResource* inResource, FfxUInt32 effectContextId, FfxResourceInternal* outResource)
{
	FfxErrorCode Result = FFX_OK;
	FFXBackendState* Context = backendInterface ? (FFXBackendState*)backendInterface->scratchBuffer : nullptr;

	if (backendInterface && inResource && inResource->resource && outResource)
	{
		if (((uintptr_t)inResource->resource) & 0x1)
		{
			switch (inResource->description.type)
			{
			case FFX_RESOURCE_TYPE_BUFFER:
			{
#if UE_VERSION_AT_LEAST(5, 0, 0)
				FRHIBuffer* Buffer = (FRHIBuffer*)((((uintptr_t)inResource->resource) & 0xfffffffffffffffe));
#else
				FRHIResource* Buffer = (FRHIResource*)((((uintptr_t)inResource->resource) & 0xfffffffffffffffe));
#endif
				outResource->internalIndex = Context->AddResource(Buffer, inResource->description.type, nullptr, nullptr, nullptr);
				check(Context->IsValidIndex(outResource->internalIndex));
				Context->MarkDynamic(outResource->internalIndex);
				Context->SetEffectId(outResource->internalIndex, effectContextId);
				Context->Resources[outResource->internalIndex].Desc = inResource->description;
				break;
			}
			case FFX_RESOURCE_TYPE_TEXTURE2D:
			case FFX_RESOURCE_TYPE_TEXTURE3D:
			{
				FRHITexture* Target = (FRHITexture*)((((uintptr_t)inResource->resource) & 0xfffffffffffffffe));
				outResource->internalIndex = Context->AddResource(Target, inResource->description.type, nullptr, nullptr, nullptr);
				check(Context->IsValidIndex(outResource->internalIndex));
				Context->MarkDynamic(outResource->internalIndex);
				Context->SetEffectId(outResource->internalIndex, effectContextId);
				Context->Resources[outResource->internalIndex].Desc = inResource->description;
				break;
			}
			default:
			{
				Result = FFX_ERROR_INVALID_ARGUMENT;
				break;
			}
			}
		}
		else
		{
			FRDGTexture* rdgRes = (FRDGTexture*)inResource->resource;
			auto const& Desc = rdgRes->Desc;
			bool bSRGB = (Desc.Flags & TexCreate_SRGB) == TexCreate_SRGB;
			outResource->internalIndex = Context->AddResource(nullptr, FFX_RESOURCE_TYPE_TEXTURE2D, nullptr, rdgRes, nullptr);
			check(Context->IsValidIndex(outResource->internalIndex));
			Context->MarkDynamic(outResource->internalIndex);
			Context->SetEffectId(outResource->internalIndex, effectContextId);

			Context->Resources[outResource->internalIndex].Desc.type = FFX_RESOURCE_TYPE_TEXTURE2D;
			Context->Resources[outResource->internalIndex].Desc.format = GetFFXFormat(Desc.Format, bSRGB);
			Context->Resources[outResource->internalIndex].Desc.width = Desc.GetSize().X;
			Context->Resources[outResource->internalIndex].Desc.height = Desc.GetSize().Y;
			Context->Resources[outResource->internalIndex].Desc.mipCount = Desc.NumMips;
		}
	}
	else
	{
		Result = FFX_ERROR_INVALID_ARGUMENT;
	}

	return Result;
}

static FfxErrorCode UnregisterResources_UE(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId)
{
	FfxErrorCode Result = backendInterface ? FFX_OK : FFX_ERROR_INVALID_ARGUMENT;
	FFXBackendState* Context = backendInterface ? (FFXBackendState*)backendInterface->scratchBuffer : nullptr;

	for (uint32 i = 0; i < FFX_RHI_MAX_RESOURCE_COUNT; i++)
	{
		if (Context->IsValidIndex(i) && Context->GetEffectId(i) == effectContextId)
		{
			auto& Block = Context->Blocks[i / FFX_MAX_BLOCK_RESOURCE_COUNT];
			if (Block.DynamicMask & (1llu << uint64(i % FFX_MAX_BLOCK_RESOURCE_COUNT)))
			{
				Context->RemoveResource(i);
				check(!(Block.DynamicMask & (1llu << uint64(i % FFX_MAX_BLOCK_RESOURCE_COUNT))));
			}
		}
	}

	return Result;
}

static FfxVersionNumber GetSDKVersion_UE(FfxInterface* backendInterface)
{
	return FFX_SDK_MAKE_VERSION(FFX_SDK_VERSION_MAJOR, FFX_SDK_VERSION_MINOR, FFX_SDK_VERSION_PATCH);
}

static FfxErrorCode GetEffectGpuMemoryUsage_UE(FfxInterface* backendInterface, FfxUInt32 effectContextId, FfxEffectMemoryUsage* outVramUsage)
{
	check(false);
	return FfxErrorCodes::FFX_OK;
}

static FfxErrorCode MapResource_UE(FfxInterface* backendInterface, FfxResourceInternal resource, void** ptr)
{
	check(false);
	return FfxErrorCodes::FFX_OK;
}

static FfxErrorCode UnmapResource_UE(FfxInterface* backendInterface, FfxResourceInternal resource)
{
	check(false);
	return FfxErrorCodes::FFX_OK;
}

static FfxResource GetResource_UE(FfxInterface* backendInterface, FfxResourceInternal resource)
{
	FfxResource Res;
	FMemory::Memzero(Res);

	FFXBackendState* backendContext = (FFXBackendState*)backendInterface->scratchBuffer;
	Res.description = backendContext->Resources[resource.internalIndex].Desc;
	if (backendContext->Resources[resource.internalIndex].Resource)
	{
		Res.resource = (void*)(((uintptr_t)backendContext->Resources[resource.internalIndex].Resource) | 0x1);
	}
	else if (backendContext->Resources[resource.internalIndex].RDG)
	{
		Res.resource = backendContext->Resources[resource.internalIndex].RDG;
	}

	return Res;
}

static FfxErrorCode RegisterStaticResource_UE(FfxInterface* backendInterface, const FfxStaticResourceDescription* desc, FfxUInt32 effectContextId)
{
	check(false);
	return FfxErrorCodes::FFX_OK;
}


static FfxErrorCode StageConstantBufferData_UE(FfxInterface* backendInterface, void* data, FfxUInt32 size, FfxConstantBuffer* constantBuffer)
{
	FfxErrorCode Result = backendInterface ? FFX_OK : FFX_ERROR_INVALID_ARGUMENT;
	FFXBackendState* Context = backendInterface ? (FFXBackendState*)backendInterface->scratchBuffer : nullptr;

	if (!data || !constantBuffer) {
		return FfxErrorCodes::FFX_ERROR_INVALID_POINTER;
	}

	if ((Context->StagingRingBufferBase + FFX_ALIGN_UP(size, 256)) >= FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE)
		Context->StagingRingBufferBase = 0;
	
	uint8* pStaging = (uint8*)&Context->StagingRingBuffer;
	pStaging += Context->StagingRingBufferBase;

	FMemory::Memcpy((void*)pStaging, data, size);

	constantBuffer->data = (uint32_t*)pStaging;
	constantBuffer->num32BitEntries = size / sizeof(uint32_t);

	Context->StagingRingBufferBase += FFX_ALIGN_UP(size, 256);

	return FfxErrorCodes::FFX_OK;
}

static FfxErrorCode BreadcrumbsAllocBlock_UE(FfxInterface* backendInterface, uint64_t blockBytes, FfxBreadcrumbsBlockData* blockData)
{
	check(false);
	return FfxErrorCodes::FFX_OK;
}

static void BreadcrumbsFreeBlock_UE(FfxInterface* backendInterface, FfxBreadcrumbsBlockData* blockData)
{
	check(false);
}

static void BreadcrumbsWrite_UE(FfxInterface* backendInterface, FfxCommandList commandList, uint32_t value, uint64_t gpuLocation, void* gpuBuffer, bool isBegin)
{
	check(false);
}

static void BreadcrumbsPrintDeviceInfo_UE(FfxInterface* backendInterface, FfxAllocationCallbacks* allocs, bool extendedInfo, char** printBuffer, size_t* printSize)
{
	check(false);
}

static FfxErrorCode GetPermutationBlobByIndex_UE(FfxEffect effectId, FfxPass passId, FfxBindStage bindStage, uint32_t permutationOptions, FfxShaderBlob* outBlob)
{
	check(false);
	return FfxErrorCodes::FFX_OK;
}

static FfxErrorCode SetFrameGenerationConfigToSwapchain_UE(FfxFrameGenerationConfig const* config)
{
	return FfxErrorCodes::FFX_OK;
}

static void RegisterConstantBufferAllocator_UE(FfxInterface* backendInterface, FfxConstantBufferAllocator  constantAllocator)
{
	check(false);
}

FfxErrorCode ffxGetInterfaceUE(FfxInterface* outInterface, void* scratchBuffer, size_t scratchBufferSize)
{
	outInterface->fpGetSDKVersion = GetSDKVersion_UE;
	outInterface->fpGetEffectGpuMemoryUsage = GetEffectGpuMemoryUsage_UE;
	outInterface->fpCreateBackendContext = CreateDevice_UE;
	outInterface->fpGetDeviceCapabilities = GetDeviceCapabilities_UE;
	outInterface->fpDestroyBackendContext = ReleaseDevice_UE;
	outInterface->fpCreateResource = CreateResource_UE;
	outInterface->fpDestroyResource = DestroyResource_UE;
	outInterface->fpMapResource = MapResource_UE;
	outInterface->fpUnmapResource = UnmapResource_UE;
	outInterface->fpGetResource = GetResource_UE;
	outInterface->fpRegisterResource = RegisterResource_UE;
	outInterface->fpUnregisterResources = UnregisterResources_UE;
	outInterface->fpRegisterStaticResource = RegisterStaticResource_UE;
	outInterface->fpGetResourceDescription = GetResourceDesc_UE;
	outInterface->fpStageConstantBufferDataFunc = StageConstantBufferData_UE;
	outInterface->fpCreatePipeline = CreatePipeline_UE;
	outInterface->fpDestroyPipeline = DestroyPipeline_UE;
	outInterface->fpScheduleGpuJob = ScheduleRenderJob_UE;
	outInterface->fpExecuteGpuJobs = FlushRenderJobs_UE;

	outInterface->fpBreadcrumbsAllocBlock = BreadcrumbsAllocBlock_UE;
	outInterface->fpBreadcrumbsFreeBlock = BreadcrumbsFreeBlock_UE;
	outInterface->fpBreadcrumbsWrite = BreadcrumbsWrite_UE;
	outInterface->fpBreadcrumbsPrintDeviceInfo = BreadcrumbsPrintDeviceInfo_UE;

	outInterface->fpGetPermutationBlobByIndex = GetPermutationBlobByIndex_UE;
	outInterface->fpSwapChainConfigureFrameGeneration = SetFrameGenerationConfigToSwapchain_UE;
	outInterface->fpRegisterConstantBufferAllocator = RegisterConstantBufferAllocator_UE;

	outInterface->scratchBuffer = scratchBuffer;
	outInterface->scratchBufferSize = scratchBufferSize;
	outInterface->device = (FfxDevice)GDynamicRHI;

	return FFX_OK;
}

size_t ffxGetScratchMemorySizeUE()
{
	return sizeof(FFXBackendState);
}

struct FFXRHIBackendCreateHeader
{
	ffxApiHeader header;
	FfxInterface** interface;
};

ffxReturnCode_t CreateBackend(const ffxCreateContextDescHeader* desc, bool& backendFound, FfxInterface* iface, size_t contexts, Allocator& alloc)
{
	ffxReturnCode_t Code = FFX_API_RETURN_ERROR;
	for (const auto* it = desc->pNext; it; it = it->pNext)
	{
		switch (it->type)
		{
		case FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_RHI:
		{
			// check for double backend just to make sure.
			if (backendFound)
				return FFX_API_RETURN_ERROR;
			backendFound = true;

			size_t scratchBufferSize = ffxGetScratchMemorySizeUE();
			void* scratchBuffer = alloc.alloc(scratchBufferSize);
			memset(scratchBuffer, 0, scratchBufferSize);
			ffxGetInterfaceUE(iface, scratchBuffer, scratchBufferSize);

			FFXRHIBackendCreateHeader const* header = (FFXRHIBackendCreateHeader const*)it;
			if (header && header->interface)
			{
				*header->interface = iface;
			}

			Code = FFX_API_RETURN_OK;
			break;
		}
		default:
			break;
		}
	}
	return Code;
}

void* GetDevice(const ffxApiHeader* desc)
{
	for (const auto* it = desc->pNext; it; it = it->pNext)
	{
		switch (it->type)
		{
		case FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_RHI:
		{
			return (void*)GDynamicRHI;
		}
		default:
			break;
		}
	}
	return nullptr;
}

static constexpr ffxProvider* providers[] = {
	&ffxProvider_FSR3Upscale::Instance,
	&ffxProvider_FrameGeneration::Instance,
};
static constexpr size_t providerCount = _countof(providers);

const ffxProvider* GetffxProvider(ffxStructType_t descType, uint64_t overrideId, void* device)
{
	for (size_t i = 0; i < providerCount; ++i)
	{
		if (providers[i]->GetId() == overrideId || (overrideId == 0 && providers[i]->CanProvide(descType)))
			return providers[i];
	}

	return nullptr;
}

const ffxProvider* GetAssociatedProvider(ffxContext* context)
{
	const InternalContextHeader* hdr = (const InternalContextHeader*)(*context);
	const ffxProvider* provider = hdr->provider;
	return provider;
}

uint64_t GetProviderCount(ffxStructType_t descType, void* device)
{
	return GetProviderVersions(descType, device, UINT64_MAX, nullptr, nullptr);
}

uint64_t GetProviderVersions(ffxStructType_t descType, void* device, uint64_t capacity, uint64_t* versionIds, const char** versionNames)
{
	uint64_t count = 0;

	for (size_t i = 0; i < providerCount; ++i)
	{
		if (count >= capacity) break;
		if (providers[i]->CanProvide(descType))
		{
			auto index = count;
			count++;
			if (versionIds)
				versionIds[index] = providers[i]->GetId();
			if (versionNames)
				versionNames[index] = providers[i]->GetVersionName();
		}
	}

	return count;
}

uint32 FFXBackendState::AllocEffect()
{
	return EffectIndex++;
}

uint32 FFXBackendState::GetEffectId(uint32 Index)
{
	if (IsValidIndex(Index))
	{
		return Resources[Index].EffectId;
	}
	return ~0u;
}

void FFXBackendState::SetEffectId(uint32 Index, uint32 EffectId)
{
	if (IsValidIndex(Index))
	{
		Resources[Index].EffectId = EffectId;
	}
}

uint32 FFXBackendState::AllocIndex()
{
	uint32 Index = ~0u;

	for (uint32 i = 0; i < FFX_MAX_BLOCK_COUNT; i++)
	{
		auto& Block = Blocks[i];
		if (Block.ResourceMask != 0)
		{
			Index = (uint32)FMath::CountTrailingZeros64(Block.ResourceMask);
			check(Index < FFX_MAX_BLOCK_RESOURCE_COUNT);
			Block.ResourceMask &= ~(1llu << uint64(Index));
			Index += (i * FFX_MAX_BLOCK_RESOURCE_COUNT);
			break;
		}
	}

	check(Index < FFX_RHI_MAX_RESOURCE_COUNT);
	return Index;
}

void FFXBackendState::MarkDynamic(uint32 Index)
{
	if (Index < FFX_RHI_MAX_RESOURCE_COUNT)
	{
		auto& Block = Blocks[Index / FFX_MAX_BLOCK_RESOURCE_COUNT];
		Block.DynamicMask |= (1llu << uint64(Index % FFX_MAX_BLOCK_RESOURCE_COUNT));
	}
}

uint32 FFXBackendState::GetDynamicIndex()
{
	uint32 Index = ~0u;

	for (uint32 i = 0; i < FFX_MAX_BLOCK_COUNT; i++)
	{
		auto& Block = Blocks[i];
		if (Block.DynamicMask)
		{
			Index = (uint32)FMath::CountTrailingZeros64(Block.DynamicMask) + (i * FFX_MAX_BLOCK_RESOURCE_COUNT);
			break;
		}
	}

	return Index;
}

bool FFXBackendState::IsValidIndex(uint32 Index)
{
	bool bResult = false;
	if (Index < FFX_RHI_MAX_RESOURCE_COUNT)
	{
		auto& Block = Blocks[Index / FFX_MAX_BLOCK_RESOURCE_COUNT];
		uint32 i = (Index % FFX_MAX_BLOCK_RESOURCE_COUNT);
		uint64 Mask = (1llu << uint64(i));
		bResult = !(Block.ResourceMask & Mask);
	}
	return bResult;
}

void FFXBackendState::FreeIndex(uint32 Index)
{
	check(IsValidIndex(Index));

	if (Index < FFX_RHI_MAX_RESOURCE_COUNT)
	{
		auto& Block = Blocks[Index / FFX_MAX_BLOCK_RESOURCE_COUNT];
		uint32 i = (Index % FFX_MAX_BLOCK_RESOURCE_COUNT);
		uint64 Mask = (1llu << uint64(i));
		Block.DynamicMask &= ~Mask;
		Block.ResourceMask |= Mask;
	}
}

uint32 FFXBackendState::AddResource(FRHIResource* Resource, FfxResourceType Type, TRefCountPtr<IPooledRenderTarget>* RT, FRDGTexture* RDG, TRefCountPtr<FRDGPooledBuffer>* PooledBuffer)
{
	check(Resource || RT || RDG || PooledBuffer);
	uint32 Index = AllocIndex();
	if (Resource)
	{
		Resource->AddRef();
	}
	Resources[Index].Resource = Resource;
	Resources[Index].RT = RT;
	Resources[Index].RDG = RDG;
	Resources[Index].PooledBuffer = PooledBuffer;
	Resources[Index].Desc.type = Type;
	return Index;
}

FRHIResource* FFXBackendState::GetResource(uint32 Index)
{
	FRHIResource* Res = nullptr;
	if (IsValidIndex(Index))
	{
		Res = Resources[Index].Resource;
	}
	return Res;
}

#if UE_VERSION_OLDER_THAN(5, 0, 0)
__declspec(noinline) FRDGTextureRef RegisterExternalTexture(FRDGBuilder& GraphBuilder, FRHITexture* Texture, const TCHAR* NameIfUnregistered)
{
	if (FRDGTextureRef FoundTexture = GraphBuilder.FindExternalTexture(Texture))
	{
		return FoundTexture;
	}

	return GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Texture, NameIfUnregistered));
}
#endif

FRDGTextureRef FFXBackendState::GetOrRegisterExternalTexture(FRDGBuilder& GraphBuilder, uint32 Index)
{
	FRDGTextureRef Texture;
	Texture = GraphBuilder.FindExternalTexture((FRHITexture*)GetResource(Index));
	if (!Texture)
	{
		Texture = GraphBuilder.RegisterExternalTexture(GetPooledRT(Index));
	}
	return Texture;
}

FRDGTexture* FFXBackendState::GetRDGTexture(FRDGBuilder& GraphBuilder, uint32 Index)
{
	FRDGTexture* RDG = nullptr;
	if (IsValidIndex(Index) && Resources[Index].Desc.type != FFX_RESOURCE_TYPE_BUFFER)
	{
		RDG = Resources[Index].RDG;
		if (!RDG && Resources[Index].RT)
		{
			RDG = GetOrRegisterExternalTexture(GraphBuilder, Index);
		}
		else if (!RDG && Resources[Index].Resource)
		{
#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT) && defined(RHI_ENABLE_RESOURCE_INFO) && (RHI_ENABLE_RESOURCE_INFO != 0)
			FRHIResourceInfo Info;
			Resources[Index].Resource->GetResourceInfo(Info);
			RDG = RegisterExternalTexture(GraphBuilder, (FRHITexture*)Resources[Index].Resource, *Info.Name.ToString());
#else
			RDG = RegisterExternalTexture(GraphBuilder, (FRHITexture*)Resources[Index].Resource, nullptr);
#endif
		}
	}
	return RDG;
}

FRDGBufferRef FFXBackendState::GetRDGBuffer(FRDGBuilder& GraphBuilder, uint32 Index)
{
	FRDGBufferRef Buffer = nullptr;
	if (IsValidIndex(Index) && Resources[Index].Desc.type == FFX_RESOURCE_TYPE_BUFFER)
	{
		Buffer = GraphBuilder.RegisterExternalBuffer(*(Resources[Index].PooledBuffer));
	}
	return Buffer;
}

TRefCountPtr<IPooledRenderTarget> FFXBackendState::GetPooledRT(uint32 Index)
{
	TRefCountPtr<IPooledRenderTarget> Res;
	if (IsValidIndex(Index) && Resources[Index].RT)
	{
		Res = *(Resources[Index].RT);
	}
	return Res;
}

FfxResourceType FFXBackendState::GetType(uint32 Index)
{
	FfxResourceType Type = FFX_RESOURCE_TYPE_BUFFER;
	if (IsValidIndex(Index))
	{
		Type = Resources[Index].Desc.type;
	}
	return Type;
}

void FFXBackendState::RemoveResource(uint32 Index)
{
	if (IsValidIndex(Index))
	{
		if (Resources[Index].Resource)
		{
			Resources[Index].Resource->Release();
		}
		if (Resources[Index].RT)
		{
			delete Resources[Index].RT;
		}
		if (Resources[Index].PooledBuffer)
		{
			delete Resources[Index].PooledBuffer;
		}
		Resources[Index].PooledBuffer = nullptr;
		Resources[Index].RDG = nullptr;
		Resources[Index].RT = nullptr;
		Resources[Index].Resource = nullptr;
		FreeIndex(Index);
	}
}

FFXRHIBackend::FFXRHIBackend()
{
}

FFXRHIBackend::~FFXRHIBackend()
{
}

static FfxErrorCode FFXFrameInterpolationUiCompositionCallback(const FfxPresentCallbackDescription* params, void* unusedUserCtx)
{
    return FFX_OK;
}

struct FFXRHIContext
{
	ffxContext Context;
	FfxInterface* Interface;
};

ffxReturnCode_t FFXRHIBackend::ffxCreateContext(ffxContext* context, ffxCreateContextDescHeader* desc)
{
	ffxReturnCode_t Code = FFX_API_RETURN_ERROR;
	FFXRHIBackendCreateHeader RhiHeader;
	RhiHeader.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_RHI;
	RhiHeader.header.pNext = nullptr;
	desc->pNext = &RhiHeader.header;

	FFXRHIContext* ContextWrapper = new FFXRHIContext;
	if (ContextWrapper)
	{
		RhiHeader.interface = &ContextWrapper->Interface;
		Code = ::ffxCreateContext(&ContextWrapper->Context, desc, &AllocCbs.Cbs);
		if (Code == FFX_API_RETURN_OK)
		{
			*context = ContextWrapper;
		}
	}
	return Code;
}

ffxReturnCode_t FFXRHIBackend::ffxDestroyContext(ffxContext* context)
{
	FFXRHIContext* RhiContext = (FFXRHIContext*)(context ? *context : nullptr);
	ffxContext* InnerContext = RhiContext ? &RhiContext->Context : nullptr;
	return ::ffxDestroyContext(InnerContext, &AllocCbs.Cbs);
}

ffxReturnCode_t FFXRHIBackend::ffxConfigure(ffxContext* context, const ffxConfigureDescHeader* desc)
{
	FFXRHIContext* RhiContext = (FFXRHIContext*)(context ? *context : nullptr);
	ffxContext* InnerContext = RhiContext ? &RhiContext->Context : nullptr;
	return ::ffxConfigure(InnerContext, desc);
}

ffxReturnCode_t FFXRHIBackend::ffxQuery(ffxContext* context, ffxQueryDescHeader* desc)
{
	FFXRHIContext* RhiContext = (FFXRHIContext*)(context ? *context : nullptr);
	ffxContext* InnerContext = RhiContext ? &RhiContext->Context : nullptr;
	return ::ffxQuery(InnerContext, desc);
}

ffxReturnCode_t FFXRHIBackend::ffxDispatch(ffxContext* context, const ffxDispatchDescHeader* desc)
{
	FFXRHIContext* RhiContext = (FFXRHIContext*)(context ? *context : nullptr);
	ffxContext* InnerContext = RhiContext ? &RhiContext->Context : nullptr;
	return ::ffxDispatch(InnerContext, desc);
}

void FFXRHIBackend::Init()
{
	static const auto CVarDefaultBackBufferPixelFormat = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DefaultBackBufferPixelFormat"));

	auto Engine = GEngine;
	auto GameViewport = Engine->GameViewport;
	auto Viewport = GameViewport->Viewport;

	if (Viewport->GetViewportRHI().IsValid() && (Viewport->GetViewportRHI()->GetCustomPresent() == nullptr) && (CVarFSR3UseRHI.GetValueOnAnyThread() || FParse::Param(FCommandLine::Get(), TEXT("fsr3rhi"))) && !FParse::Param(FCommandLine::Get(), TEXT("fsr3native")))
	{
		IFFXFrameInterpolationModule* FFXFrameInterpolationModule = FModuleManager::GetModulePtr<IFFXFrameInterpolationModule>(TEXT("FFXFrameInterpolation"));
		check(FFXFrameInterpolationModule);

		IFFXFrameInterpolation* FFXFrameInterpolation = FFXFrameInterpolationModule->GetImpl();
		check(FFXFrameInterpolation);

		uint32 Flags = 0;
		Flags |= bool(ERHIZBuffer::IsInverted) ? FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED : 0;
		Flags |= FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INFINITE;

		EPixelFormat SurfaceFormat = EDefaultBackBufferPixelFormat::Convert2PixelFormat(EDefaultBackBufferPixelFormat::FromInt(CVarDefaultBackBufferPixelFormat->GetValueOnAnyThread()));
		SurfaceFormat = RHIPreferredPixelFormatHint(SurfaceFormat);
		auto SwapChainSize = Viewport->GetSizeXY();
		ENQUEUE_RENDER_COMMAND(FFXFrameInterpolationCreateCustomPresent)([FFXFrameInterpolation, this, Flags, SwapChainSize, SurfaceFormat](FRHICommandListImmediate& RHICmdList)
		{
			auto* CustomPresent = FFXFrameInterpolation->CreateCustomPresent(this, Flags, SwapChainSize, SwapChainSize, (FfxSwapchain)nullptr, (FfxCommandQueue)GDynamicRHI, GetFFXApiFormat(SurfaceFormat, false), EFFXBackendAPI::Unreal);
			if (CustomPresent)
			{
				CustomPresent->InitViewport(GEngine->GameViewport->Viewport, GEngine->GameViewport->Viewport->GetViewportRHI());
			}
		});
	}
}
EFFXBackendAPI FFXRHIBackend::GetAPI() const
{
	return EFFXBackendAPI::Unreal;
}
void FFXRHIBackend::SetFeatureLevel(ffxContext* context, ERHIFeatureLevel::Type FeatureLevel)
{
	FFXRHIContext* RhiContext = (FFXRHIContext*)(context ? *context : nullptr);
	FFXBackendState* Backend = (RhiContext && RhiContext->Interface) ? (FFXBackendState*)RhiContext->Interface->scratchBuffer : nullptr;
	if (Backend)
	{
		Backend->FeatureLevel = FeatureLevel;
	}
}
FfxSwapchain FFXRHIBackend::GetSwapchain(void* swapChain)
{
	return (FfxSwapchain)swapChain;
}
FfxApiResource FFXRHIBackend::GetNativeResource(FRDGTexture* Texture, FfxApiResourceState State)
{
	FfxApiResource resources = {};
	if (Texture)
	{
		auto& Desc = Texture->Desc;
		bool bSRGB = (Desc.Flags & TexCreate_SRGB) == TexCreate_SRGB;
		resources.resource = (void*)Texture;
		resources.state = State;
		resources.description.format = GetFFXFormat(Texture->Desc.Format, bSRGB);
		resources.description.width = Desc.Extent.X;
		resources.description.height = Desc.Extent.Y;
		resources.description.depth = Texture->Desc.Depth;
		resources.description.mipCount = Texture->Desc.NumMips;
		resources.description.flags = FFX_API_RESOURCE_FLAGS_NONE;

		switch (Desc.Dimension)
		{
		case ETextureDimension::Texture2D:
			resources.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
			break;
		case ETextureDimension::Texture2DArray:
			resources.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
			resources.description.depth = Desc.ArraySize;
			break;
		case ETextureDimension::Texture3D:
		case ETextureDimension::TextureCube:
		case ETextureDimension::TextureCubeArray:
		default:
			check(false);
			break;
		}
	}
	return resources;
}
FfxApiResource FFXRHIBackend::GetNativeResource(FRHITexture* Texture, FfxApiResourceState State)
{
	FfxApiResource Result;
	Result.resource = (void*)(((uintptr_t)Texture) | 0x1);
	Result.state = State;
	Result.description.flags = FFX_API_RESOURCE_FLAGS_NONE;

#if UE_VERSION_AT_LEAST(5, 1, 0)
	auto& Desc = Texture->GetDesc();
	bool bSRGB = (Desc.Flags & TexCreate_SRGB) == TexCreate_SRGB;
	Result.description.format = GetFFXFormat(Desc.Format, bSRGB);
	Result.description.width = Desc.Extent.X;
	Result.description.height = Desc.Extent.Y;
	Result.description.depth = Desc.Depth;
	Result.description.mipCount = Desc.NumMips;

	switch (Desc.Dimension)
	{
	case ETextureDimension::Texture2D:
		Result.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
		break;
	case ETextureDimension::Texture2DArray:
		Result.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
		Result.description.depth = Desc.ArraySize;
		break;
	case ETextureDimension::Texture3D:
	case ETextureDimension::TextureCube:
	case ETextureDimension::TextureCubeArray:
	default:
		check(false);
		break;
	}
#else
	auto Size = Texture->GetSizeXYZ();
	bool bSRGB = (Texture->GetFlags() & TexCreate_SRGB) == TexCreate_SRGB;
	Result.description.format = GetFFXFormat(Texture->GetFormat(), bSRGB);
	Result.description.width = Size.X;
	Result.description.height = Size.Y;
	Result.description.depth = Size.Z;
	Result.description.mipCount = Texture->GetNumMips();
	
#if UE_VERSION_AT_LEAST(5, 0, 0)
	switch (Texture->GetType())
	{
	case RRT_Texture2D:
		Result.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
		break;
	case RRT_Texture2DArray:
		Result.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
		Result.description.depth = Size.Z;
		break;
	default:
		check(false);
		break;
	}
#else
	if (Texture->GetTexture2D())
	{
		Result.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
	}
	else if (Texture->GetTexture2DArray())
	{
		Result.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
		Result.description.depth = Size.Z;
	}
	else
	{
		check(false);
	}
#endif
#endif

	return Result;
}
FfxCommandList FFXRHIBackend::GetNativeCommandBuffer(FRHICommandListImmediate& RHICmdList, FRHITexture* Texture)
{
	return (FfxCommandList)&RHICmdList;
}
FfxShaderModel FFXRHIBackend::GetSupportedShaderModel()
{
	FfxShaderModel ShaderModel = FFX_SHADER_MODEL_5_1;
	switch (GMaxRHIFeatureLevel)
	{
#if UE_VERSION_AT_LEAST(5, 0, 0)
		case ERHIFeatureLevel::SM6:
		{
			ShaderModel = FFX_SHADER_MODEL_6_5;
			break;
		}
#endif
		case ERHIFeatureLevel::ES3_1:
		case ERHIFeatureLevel::SM5:
		case ERHIFeatureLevel::ES2_REMOVED:
		case ERHIFeatureLevel::SM4_REMOVED:
		default:
		{
			ShaderModel = FFX_SHADER_MODEL_5_1;
			break;
		}
	}
	return ShaderModel;
}
bool FFXRHIBackend::IsFloat16Supported()
{
	// Needs implementation;
	check(false);
	return false;
}
void FFXRHIBackend::ForceUAVTransition(FRHICommandListImmediate& RHICmdList, FRHITexture* OutputTexture, ERHIAccess Access)
{
	// Deliberately blank
}

void FFXRHIBackend::UpdateSwapChain(ffxContext* Context, ffxConfigureDescFrameGeneration& Desc)
{
	if (Context && Desc.swapChain)
	{
		Desc.swapChain = nullptr;
		Desc.presentCallback = nullptr;

		auto Code = ffxConfigure(Context, &Desc.header);
		check(Code == FFX_API_RETURN_OK);
	}
}

void FFXRHIBackend::UpdateSwapChain(ffxContext* Context, ffxConfigureDescFrameGeneration& Desc, ffxConfigureDescFrameGenerationRegisterDistortionFieldResource& DescDistortion)
{
	Desc.header.pNext = &(DescDistortion.header);
	UpdateSwapChain(Context, Desc);
}

FfxApiResource FFXRHIBackend::GetInterpolationOutput(FfxSwapchain SwapChain)
{
	return { nullptr };
}

void* FFXRHIBackend::GetInterpolationCommandList(FfxSwapchain SwapChain)
{
	return nullptr;
}

void FFXRHIBackend::RegisterFrameResources(FRHIResource* FIResources, uint64 FrameID)
{

}

bool FFXRHIBackend::GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS)
{
	return false;
}

void FFXRHIBackend::CopySubRect(FfxCommandList CmdList, FfxApiResource Src, FfxApiResource Dst, FIntPoint OutputExtents, FIntPoint OutputPoint)
{
	// Deliberately blank
}

void FFXRHIBackend::Flush(FRHITexture* Tex, FRHICommandListImmediate& RHICmdList)
{
	// Deliberately blank
}
