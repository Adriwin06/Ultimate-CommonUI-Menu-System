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

#include "FFXRHIBackend.h"
#include "RHIResources.h"
#include "RendererInterface.h"
#include "RenderGraphDefinitions.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"

//-------------------------------------------------------------------------------------
// Each FFX subpass implements this common interface to hide the internal details
//-------------------------------------------------------------------------------------
class IFFXRHIBackendSubPass
{
public:
	virtual ~IFFXRHIBackendSubPass() {}
	virtual void SetupPipeline(const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf) = 0;
	virtual void Dispatch(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job) = 0;
};

//-------------------------------------------------------------------------------------
// The TFFXRHIBackendSubPass template reduces the boilerplate required for each pass.
// The template handles binding resources & issuing the pass through RDG.
//-------------------------------------------------------------------------------------
template <typename TShaderClass>
class TFFXRHIBackendSubPass : public IFFXRHIBackendSubPass
{
	TCHAR const* Name;

public:
	typename TShaderClass::FPermutationDomain Permutation;

	TFFXRHIBackendSubPass(TCHAR const* InName, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf)
		: Name(InName)
	{
		SetupPipeline(desc, outPipeline, bSupportHalf);
	}

	virtual ~TFFXRHIBackendSubPass()
	{

	}

	void SetupPipeline(const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf) override
	{
		outPipeline->srvTextureCount = TShaderClass::GetNumBoundSRVs();
		outPipeline->uavTextureCount = TShaderClass::GetNumBoundUAVs();
		outPipeline->constCount = TShaderClass::GetNumConstants();

		for (uint32 i = 0; i < outPipeline->srvTextureCount && i < FFX_MAX_NUM_SRVS; i++)
		{
			outPipeline->srvTextureBindings[i].slotIndex = i;
			outPipeline->srvTextureBindings[i].resourceIdentifier = TShaderClass::GetBoundSRVs()[i];
			FCStringWide::Strcpy(outPipeline->srvTextureBindings[i].name, 63, TShaderClass::GetBoundSRVNames()[i]);
		}

		for (uint32 i = 0; i < outPipeline->uavTextureCount && i < FFX_MAX_NUM_UAVS; i++)
		{
			outPipeline->uavTextureBindings[i].slotIndex = i;
			outPipeline->uavTextureBindings[i].resourceIdentifier = TShaderClass::GetBoundUAVs()[i];
			FCStringWide::Strcpy(outPipeline->uavTextureBindings[i].name, 63, TShaderClass::GetBoundUAVNames()[i]);
		}

		for (uint32 i = 0; i < outPipeline->constCount && i < FFX_MAX_NUM_CONST_BUFFERS; i++)
		{
			outPipeline->constantBufferBindings[i].slotIndex = i;
			outPipeline->constantBufferBindings[i].resourceIdentifier = TShaderClass::GetBoundCBs()[i];
			FCStringWide::Strcpy(outPipeline->constantBufferBindings[i].name, 63, TShaderClass::GetBoundCBNames()[i]);
		}
	}

	void Dispatch(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job) override
	{
		TShaderMapRef<TShaderClass> ComputeShader(GetGlobalShaderMap(Context->FeatureLevel), Permutation);

		FIntVector DispatchCount(job->computeJobDescriptor.dimensions[0], job->computeJobDescriptor.dimensions[1], job->computeJobDescriptor.dimensions[2]);
		typename TShaderClass::FParameters* Parameters = GraphBuilder.AllocParameters<typename TShaderClass::FParameters>();
		TShaderClass::BindParameters(GraphBuilder, Context, job, Parameters);

		ClearUnusedGraphResources(ComputeShader, Parameters);

		GraphBuilder.AddPass(
			FRDGEventName(Name),
			Parameters,
			ERDGPassFlags::Compute,
			[Parameters, ComputeShader, DispatchCount](FRHIComputeCommandList& RHICmdList)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *Parameters, DispatchCount);
			});
	}
};

//-------------------------------------------------------------------------------------
// These functions instantiate the actual passes.
//-------------------------------------------------------------------------------------
typedef IFFXRHIBackendSubPass* (*CreateFFXPassFunc)(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription*, FfxPipelineState*, bool, bool);
extern FFXRHIBACKEND_API void RegisterFFXPass(FfxEffect Effect, CreateFFXPassFunc Func);

template<FfxEffect Effect, CreateFFXPassFunc Func>
class FFXRHIBackendRegisterEffect
{
	static FFXRHIBackendRegisterEffect sSelf;
public:
	FFXRHIBackendRegisterEffect()
	{
		RegisterFFXPass(Effect, Func);
	}
};

extern FFXRHIBACKEND_API IFFXRHIBackendSubPass* GetFFXPass(FfxEffect effect, FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
