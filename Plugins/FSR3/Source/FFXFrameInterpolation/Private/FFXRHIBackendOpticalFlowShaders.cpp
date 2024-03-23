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

#include "FFXRHIBackendOpticalFlowShaders.h"
#include "FFXRHIBackendSubPass.h"
#include "FFXRHIBackend.h"

#include "FFXOpticalFlowApi.h"

extern IFFXRHIBackendSubPass* GetOpticalFlowComputeLumaPyramidPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetOpticalFlowComputeOpticalFlowAdvPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetOpticalFlowComputeSCDDivergencePass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetOpticalFlowFilterOpticalFlowPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetOpticalFlowPrepareLumaPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetOpticalFlowScaleOpticalFlowAdvPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetOpticalFlowGenSCDHistogramPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFXOpticalFlowParameters, "cbOF");
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFXOpticalFlowSPDParameters, "cbOF_SPD");

IFFXRHIBackendSubPass* GetOpticalFlowPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64)
{
	IFFXRHIBackendSubPass* SubPass = nullptr;
	switch (pass)
	{
		case FFX_OPTICALFLOW_PASS_PREPARE_LUMA:
			SubPass = GetOpticalFlowPrepareLumaPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_OPTICALFLOW_PASS_GENERATE_OPTICAL_FLOW_INPUT_PYRAMID:
			SubPass = GetOpticalFlowComputeLumaPyramidPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_OPTICALFLOW_PASS_COMPUTE_OPTICAL_FLOW_ADVANCED_V5:
			SubPass = GetOpticalFlowComputeOpticalFlowAdvPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_OPTICALFLOW_PASS_FILTER_OPTICAL_FLOW_V5:
			SubPass = GetOpticalFlowFilterOpticalFlowPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_OPTICALFLOW_PASS_SCALE_OPTICAL_FLOW_ADVANCED_V5:
			SubPass = GetOpticalFlowScaleOpticalFlowAdvPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_OPTICALFLOW_PASS_GENERATE_SCD_HISTOGRAM:
			SubPass = GetOpticalFlowGenSCDHistogramPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_OPTICALFLOW_PASS_COMPUTE_SCD_DIVERGENCE:
			SubPass = GetOpticalFlowComputeSCDDivergencePass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		default:
			break;
	}
	return SubPass;
}

FFXRHIBackendRegisterEffect<FFX_EFFECT_OPTICALFLOW, GetOpticalFlowPass> FFXRHIBackendRegisterEffect<FFX_EFFECT_OPTICALFLOW, GetOpticalFlowPass>::sSelf;

bool FFXOpticalFlowGlobalShader::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return FFXGlobalShader::ShouldCompilePermutation(Parameters);
}

void FFXOpticalFlowGlobalShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FPermutationDomain PermutationVector(Parameters.PermutationId);
	bool bUseHalf = PermutationVector.Get<FFX_UseHalf>();
	bool bPreferWave64 = PermutationVector.Get<FFX_PreferWave64>();
	if ((bUseHalf || bPreferWave64) && Parameters.Platform == SP_PCD3D_SM5)
	{
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceDXC);
	}
	OutEnvironment.CompilerFlags.Add(CFLAG_WaveOperations);
	FFXGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment, bPreferWave64);

	OutEnvironment.SetDefine(TEXT("OPTICALFLOW_HDR_COLOR_INPUT"), TEXT("1"));
}

void FFXOpticalFlowGlobalShader::BindParameters(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job, FParameters* Parameters)
{
	for (uint32 i = 0; i < job->computeJobDescriptor.pipeline.constCount; i++)
	{
		switch (job->computeJobDescriptor.pipeline.constantBufferBindings[i].resourceIdentifier)
		{
			case FFX_OPTICALFLOW_CONSTANTBUFFER_IDENTIFIER:
			{
				FFXOpticalFlowParameters Buffer;
				FMemory::Memcpy(&Buffer, job->computeJobDescriptor.cbs[i].data, sizeof(FFXOpticalFlowParameters));
				Parameters->cbOF = TUniformBufferRef<FFXOpticalFlowParameters>::CreateUniformBufferImmediate(Buffer, UniformBuffer_SingleDraw);
				break;
			}
			case FFX_OPTICALFLOW_CONSTANTBUFFER_IDENTIFIER_SPD:
			{
				FFXOpticalFlowSPDParameters Buffer;
				FMemory::Memcpy(&Buffer, job->computeJobDescriptor.cbs[i].data, sizeof(FFXOpticalFlowSPDParameters));
				Parameters->cbOF_SPD = TUniformBufferRef<FFXOpticalFlowSPDParameters>::CreateUniformBufferImmediate(Buffer, UniformBuffer_SingleDraw);
				break;
			}
			default:
			{
				break;
			}
		}
	}

	// Using string comparisons like this is terrible, but the underlying OpticalFlow library relies upon it and keeps actual identifiers private and unusable for this purpose.
	for (uint32 i = 0; i < job->computeJobDescriptor.pipeline.srvTextureCount; i++)
	{
		if (!wcscmp(job->computeJobDescriptor.pipeline.srvTextureBindings[i].name, L"r_input_color"))
		{
			Parameters->r_input_color = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.srvTextureBindings[i].name, L"r_optical_flow_input"))
		{
			Parameters->r_optical_flow_input = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.srvTextureBindings[i].name, L"r_optical_flow_previous_input"))
		{
			Parameters->r_optical_flow_previous_input = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.srvTextureBindings[i].name, L"r_optical_flow"))
		{
			Parameters->r_optical_flow = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.srvTextureBindings[i].name, L"r_optical_flow_previous"))
		{
			Parameters->r_optical_flow_previous = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
		}
	}

	for (uint32 i = 0; i < job->computeJobDescriptor.pipeline.uavTextureCount; i++)
	{
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_input"))
		{
			Parameters->rw_optical_flow_input = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_input_level_1"))
		{
			Parameters->rw_optical_flow_input_level_1 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_input_level_2"))
		{
			Parameters->rw_optical_flow_input_level_2 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_input_level_3"))
		{
			Parameters->rw_optical_flow_input_level_3 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_input_level_4"))
		{
			Parameters->rw_optical_flow_input_level_4 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_input_level_5"))
		{
			Parameters->rw_optical_flow_input_level_5 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_input_level_6"))
		{
			Parameters->rw_optical_flow_input_level_6 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow"))
		{
			Parameters->rw_optical_flow = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_next_level"))
		{
			Parameters->rw_optical_flow_next_level = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_scd_histogram"))
		{
			Parameters->rw_optical_flow_scd_histogram = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_scd_previous_histogram"))
		{
			Parameters->rw_optical_flow_scd_previous_histogram = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_scd_temp"))
		{
			Parameters->rw_optical_flow_scd_temp = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
		if (!wcscmp(job->computeJobDescriptor.pipeline.uavTextureBindings[i].name, L"rw_optical_flow_scd_output"))
		{
			Parameters->rw_optical_flow_scd_output = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
		}
	}

	Parameters->s_LinearClamp = TStaticSamplerState<SF_Bilinear>::GetRHI();
	Parameters->s_PointClamp = TStaticSamplerState<SF_Point>::GetRHI();
}
