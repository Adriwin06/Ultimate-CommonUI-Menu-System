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
#include "ShaderParameterStruct.h"

#if UE_VERSION_OLDER_THAN(5, 0, 0)
using FVector3f = float[3];
typedef FVector2D FVector2f;
using FVector4f = float[4];
typedef FIntPoint FUintVector2;
#endif

struct FFXBackendState;
class FRDGBuilder;
typedef struct FfxGpuJobDescription FfxGpuJobDescription;

//-------------------------------------------------------------------------------------
// Permutation variables for FSR3 shaders.
//-------------------------------------------------------------------------------------
class FFX_IsHDR : SHADER_PERMUTATION_BOOL("FFX_FSR3UPSCALER_OPTION_HDR_COLOR_INPUT");
class FFX_MVLowRes : SHADER_PERMUTATION_BOOL("FFX_FSR3UPSCALER_OPTION_LOW_RESOLUTION_MOTION_VECTORS");
class FFX_MVJittered : SHADER_PERMUTATION_BOOL("FFX_FSR3UPSCALER_OPTION_JITTERED_MOTION_VECTORS");
class FFX_DepthInverted : SHADER_PERMUTATION_BOOL("FFX_FSR3UPSCALER_OPTION_INVERTED_DEPTH");
class FFX_DoSharpening : SHADER_PERMUTATION_BOOL("FFX_FSR3UPSCALER_OPTION_APPLY_SHARPENING");
class FFX_UseLanczosType : SHADER_PERMUTATION_BOOL("FFX_FSR3UPSCALER_OPTION_REPROJECT_USE_LANCZOS_TYPE");

//-------------------------------------------------------------------------------------
// Common uniform buffer structure for the FSR pass parameters.
//-------------------------------------------------------------------------------------
BEGIN_UNIFORM_BUFFER_STRUCT(FFXFSRPassParameters, )
	SHADER_PARAMETER(FIntPoint, iRenderSize)
	SHADER_PARAMETER(FIntPoint, iPrevRenderSize)
	SHADER_PARAMETER(FIntPoint, iMaxRenderSize)
	SHADER_PARAMETER(FIntPoint, iUpscaleSize)
	SHADER_PARAMETER(FIntPoint, iMaxUpscaleSize)
	SHADER_PARAMETER(FIntPoint, pad)

	SHADER_PARAMETER(FVector4f, fDeviceToViewDepth)

	SHADER_PARAMETER(FVector2f, fJitter)
	SHADER_PARAMETER(FVector2f, fPrevJitter)

	SHADER_PARAMETER(FVector2f, fMotionVectorScale)
	SHADER_PARAMETER(FVector2f, fDownscaleFactor)

	SHADER_PARAMETER(FVector2f, fMotionVectorJitterCancellation)
	SHADER_PARAMETER(float, fTanHalfFOV)
	SHADER_PARAMETER(float, fJitterPhaseCount)

	SHADER_PARAMETER(float, fDeltaTime)
	SHADER_PARAMETER(float, fDeltaPreExposure)
	SHADER_PARAMETER(float, fViewSpaceToMetersFactor)
	SHADER_PARAMETER(float, fFrameIndex)

	SHADER_PARAMETER(float, fVelocityFactor)
END_UNIFORM_BUFFER_STRUCT()

//-------------------------------------------------------------------------------------
// Parameters for the compute luminance pyramid pass.
//-------------------------------------------------------------------------------------
BEGIN_UNIFORM_BUFFER_STRUCT(FFXLumaPyramidParameters, )
	SHADER_PARAMETER(uint32, mips)
	SHADER_PARAMETER(uint32, numWorkGroups)
	SHADER_PARAMETER(FUintVector2, workGroupOffset)
	SHADER_PARAMETER(FUintVector2, renderSize)
END_UNIFORM_BUFFER_STRUCT()

//-------------------------------------------------------------------------------------
// Paramters for the sharpening pass.
//-------------------------------------------------------------------------------------
BEGIN_UNIFORM_BUFFER_STRUCT(FFXRCASParameters, )
	SHADER_PARAMETER(FUintVector4, rcasConfig)
END_UNIFORM_BUFFER_STRUCT()

//-------------------------------------------------------------------------------------
// Paramters for the TCR Autogenerate pass.
//-------------------------------------------------------------------------------------
BEGIN_UNIFORM_BUFFER_STRUCT(FFXGenerateReactiveParameters, )
	SHADER_PARAMETER(float, scale)
	SHADER_PARAMETER(float, threshold)
	SHADER_PARAMETER(float, binaryValue)
	SHADER_PARAMETER(uint32, flags)
END_UNIFORM_BUFFER_STRUCT()

//-------------------------------------------------------------------------------------
// Common base class for the FFX shaders that handles the permutation support.
//-------------------------------------------------------------------------------------
class FFXFSRGlobalShader : public FFXGlobalShader
{
public:
	typedef TShaderPermutationDomain<FFX_IsHDR, FFX_MVLowRes, FFX_MVJittered, FFX_DepthInverted, FFX_DoSharpening, FFX_UseLanczosType, FFX_UseHalf, FFX_PreferWave64> FPermutationDomain;
	FFXFSRGlobalShader() = default;
	FFXFSRGlobalShader(const ShaderMetaType::CompiledShaderInitializerType Init) : FFXGlobalShader(Init) {}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FFXFSRPassParameters, cbFSR3Upscaler)
		SHADER_PARAMETER_STRUCT_REF(FFXRCASParameters, cbRCAS)
		SHADER_PARAMETER_STRUCT_REF(FFXLumaPyramidParameters, cbSPD)
		SHADER_PARAMETER_STRUCT_REF(FFXGenerateReactiveParameters, cbGenerateReactive)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_input_color_jittered)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_input_opaque_only)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_input_motion_vectors)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_input_depth)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_input_exposure)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_frame_info)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_reactive_mask)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_transparency_and_composition_mask)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_reconstructed_previous_nearest_depth)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_dilated_motion_vectors)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_dilated_depth)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_internal_upscaled_color)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_accumulation)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_luma_history)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_rcas_input)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_lanczos_lut)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_spd_mips)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_dilated_reactive_masks)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_new_locks)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_farthest_depth)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_farthest_depth_mip1)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_shading_change)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_current_luma)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_previous_luma)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_luma_instability)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_reconstructed_previous_nearest_depth)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_dilated_motion_vectors)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_dilated_depth)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_internal_upscaled_color)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_accumulation)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_luma_history)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_upscaled_output)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_dilated_reactive_masks)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_frame_info)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_spd_global_atomic)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_new_locks)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_output_autoreactive)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_shading_change)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_farthest_depth)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_farthest_depth_mip1)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_current_luma)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_luma_instability)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_spd_mip0)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_spd_mip1)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_spd_mip2)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_spd_mip3)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_spd_mip4)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_spd_mip5)
		SHADER_PARAMETER_SAMPLER(SamplerState, s_LinearClamp)
		SHADER_PARAMETER_SAMPLER(SamplerState, s_PointClamp)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
	static void BindParameters(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job, FParameters* Parameters);
};
