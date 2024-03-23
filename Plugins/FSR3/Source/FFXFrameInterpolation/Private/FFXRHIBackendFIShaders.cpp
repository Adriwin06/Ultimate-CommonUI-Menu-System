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

#include "FFXRHIBackendFIShaders.h"
#include "FFXFrameInterpolationApi.h"
#include "FFXRHIBackendSubPass.h"

extern IFFXRHIBackendSubPass* GetFIDisocclusionMaskPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetFIFrameInterpolationPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetFISetupPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetFIReconstructPrevDepthPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetFIGameMotionVectorFieldPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetFIGameVectorFieldInpaintingPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetFIOpticalFlowVectorFieldPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetFIInpaintingPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetFIInpaintingPyramidPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetFIDebugViewPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFXFrameInterpolationParameters, "cbFI");
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFXInpaintingPyramidParameters, "cbInpaintingPyramid");

IFFXRHIBackendSubPass* GetFIPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64)
{
	IFFXRHIBackendSubPass* SubPass = nullptr;
	switch (pass)
	{
	case FFX_FRAMEINTERPOLATION_PASS_SETUP:
		SubPass = GetFISetupPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
		break;
	case FFX_FRAMEINTERPOLATION_PASS_RECONSTRUCT_PREV_DEPTH:
		SubPass = GetFIReconstructPrevDepthPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
		break;
	case FFX_FRAMEINTERPOLATION_PASS_GAME_MOTION_VECTOR_FIELD:
		SubPass = GetFIGameMotionVectorFieldPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
		break;
	case FFX_FRAMEINTERPOLATION_PASS_OPTICAL_FLOW_VECTOR_FIELD:
		SubPass = GetFIOpticalFlowVectorFieldPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
		break;
	case FFX_FRAMEINTERPOLATION_PASS_GAME_VECTOR_FIELD_INPAINTING_PYRAMID:
		SubPass = GetFIGameVectorFieldInpaintingPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
		break;
	case FFX_FRAMEINTERPOLATION_PASS_DISOCCLUSION_MASK:
		SubPass = GetFIDisocclusionMaskPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
		break;
	case FFX_FRAMEINTERPOLATION_PASS_INTERPOLATION:
		SubPass = GetFIFrameInterpolationPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
		break;
	case FFX_FRAMEINTERPOLATION_PASS_INPAINTING_PYRAMID:
		SubPass = GetFIInpaintingPyramidPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
		break;
	case FFX_FRAMEINTERPOLATION_PASS_INPAINTING:
		SubPass = GetFIInpaintingPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
		break;
	case FFX_FRAMEINTERPOLATION_PASS_DEBUG_VIEW:
		SubPass = GetFIDebugViewPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
		break;
	default:
		break;
	}
	return SubPass;
}

FFXRHIBackendRegisterEffect<FFX_EFFECT_FRAMEINTERPOLATION, GetFIPass> FFXRHIBackendRegisterEffect<FFX_EFFECT_FRAMEINTERPOLATION, GetFIPass>::sSelf;

bool FFXFIGlobalShader::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return FFXGlobalShader::ShouldCompilePermutation(Parameters);
}

void FFXFIGlobalShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
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
}

void FFXFIGlobalShader::BindParameters(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job, FParameters* Parameters)
{
	for (uint32 i = 0; i < job->computeJobDescriptor.pipeline.constCount; i++)
	{
		switch (job->computeJobDescriptor.pipeline.constantBufferBindings[i].resourceIdentifier)
		{
			case FFX_FRAMEINTERPOLATION_CONSTANTBUFFER_IDENTIFIER:
			{
				FFXFrameInterpolationParameters Buffer;
				FMemory::Memcpy(&Buffer, job->computeJobDescriptor.cbs[i].data, sizeof(FFXFrameInterpolationParameters));
				Parameters->cbFI = TUniformBufferRef<FFXFrameInterpolationParameters>::CreateUniformBufferImmediate(Buffer, UniformBuffer_SingleDraw);
				break;
			}
			case FFX_FRAMEINTERPOLATION_INPAINTING_PYRAMID_CONSTANTBUFFER_IDENTIFIER:
			{
				FFXInpaintingPyramidParameters Buffer;
				FMemory::Memcpy(&Buffer, job->computeJobDescriptor.cbs[i].data, sizeof(FFXInpaintingPyramidParameters));
				Parameters->cbInpaintingPyramid = TUniformBufferRef<FFXInpaintingPyramidParameters>::CreateUniformBufferImmediate(Buffer, UniformBuffer_SingleDraw);
				break;
			}
			default:
			{
				break;
			}
		}
	}

	for (uint32 i = 0; i < job->computeJobDescriptor.pipeline.srvTextureCount; i++)
	{
		switch (job->computeJobDescriptor.pipeline.srvTextureBindings[i].resourceIdentifier)
		{
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DILATED_DEPTH:
				Parameters->r_dilated_depth = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS:
				Parameters->r_dilated_motion_vectors = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_PREVIOUS_FRAME:
				Parameters->r_reconstructed_depth_previous_frame = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_INTERPOLATED_FRAME:
				Parameters->r_reconstructed_depth_interpolated_frame = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_PREVIOUS_INTERPOLATION_SOURCE:
				Parameters->r_previous_interpolation_source = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_CURRENT_INTERPOLATION_SOURCE:
				Parameters->r_current_interpolation_source = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DISOCCLUSION_MASK:
				Parameters->r_disocclusion_mask = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_X:
				Parameters->r_game_motion_vector_field_x = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_Y:
				Parameters->r_game_motion_vector_field_y = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X:
				Parameters->r_optical_flow_motion_vector_field_x = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y:
				Parameters->r_optical_flow_motion_vector_field_y = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_VECTOR:
				Parameters->r_optical_flow = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_CONFIDENCE:
				Parameters->r_optical_flow_confidence = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_GLOBAL_MOTION:
				Parameters->r_optical_flow_global_motion = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_SCENE_CHANGE_DETECTION:
				Parameters->r_optical_flow_scd = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT:
				Parameters->r_output = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID:
				Parameters->r_inpainting_pyramid = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_PRESENT_BACKBUFFER:
				Parameters->r_present_backbuffer = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNTERS:
				Parameters->r_counters = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			default:
			{
				break;
			}
		}
	}

	for (uint32 i = 0; i < job->computeJobDescriptor.pipeline.uavTextureCount; i++)
	{
		switch (job->computeJobDescriptor.pipeline.uavTextureBindings[i].resourceIdentifier)
		{
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DILATED_DEPTH:
				Parameters->rw_dilated_depth = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS:
				Parameters->rw_dilated_motion_vectors = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_PREVIOUS_FRAME:
				Parameters->rw_reconstructed_depth_previous_frame = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_INTERPOLATED_FRAME:
				Parameters->rw_reconstructed_depth_interpolated_frame = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT:
				Parameters->rw_output = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DISOCCLUSION_MASK:
				Parameters->rw_disocclusion_mask = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_X:
				Parameters->rw_game_motion_vector_field_x = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_Y:
				Parameters->rw_game_motion_vector_field_y = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X:
				Parameters->rw_optical_flow_motion_vector_field_x = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y:
				Parameters->rw_optical_flow_motion_vector_field_y = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNTERS:
				Parameters->rw_counters = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_0:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid0 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_1:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid1 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_2:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid2 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_3:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid3 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_4:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid4 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_5:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid5 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_6:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid6 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_7:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid7 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_8:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid8 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_9:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid9 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_10:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid10 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_11:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid11 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			case FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_12:
			{
				FRDGTextureRef Texture = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex);
				Parameters->rw_inpainting_pyramid12 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, FMath::Min((uint32)job->computeJobDescriptor.uavTextureMips[i], (uint32)Texture->Desc.NumMips - 1)));
				break;
			}
			default:
			{
				break;
			}
		}
	}

	Parameters->s_LinearClamp = TStaticSamplerState<SF_Bilinear>::GetRHI();
}
