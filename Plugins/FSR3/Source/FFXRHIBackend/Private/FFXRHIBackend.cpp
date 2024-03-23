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

#include "FFXRHIBackend.h"
#include "FFXRHIBackendSubPass.h"
#include "../../FFXFrameInterpolation/Public/FFXFrameInterpolationModule.h"
#include "../../FFXFrameInterpolation/Public/IFFXFrameInterpolation.h"
#include "RenderGraphUtils.h"
#include "Engine/RendererSettings.h"
#include "Containers/ResourceArray.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"

#include "FFXShared.h"
#include "FFXFrameInterpolationApi.h"
#include "FFXFSR3Settings.h"

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

static FfxErrorCode CreateResource_UE(FfxInterface* backendInterface, const FfxCreateResourceDescription* desc, FfxUInt32 effectContextId, FfxResourceInternal* outTexture)
{
	FfxErrorCode Result = FFX_OK;
	FFXBackendState* Context = (FFXBackendState*)backendInterface->scratchBuffer;
	
	if (Context)
	{
		ETextureCreateFlags Flags = TexCreate_None;
		Flags |= (desc->resourceDescription.usage & FFX_RESOURCE_USAGE_READ_ONLY) ? TexCreate_ShaderResource : TexCreate_None;
		Flags |= (desc->resourceDescription.usage & FFX_RESOURCE_USAGE_RENDERTARGET) ? TexCreate_RenderTargetable | TexCreate_UAV | TexCreate_ShaderResource : TexCreate_None;
		Flags |= (desc->resourceDescription.usage & FFX_RESOURCE_USAGE_UAV) ? TexCreate_UAV | TexCreate_ShaderResource : TexCreate_None;
		Flags |= desc->resourceDescription.format == FFX_SURFACE_FORMAT_R8G8B8A8_SRGB ? TexCreate_SRGB : TexCreate_None;
				
		FRHIResourceCreateInfo Info(WCHAR_TO_TCHAR(desc->name));

		uint32 Size = desc->resourceDescription.width;
		FFXTextureBulkData BulkData(desc->initData, desc->initDataSize);
		if (desc->resourceDescription.format == FFX_SURFACE_FORMAT_R16_SNORM && desc->initData)
		{
			int16* Data = (int16*)FMemory::Malloc(desc->initDataSize * 4);
			for (uint32 i = 0; i < (desc->initDataSize / sizeof(int16)); i++)
			{
				Data[i * 4] = ((int16*)desc->initData)[i];
				Data[i * 4 + 1] = 0;
				Data[i * 4 + 2] = 0;
				Data[i * 4 + 3] = 0;
			}

			BulkData.Data = Data;
			BulkData.DataSize = desc->initDataSize * 4;
			Size = desc->resourceDescription.width * 4;
		}
		else if (desc->resourceDescription.format == FFX_SURFACE_FORMAT_R16G16_SINT && desc->initData)
		{
			int16* Data = (int16*)FMemory::Malloc(desc->initDataSize * 2);
			for (uint32 i = 0; i < (desc->initDataSize / (sizeof(int16) * 2)); i+=2)
			{
				Data[i * 2] = ((int16*)desc->initData)[i];
				Data[i * 2 + 1] = ((int16*)desc->initData)[i+1];
				Data[i * 2 + 2] = 0;
				Data[i * 2 + 3] = 0;
			}

			BulkData.Data = Data;
			BulkData.DataSize = desc->initDataSize * 2;
			Size = desc->resourceDescription.width * 2;
		}

		auto Type = desc->resourceDescription.type;

		Info.BulkData = desc->initData && desc->initDataSize ? &BulkData : nullptr;

		switch (Type)
		{
			case FFX_RESOURCE_TYPE_BUFFER:
			{
				FRDGBufferDesc Desc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), Size);
#if UE_VERSION_AT_LEAST(5, 3, 0)
				FBufferRHIRef VB = FRHICommandListExecutor::GetImmediateCommandList().CreateBuffer(Size, Desc.Usage, sizeof(uint32), Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initalState), Info);
#else
				FBufferRHIRef VB = RHICreateBuffer(Size, Desc.Usage, sizeof(uint32), Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initalState), Info);
#endif
				check(VB.GetReference());
				TRefCountPtr<FRDGPooledBuffer>* PooledBuffer = new TRefCountPtr<FRDGPooledBuffer>;
				*PooledBuffer = new FRDGPooledBuffer(VB, Desc, desc->resourceDescription.width, WCHAR_TO_TCHAR(desc->name));
				if (Info.BulkData)
				{
#if UE_VERSION_AT_LEAST(5, 3, 0)
					void* Dest = FRHICommandListExecutor::GetImmediateCommandList().LockBuffer(VB, 0, desc->resourceDescription.width, EResourceLockMode::RLM_WriteOnly);
#else
					void* Dest = RHILockBuffer(VB, 0, desc->resourceDescription.width, EResourceLockMode::RLM_WriteOnly);
#endif
					FMemory::Memcpy(Dest, BulkData.Data, FMath::Min(Size, desc->initDataSize));
#if UE_VERSION_AT_LEAST(5, 3, 0)
					FRHICommandListExecutor::GetImmediateCommandList().UnlockBuffer(VB);
#else
					RHIUnlockBuffer(VB);
#endif
				}

				outTexture->internalIndex = Context->AddResource(VB.GetReference(), desc->resourceDescription.type, nullptr, nullptr, PooledBuffer);
				Context->Resources[outTexture->internalIndex].Desc = desc->resourceDescription;
				Context->Resources[outTexture->internalIndex].Desc.type = Type;
				Context->SetEffectId(outTexture->internalIndex, effectContextId);
				break;
			}
			case FFX_RESOURCE_TYPE_TEXTURE2D:
			{
				uint32 NumMips = desc->resourceDescription.mipCount > 0 ? desc->resourceDescription.mipCount : FMath::FloorToInt(FMath::Log2((float)FMath::Max(desc->resourceDescription.width, desc->resourceDescription.height)));
				FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(WCHAR_TO_TCHAR(desc->name), desc->resourceDescription.width, desc->resourceDescription.height, GetUEFormat(desc->resourceDescription.format));
				Desc.SetBulkData(Info.BulkData);
				Desc.SetNumMips(NumMips);
				Desc.SetInitialState(Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initalState));
				Desc.SetNumSamples(1);
				Desc.SetFlags(Flags);
				FTextureRHIRef Texture = RHICreateTexture(Desc);

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
				FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create3D(WCHAR_TO_TCHAR(desc->name), desc->resourceDescription.width, desc->resourceDescription.height, desc->resourceDescription.depth, GetUEFormat(desc->resourceDescription.format));
				Desc.SetBulkData(Info.BulkData);
				Desc.SetNumMips(NumMips);
				Desc.SetInitialState(Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initalState));
				Desc.SetNumSamples(1);
				Desc.SetFlags(Flags);
				FTextureRHIRef Texture = RHICreateTexture(Desc);
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

		if (desc->resourceDescription.format == FFX_SURFACE_FORMAT_R16_SNORM && Info.BulkData)
		{
			FMemory::Free(const_cast<void*>(BulkData.Data));
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
	if (GetFeatureLevelShaderPlatform(ERHIFeatureLevel::SM6) != SP_NumPlatforms)
	{
		deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_0;
	}
	else
	{
		deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
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
				deviceCapabilities->minimumSupportedShaderModel = (FfxShaderModel)ApiAccessor->GetSupportedShaderModel();
				deviceCapabilities->fp16Supported = ApiAccessor->IsFloat16Supported();
			}
		}
	}
	
	// We can rely on the RHI telling us if raytracing is supported
	deviceCapabilities->raytracingSupported = GRHISupportsRayTracing;
	return FFX_OK;
}

static FfxErrorCode CreateDevice_UE(FfxInterface* backendInterface, FfxUInt32* effectContextId)
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

		bool const bPreferWave64 = (deviceCapabilities.minimumSupportedShaderModel >= FFX_SHADER_MODEL_6_6 && deviceCapabilities.waveLaneCountMin == 32 && deviceCapabilities.waveLaneCountMax == 64);
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

static FfxErrorCode FlushRenderJobs_UE(FfxInterface* backendInterface, FfxCommandList commandList)
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
						FRDGTextureUAVRef UAV = GraphBuilder->CreateUAV(RdgTex);
						if (IsFloatFormat(RdgTex->Desc.Format))
						{
							AddClearUAVPass(*GraphBuilder, UAV, job->clearJobDescriptor.color);
						}
						else
						{
							uint32 UintVector[4];
							FMemory::Memcpy(UintVector, job->clearJobDescriptor.color, sizeof(uint32) * 4);
							AddClearUAVPass(*GraphBuilder, UAV, UintVector);
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
				FRHIBuffer* Buffer = (FRHIBuffer*)((((uintptr_t)inResource->resource) & 0xfffffffffffffffe));
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

static FfxUInt32 GetSDKVersion_UE(FfxInterface* backendInterface)
{
	return FFX_SDK_MAKE_VERSION(FFX_SDK_VERSION_MAJOR, FFX_SDK_VERSION_MINOR, FFX_SDK_VERSION_PATCH);
}

FfxErrorCode ffxGetInterfaceUE(FfxInterface* outInterface, void* scratchBuffer, size_t scratchBufferSize)
{
	outInterface->fpGetSDKVersion = GetSDKVersion_UE;
	outInterface->fpCreateBackendContext = CreateDevice_UE;
	outInterface->fpGetDeviceCapabilities = GetDeviceCapabilities_UE;
	outInterface->fpDestroyBackendContext = ReleaseDevice_UE;
	outInterface->fpCreateResource = CreateResource_UE;
	outInterface->fpRegisterResource = RegisterResource_UE;
	outInterface->fpUnregisterResources = UnregisterResources_UE;
	outInterface->fpGetResourceDescription = GetResourceDesc_UE;
	outInterface->fpDestroyResource = DestroyResource_UE;
	outInterface->fpCreatePipeline = CreatePipeline_UE;
	outInterface->fpDestroyPipeline = DestroyPipeline_UE;
	outInterface->fpScheduleGpuJob = ScheduleRenderJob_UE;
	outInterface->fpExecuteGpuJobs = FlushRenderJobs_UE;
	outInterface->scratchBuffer = scratchBuffer;
	outInterface->scratchBufferSize = scratchBufferSize;
	outInterface->device = (FfxDevice)GDynamicRHI;

	return FFX_OK;
}

size_t ffxGetScratchMemorySizeUE()
{
	return sizeof(FFXBackendState);
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
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
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

static FfxErrorCode FFXFrameInterpolationUiCompositionCallback(const FfxPresentCallbackDescription* params)
{
    return FFX_OK;
}

void FFXRHIBackend::Init()
{
	static const auto CVarDefaultBackBufferPixelFormat = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DefaultBackBufferPixelFormat"));

	auto Engine = GEngine;
	auto GameViewport = Engine->GameViewport;
	auto Viewport = GameViewport->Viewport;

	if (Viewport->GetViewportRHI().IsValid() && (Viewport->GetViewportRHI()->GetCustomPresent() == nullptr) && CVarFSR3UseRHI.GetValueOnAnyThread() && !FParse::Param(FCommandLine::Get(), TEXT("fsr3native")))
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
			auto* CustomPresent = FFXFrameInterpolation->CreateCustomPresent(this, Flags, SwapChainSize, SwapChainSize, (FfxSwapchain)nullptr, (FfxCommandQueue)GDynamicRHI, GetFFXFormat(SurfaceFormat, false), &FFXFrameInterpolationUiCompositionCallback);
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
void FFXRHIBackend::SetFeatureLevel(FfxInterface& Interface, ERHIFeatureLevel::Type FeatureLevel)
{
	FFXBackendState* Backend = (FFXBackendState*)Interface.scratchBuffer;
	if (Backend)
	{
		Backend->FeatureLevel = FeatureLevel;
	}
}
size_t FFXRHIBackend::GetGetScratchMemorySize()
{
	return sizeof(FFXBackendState);
}
FfxErrorCode FFXRHIBackend::CreateInterface(FfxInterface& OutInterface, uint32 MaxContexts)
{
	FfxErrorCode Code = FFX_OK;
	if (OutInterface.device == nullptr)
	{
		size_t InscratchBufferSize = GetGetScratchMemorySize();
		void* InScratchBuffer = FMemory::Malloc(InscratchBufferSize);
		Code = ffxGetInterfaceUE(&OutInterface, InScratchBuffer, InscratchBufferSize);
		if (Code != FFX_OK)
		{
			FMemory::Free(InScratchBuffer);
			FMemory::Memzero(OutInterface);
		}
	}
	else
	{
		Code = FFX_ERROR_INVALID_ARGUMENT;
	}
	return Code;
}
FfxDevice FFXRHIBackend::GetDevice(void* device)
{
	return (FfxDevice)device;
}
FfxCommandList FFXRHIBackend::GetCommandList(void* list)
{
	return (FfxCommandList)list;
}
FfxResource FFXRHIBackend::GetResource(void* resource, wchar_t* name, FfxResourceStates state, uint32 shaderComponentMapping)
{
	check(false);
	FfxResource Result = GetNativeResource((FRHITexture*)resource, state);
	return Result;
}
FfxCommandQueue FFXRHIBackend::GetCommandQueue(void* cmdQueue)
{
	return (FfxCommandQueue)cmdQueue;
}
FfxSwapchain FFXRHIBackend::GetSwapchain(void* swapChain)
{
	return (FfxSwapchain)swapChain;
}
FfxDevice FFXRHIBackend::GetNativeDevice()
{
	return (FfxDevice)GDynamicRHI;
}
FfxResource FFXRHIBackend::GetNativeResource(FRDGTexture* Texture, FfxResourceStates State)
{
	FfxResource resources = {};
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
		resources.description.flags = FFX_RESOURCE_FLAGS_NONE;

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
			resources.description.type = FFX_RESOURCE_TYPE_TEXTURE3D;
			break;
		case ETextureDimension::TextureCube:
		case ETextureDimension::TextureCubeArray:
		default:
			check(false);
			break;
		}
	}
	return resources;
}
FfxResource FFXRHIBackend::GetNativeResource(FRHITexture* Texture, FfxResourceStates State)
{
	FfxResource Result;
	auto& Desc = Texture->GetDesc();
	bool bSRGB = (Desc.Flags & TexCreate_SRGB) == TexCreate_SRGB;
	Result.resource = (void*)(((uintptr_t)Texture) | 0x1);
	Result.state = State;

	Result.description.format = GetFFXFormat(Desc.Format, bSRGB);
	Result.description.width = Desc.Extent.X;
	Result.description.height = Desc.Extent.Y;
	Result.description.depth = Desc.Depth;
	Result.description.mipCount = Desc.NumMips;
	Result.description.flags = FFX_RESOURCE_FLAGS_NONE;

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
		Result.description.type = FFX_RESOURCE_TYPE_TEXTURE3D;
		break;
	case ETextureDimension::TextureCube:
	case ETextureDimension::TextureCubeArray:
	default:
		check(false);
		break;
	}

	return Result;
}
FfxCommandList FFXRHIBackend::GetNativeCommandBuffer(FRHICommandListImmediate& RHICmdList)
{
	return (FfxCommandList)&RHICmdList;
}
uint32 FFXRHIBackend::GetNativeTextureFormat(FRHITexture* Texture)
{
	return Texture->GetDesc().Format;
}
FfxShaderModel FFXRHIBackend::GetSupportedShaderModel()
{
	FfxShaderModel ShaderModel = FFX_SHADER_MODEL_5_1;
	switch (GMaxRHIFeatureLevel)
	{
		case ERHIFeatureLevel::SM6:
		{
			ShaderModel = FFX_SHADER_MODEL_6_5;
			break;
		}
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

void FFXRHIBackend::UpdateSwapChain(FfxInterface& Interface, void* SwapChain, bool mode, bool allowAsyncWorkloads, bool showDebugView)
{
	// Deliberately blank
}

FfxResource FFXRHIBackend::GetInterpolationOutput(FfxSwapchain SwapChain)
{
	return { nullptr };
}

FfxCommandList FFXRHIBackend::GetInterpolationCommandList(FfxSwapchain SwapChain)
{
	return nullptr;
}

void FFXRHIBackend::BindUITexture(FfxSwapchain gameSwapChain, FfxResource uiResource)
{

}

void FFXRHIBackend::RegisterFrameResources(FRHIResource* FIResources, IRefCountedObject* FSR3Resources)
{

}

FFXSharedResource FFXRHIBackend::CreateResource(FfxInterface& Interface, const FfxCreateResourceDescription* desc)
{
	FFXSharedResource Result = {};
	ETextureCreateFlags Flags = TexCreate_None;
	Flags |= (desc->resourceDescription.usage & FFX_RESOURCE_USAGE_READ_ONLY) ? TexCreate_ShaderResource : TexCreate_None;
	Flags |= (desc->resourceDescription.usage & FFX_RESOURCE_USAGE_RENDERTARGET) ? TexCreate_RenderTargetable | TexCreate_UAV | TexCreate_ShaderResource : TexCreate_None;
	Flags |= (desc->resourceDescription.usage & FFX_RESOURCE_USAGE_UAV) ? TexCreate_UAV | TexCreate_ShaderResource : TexCreate_None;
	Flags |= desc->resourceDescription.format == FFX_SURFACE_FORMAT_R8G8B8A8_SRGB ? TexCreate_SRGB : TexCreate_None;

	FRHIResourceCreateInfo Info(WCHAR_TO_TCHAR(desc->name));

	uint32 Size = desc->resourceDescription.width;
	FFXTextureBulkData BulkData(desc->initData, desc->initDataSize);
	if (desc->resourceDescription.format == FFX_SURFACE_FORMAT_R16_SNORM && desc->initData)
	{
		int16* Data = (int16*)FMemory::Malloc(desc->initDataSize * 4);
		for (uint32 i = 0; i < (desc->initDataSize / sizeof(int16)); i++)
		{
			Data[i * 4] = ((int16*)desc->initData)[i];
			Data[i * 4 + 1] = 0;
			Data[i * 4 + 2] = 0;
			Data[i * 4 + 3] = 0;
		}

		BulkData.Data = Data;
		BulkData.DataSize = desc->initDataSize * 4;
		Size = desc->resourceDescription.width * 4;
	}
	else if (desc->resourceDescription.format == FFX_SURFACE_FORMAT_R16G16_SINT && desc->initData)
	{
		int16* Data = (int16*)FMemory::Malloc(desc->initDataSize * 2);
		for (uint32 i = 0; i < (desc->initDataSize / (sizeof(int16) * 2)); i += 2)
		{
			Data[i * 2] = ((int16*)desc->initData)[i];
			Data[i * 2 + 1] = ((int16*)desc->initData)[i + 1];
			Data[i * 2 + 2] = 0;
			Data[i * 2 + 3] = 0;
		}

		BulkData.Data = Data;
		BulkData.DataSize = desc->initDataSize * 2;
		Size = desc->resourceDescription.width * 2;
	}

	auto Type = desc->resourceDescription.type;

	Info.BulkData = desc->initData && desc->initDataSize ? &BulkData : nullptr;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (desc->name)
	{
		FCStringWide::Strcpy(Result.Resource.name, 63, desc->name);
	}
#endif

	switch (Type)
	{
	case FFX_RESOURCE_TYPE_BUFFER:
	{
		FRDGBufferDesc Desc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), Size);
#if UE_VERSION_AT_LEAST(5, 3, 0)
		FBufferRHIRef VB = FRHICommandListExecutor::GetImmediateCommandList().CreateBuffer(Size, Desc.Usage, sizeof(uint32), Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initalState), Info);
#else
		FBufferRHIRef VB = RHICreateBuffer(Size, Desc.Usage, sizeof(uint32), Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initalState), Info);
#endif
		check(VB.GetReference());
		if (Info.BulkData)
		{
#if UE_VERSION_AT_LEAST(5, 3, 0)
			void* Dest = FRHICommandListExecutor::GetImmediateCommandList().LockBuffer(VB, 0, desc->resourceDescription.width, EResourceLockMode::RLM_WriteOnly);
#else
			void* Dest = RHILockBuffer(VB, 0, desc->resourceDescription.width, EResourceLockMode::RLM_WriteOnly);
#endif
			FMemory::Memcpy(Dest, BulkData.Data, FMath::Min(Size, desc->initDataSize));
#if UE_VERSION_AT_LEAST(5, 3, 0)
			FRHICommandListExecutor::GetImmediateCommandList().UnlockBuffer(VB);
#else
			RHIUnlockBuffer(VB);
#endif
		}
		VB->AddRef();
		Result.Resource.resource = (void*)(((uintptr_t)VB.GetReference()) | 0x1);
		Result.Resource.state = desc->initalState;
		Result.Resource.description = desc->resourceDescription;
		break;
	}
	case FFX_RESOURCE_TYPE_TEXTURE2D:
	{
		uint32 NumMips = desc->resourceDescription.mipCount > 0 ? desc->resourceDescription.mipCount : FMath::FloorToInt(FMath::Log2((float)FMath::Max(desc->resourceDescription.width, desc->resourceDescription.height)));
		FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(WCHAR_TO_TCHAR(desc->name), desc->resourceDescription.width, desc->resourceDescription.height, GetUEFormat(desc->resourceDescription.format));
		Desc.SetBulkData(Info.BulkData);
		Desc.SetNumMips(NumMips);
		Desc.SetInitialState(Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initalState));
		Desc.SetNumSamples(1);
		Desc.SetFlags(Flags);
		FTextureRHIRef Texture = RHICreateTexture(Desc);
		Texture->AddRef();
		Result.Resource = GetNativeResource(Texture.GetReference(), Info.BulkData ? FFX_RESOURCE_STATE_COMPUTE_READ : desc->initalState);
		break;
	}
	case FFX_RESOURCE_TYPE_TEXTURE3D:
	{
		uint32 NumMips = desc->resourceDescription.mipCount > 0 ? desc->resourceDescription.mipCount : FMath::FloorToInt(FMath::Log2((float)FMath::Max(FMath::Max(desc->resourceDescription.width, desc->resourceDescription.height), desc->resourceDescription.depth)));
		FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create3D(WCHAR_TO_TCHAR(desc->name), desc->resourceDescription.width, desc->resourceDescription.height, desc->resourceDescription.depth, GetUEFormat(desc->resourceDescription.format));
		Desc.SetBulkData(Info.BulkData);
		Desc.SetNumMips(NumMips);
		Desc.SetInitialState(Info.BulkData ? ERHIAccess::SRVCompute : GetUEAccessState(desc->initalState));
		Desc.SetNumSamples(1);
		Desc.SetFlags(Flags);
		FTextureRHIRef Texture = RHICreateTexture(Desc);
		Texture->AddRef();
		Result.Resource = GetNativeResource(Texture.GetReference(), Info.BulkData ? FFX_RESOURCE_STATE_COMPUTE_READ : desc->initalState);
		break;
	}
	case FFX_RESOURCE_TYPE_TEXTURE1D:
	default:
	{
		break;
	}
	}

	if (desc->resourceDescription.format == FFX_SURFACE_FORMAT_R16_SNORM && Info.BulkData)
	{
		FMemory::Free(const_cast<void*>(BulkData.Data));
	}

	return Result;
}

FfxErrorCode FFXRHIBackend::ReleaseResource(FfxInterface& Interface, FFXSharedResource Resource)
{
	FfxErrorCode Result = FFX_ERROR_INVALID_ARGUMENT;
	if (((uintptr_t)Resource.Resource.resource) & 0x1)
	{
		switch (Resource.Resource.description.type)
		{
		case FFX_RESOURCE_TYPE_BUFFER:
		{
			FRHIBuffer* Buffer = (FRHIBuffer*)((void*)(((uintptr_t)Resource.Resource.resource) & 0xfffffffffffffffe));
			Buffer->Release();
			Result = FFX_OK;
			break;
		}
		case FFX_RESOURCE_TYPE_TEXTURE2D:
		case FFX_RESOURCE_TYPE_TEXTURE3D:
		{
			FRHITexture* Texture = (FRHITexture*)((void*)(((uintptr_t)Resource.Resource.resource) & 0xfffffffffffffffe));
			Texture->Release();
			Result = FFX_OK;
			break;
		}
		case FFX_RESOURCE_TYPE_TEXTURE1D:
		default:
			break;
		}
	}
	return Result;
}

bool FFXRHIBackend::GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS)
{
	return false;
}

void FFXRHIBackend::CopySubRect(FfxCommandList CmdList, FfxResource Src, FfxResource Dst, FIntPoint OutputExtents, FIntPoint OutputPoint)
{
	// Deliberately blank
}
