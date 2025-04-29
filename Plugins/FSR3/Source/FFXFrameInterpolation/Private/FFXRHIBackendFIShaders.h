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

#include "FFXRHIBackendShaders.h"

struct FFXBackendState;
class FRDGBuilder;
typedef struct FfxGpuJobDescription FfxGpuJobDescription;

#if UE_VERSION_OLDER_THAN(5, 0, 0)
using FVector3f = float[3];
using FVector4f = float[4];
typedef FVector2D FVector2f;
typedef FIntPoint FUintVector2;
#endif
class FFXFI_DepthInverted : SHADER_PERMUTATION_BOOL("FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH");

//-------------------------------------------------------------------------------------
// Paramters for the frame interpolation pass.
//-------------------------------------------------------------------------------------
BEGIN_UNIFORM_BUFFER_STRUCT(FFXFrameInterpolationParameters, )
	SHADER_PARAMETER(FIntPoint, renderSize)
	SHADER_PARAMETER(FIntPoint, displaySize)
	SHADER_PARAMETER(FVector2f, displaySizeRcp)
	SHADER_PARAMETER(float, cameraNear)
	SHADER_PARAMETER(float, cameraFar)
	SHADER_PARAMETER(FIntPoint, upscalerTargetSize)
	SHADER_PARAMETER(int32, Mode)
	SHADER_PARAMETER(int32, reset)
	SHADER_PARAMETER(FVector4f, fDeviceToViewDepth)
	SHADER_PARAMETER(float, deltaTime)
	SHADER_PARAMETER(float, HUDLessAttachedFactor)
	SHADER_PARAMETER(FIntPoint, distortionFieldSize)
	SHADER_PARAMETER(FVector2f, opticalFlowScale)
	SHADER_PARAMETER(int32, opticalFlowBlockSize)
	SHADER_PARAMETER(uint32, dispatchFlags)
    SHADER_PARAMETER(FIntPoint, maxRenderSize)
    SHADER_PARAMETER(int32, opticalFlowHalfResMode)
    SHADER_PARAMETER(int32, NumInstances)
	SHADER_PARAMETER(FIntPoint, interpolationRectBase)
	SHADER_PARAMETER(FIntPoint, interpolationRectSize)
	SHADER_PARAMETER(FVector3f, debugBarColor)
	SHADER_PARAMETER(uint32, backbufferTransferFunction)
	SHADER_PARAMETER(FVector2f, minMaxLuminance)
	SHADER_PARAMETER(float, fTanHalfFOV)
	SHADER_PARAMETER(float, _pad1)
	SHADER_PARAMETER(FVector2f, fJitter)
	SHADER_PARAMETER(FVector2f, fMotionVectorScale)
END_UNIFORM_BUFFER_STRUCT()

//-------------------------------------------------------------------------------------
// Paramters for the Inpainting pyramid pass.
//-------------------------------------------------------------------------------------
BEGIN_UNIFORM_BUFFER_STRUCT(FFXInpaintingPyramidParameters, )
	SHADER_PARAMETER(uint32, mips)
	SHADER_PARAMETER(uint32, numWorkGroups)
	SHADER_PARAMETER(FUintVector2, workGroupOffset)
END_UNIFORM_BUFFER_STRUCT()

//-------------------------------------------------------------------------------------
// Common base class for the FFX shaders that handles the permutation support.
//-------------------------------------------------------------------------------------
class FFXFIGlobalShader : public FFXGlobalShader
{
public:
	typedef TShaderPermutationDomain<FFXFI_DepthInverted, FFX_UseHalf, FFX_PreferWave64> FPermutationDomain;
	FFXFIGlobalShader() = default;
	FFXFIGlobalShader(const ShaderMetaType::CompiledShaderInitializerType Init) : FFXGlobalShader(Init) {}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FFXFrameInterpolationParameters, cbFI)
		SHADER_PARAMETER_STRUCT_REF(FFXInpaintingPyramidParameters, cbInpaintingPyramid)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_input_depth)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_input_motion_vectors)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_dilated_depth)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_dilated_motion_vectors)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_reconstructed_depth_previous_frame)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_reconstructed_depth_interpolated_frame)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_previous_interpolation_source)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_current_interpolation_source)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_disocclusion_mask)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_game_motion_vector_field_x)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_game_motion_vector_field_y)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_optical_flow_motion_vector_field_x)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_optical_flow_motion_vector_field_y)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_optical_flow)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_optical_flow_confidence)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_optical_flow_global_motion)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_optical_flow_scd)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_output)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_inpainting_mask)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_inpainting_pyramid)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_present_backbuffer)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_input_distortion_field)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, r_counters)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_dilated_depth)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_dilated_motion_vectors)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_reconstructed_depth_previous_frame)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_reconstructed_depth_interpolated_frame)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_output)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_disocclusion_mask)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_game_motion_vector_field_x)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_game_motion_vector_field_y)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_motion_vector_field_x)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_motion_vector_field_y)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, rw_counters)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_mask)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid0)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid1)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid2)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid3)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid4)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid5)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid6)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid7)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid8)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid9)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid10)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid11)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_inpainting_pyramid12)
		SHADER_PARAMETER_SAMPLER(SamplerState, s_LinearClamp)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
	static void BindParameters(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job, FParameters* Parameters);
};
