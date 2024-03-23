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

#include "FFXRHIBackendFSRShaders.h"
#include "FFXRHIBackendSubPass.h"
#include "ShaderCompilerCore.h"

#include "FFXFSR3.h"

extern IFFXRHIBackendSubPass* GetDepthClipPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetReconstructPreviousDepthPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetLockPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetAccumulatePass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetRCASPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetComputeLuminancePyramidPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);
extern IFFXRHIBackendSubPass* GetAutogenReactiveMaskPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64);

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFXFSRPassParameters, "cbFSR3Upscaler");
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFXComputeLuminanceParameters, "cbSPD");
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFXRCASParameters, "cbRCAS");
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFXGenerateReactiveParameters, "cbGenerateReactive");

IFFXRHIBackendSubPass* GetFSRPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64)
{
	IFFXRHIBackendSubPass* SubPass = nullptr;
	switch (pass)
	{
		case FFX_FSR3UPSCALER_PASS_DEPTH_CLIP:
			SubPass = GetDepthClipPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_FSR3UPSCALER_PASS_RECONSTRUCT_PREVIOUS_DEPTH:
			SubPass = GetReconstructPreviousDepthPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_FSR3UPSCALER_PASS_LOCK:
			SubPass = GetLockPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_FSR3UPSCALER_PASS_ACCUMULATE:
			SubPass = GetAccumulatePass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_FSR3UPSCALER_PASS_ACCUMULATE_SHARPEN:
			SubPass = GetAccumulatePass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_FSR3UPSCALER_PASS_RCAS:
			SubPass = GetRCASPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_FSR3UPSCALER_PASS_COMPUTE_LUMINANCE_PYRAMID:
			SubPass = GetComputeLuminancePyramidPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		case FFX_FSR3UPSCALER_PASS_GENERATE_REACTIVE:
			SubPass = GetAutogenReactiveMaskPass(pass, permutationOptions, desc, outPipeline, bSupportHalf, bPreferWave64);
			break;
		default:
			break;
	}
	return SubPass;
}

FFXRHIBackendRegisterEffect<FFX_EFFECT_FSR3UPSCALER, GetFSRPass> FFXRHIBackendRegisterEffect<FFX_EFFECT_FSR3UPSCALER, GetFSRPass>::sSelf;

bool FFXFSRGlobalShader::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return FFXGlobalShader::ShouldCompilePermutation(Parameters);
}

void FFXFSRGlobalShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FPermutationDomain PermutationVector(Parameters.PermutationId);
	bool bUseHalf = PermutationVector.Get<FFX_UseHalf>();
	bool bPreferWave64 = PermutationVector.Get<FFX_PreferWave64>();
	if ((bUseHalf || bPreferWave64) && Parameters.Platform == SP_PCD3D_SM5)
	{
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceDXC);
	}
	FFXGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment, bPreferWave64);

	OutEnvironment.SetDefine(TEXT("FFX_FSR3UPSCALER_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF"), 0);
	OutEnvironment.SetDefine(TEXT("FFX_FSR3UPSCALER_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF"), 0);
	OutEnvironment.SetDefine(TEXT("FFX_FSR3UPSCALER_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF"), 1);
	OutEnvironment.SetDefine(TEXT("FFX_FSR3UPSCALER_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF"), 0);
	OutEnvironment.SetDefine(TEXT("FFX_FSR3UPSCALER_OPTION_UPSAMPLE_USE_LANCZOS_TYPE"), 2);
}

void FFXFSRGlobalShader::BindParameters(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job, FParameters* Parameters)
{
	for (uint32 i = 0; i < job->computeJobDescriptor.pipeline.constCount; i++)
	{
		switch (job->computeJobDescriptor.pipeline.constantBufferBindings[i].resourceIdentifier)
		{
			case FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_FSR3UPSCALER:
			{
				FFXFSRPassParameters Buffer;
				FMemory::Memcpy(&Buffer, job->computeJobDescriptor.cbs[i].data, sizeof(FFXFSRPassParameters));
				Parameters->cbFSR3Upscaler = TUniformBufferRef<FFXFSRPassParameters>::CreateUniformBufferImmediate(Buffer, UniformBuffer_SingleDraw);
				break;
			}
			case FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_RCAS:
			{
				FFXRCASParameters Buffer;
				FMemory::Memcpy(&Buffer, job->computeJobDescriptor.cbs[i].data, sizeof(FFXRCASParameters));
				Parameters->cbRCAS = TUniformBufferRef<FFXRCASParameters>::CreateUniformBufferImmediate(Buffer, UniformBuffer_SingleDraw);
				break;
			}
			case FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_SPD:
			{
				FFXComputeLuminanceParameters Buffer;
				FMemory::Memcpy(&Buffer, job->computeJobDescriptor.cbs[i].data, sizeof(FFXComputeLuminanceParameters));
				Parameters->cbSPD = TUniformBufferRef<FFXComputeLuminanceParameters>::CreateUniformBufferImmediate(Buffer, UniformBuffer_SingleDraw);
				break;
			}
			case FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_GENREACTIVE:
			{
				FFXGenerateReactiveParameters Buffer;
				FMemory::Memcpy(&Buffer, job->computeJobDescriptor.cbs[i].data, sizeof(FFXGenerateReactiveParameters));
				Parameters->cbGenerateReactive = TUniformBufferRef<FFXGenerateReactiveParameters>::CreateUniformBufferImmediate(Buffer, UniformBuffer_SingleDraw);
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
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_COLOR:
				Parameters->r_input_color_jittered = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_OPAQUE_ONLY:
				Parameters->r_input_opaque_only = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_MOTION_VECTORS:
				Parameters->r_input_motion_vectors = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_DEPTH:
				Parameters->r_input_depth = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_EXPOSURE:
				Parameters->r_input_exposure = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_AUTO_EXPOSURE:
				Parameters->r_auto_exposure = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_REACTIVE_MASK:
				Parameters->r_reactive_mask = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_TRANSPARENCY_AND_COMPOSITION_MASK:
				Parameters->r_transparency_and_composition_mask = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH:
				Parameters->r_reconstructed_previous_nearest_depth = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS:
				Parameters->r_dilated_motion_vectors = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_PREVIOUS_DILATED_MOTION_VECTORS:
				Parameters->r_previous_dilated_motion_vectors = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_DEPTH:
				Parameters->r_dilated_depth = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR:
				Parameters->r_internal_upscaled_color = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LOCK_STATUS:
				Parameters->r_lock_status = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_PREPARED_INPUT_COLOR:
				Parameters->r_prepared_input_color = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY:
				Parameters->r_luma_history = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RCAS_INPUT:
				Parameters->r_rcas_input = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LANCZOS_LUT:
				Parameters->r_lanczos_lut = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SCENE_LUMINANCE:
				Parameters->r_imgMips = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_SHADING_CHANGE:
				Parameters->r_img_mip_shading_change = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_5:
				Parameters->r_img_mip_5 = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTITIER_UPSAMPLE_MAXIMUM_BIAS_LUT:
				Parameters->r_upsample_maximum_bias_lut = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS:
				Parameters->r_dilated_reactive_masks = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NEW_LOCKS:
				Parameters->r_new_locks = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LOCK_INPUT_LUMA:
				Parameters->r_lock_input_luma = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_PREV_PRE_ALPHA_COLOR:
				Parameters->r_input_prev_color_pre_alpha = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
				break;
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_PREV_POST_ALPHA_COLOR:
				Parameters->r_input_prev_color_post_alpha = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].internalIndex);
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
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH:
			{
				Parameters->rw_reconstructed_previous_nearest_depth = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS:
			{
				Parameters->rw_dilated_motion_vectors = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_DEPTH:
			{
				Parameters->rw_dilated_depth = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INTERNAL_UPSCALED_COLOR:
			{
				Parameters->rw_internal_upscaled_color = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LOCK_STATUS:
			{
				Parameters->rw_lock_status = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_PREPARED_INPUT_COLOR:
			{
				Parameters->rw_prepared_input_color = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LUMA_HISTORY:
			{
				Parameters->rw_luma_history = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT:
			{
				Parameters->rw_upscaled_output = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_SHADING_CHANGE:
			{
				Parameters->rw_img_mip_shading_change = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]), ERDGUnorderedAccessViewFlags::None);
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SCENE_LUMINANCE_MIPMAP_5:
			{
				Parameters->rw_img_mip_5 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]), ERDGUnorderedAccessViewFlags::None);
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_DILATED_REACTIVE_MASKS:
			{
				Parameters->rw_dilated_reactive_masks = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_AUTO_EXPOSURE:
			{
				Parameters->rw_auto_exposure = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_SPD_ATOMIC_COUNT:
			{
				Parameters->rw_spd_global_atomic = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_NEW_LOCKS:
			{
				Parameters->rw_new_locks = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_LOCK_INPUT_LUMA:
			{
				Parameters->rw_lock_input_luma = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_AUTOREACTIVE:
			{
				Parameters->rw_output_autoreactive = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_AUTOCOMPOSITION_DEPRECATED:
			{
				Parameters->rw_output_autocomposition = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_PREV_PRE_ALPHA_COLOR:
			{
				Parameters->rw_output_prev_color_pre_alpha = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_PREV_POST_ALPHA_COLOR:
			{
				Parameters->rw_output_prev_color_post_alpha = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].internalIndex), job->computeJobDescriptor.uavTextureMips[i]));
				break;
			}
			default:
			{
				break;
			}
		}
	}

	Parameters->s_LinearClamp = TStaticSamplerState<SF_Bilinear>::GetRHI();
	Parameters->s_PointClamp = TStaticSamplerState<SF_Point>::GetRHI();
}
