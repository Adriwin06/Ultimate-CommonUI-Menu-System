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

struct FFXBackendState;
class FRDGBuilder;
typedef struct FfxGpuJobDescription FfxGpuJobDescription;

#if UE_VERSION_OLDER_THAN(5, 0, 0)
using FVector3f = float[3];
typedef FVector2D FVector2f;
using FVector4f = float[4];
typedef FIntPoint FUintVector2;
#endif

//-------------------------------------------------------------------------------------
// Paramters for the optical flow pass.
//-------------------------------------------------------------------------------------
BEGIN_UNIFORM_BUFFER_STRUCT(FFXOpticalFlowParameters, )
	SHADER_PARAMETER(FIntPoint, iInputLumaResolution)
	SHADER_PARAMETER(uint32, uOpticalFlowPyramidLevel)
	SHADER_PARAMETER(uint32, uOpticalFlowPyramidLevelCount)
	SHADER_PARAMETER(uint32, iFrameIndex)
	SHADER_PARAMETER(uint32, backbufferTransferFunction)
	SHADER_PARAMETER(FVector2f, minMaxLuminance)
END_UNIFORM_BUFFER_STRUCT()

//-------------------------------------------------------------------------------------
// Paramters for the optical flow pass.
//-------------------------------------------------------------------------------------
BEGIN_UNIFORM_BUFFER_STRUCT(FFXOpticalFlowSPDParameters, )
	SHADER_PARAMETER(uint32, mips)
	SHADER_PARAMETER(uint32, numworkGroups)
	SHADER_PARAMETER(FUintVector2, workGroupOffset)
END_UNIFORM_BUFFER_STRUCT()

//-------------------------------------------------------------------------------------
// Common base class for the FFX shaders that handles the permutation support.
//-------------------------------------------------------------------------------------
class FFXOpticalFlowGlobalShader : public FFXGlobalShader
{
public:
	typedef TShaderPermutationDomain<FFX_UseHalf, FFX_PreferWave64> FPermutationDomain;
	FFXOpticalFlowGlobalShader() = default;
	FFXOpticalFlowGlobalShader(const ShaderMetaType::CompiledShaderInitializerType Init) : FFXGlobalShader(Init) {}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FFXOpticalFlowParameters, cbOF)
		SHADER_PARAMETER_STRUCT_REF(FFXOpticalFlowSPDParameters, cbOF_SPD)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_input_color)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_optical_flow_input)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_optical_flow_previous_input)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_optical_flow)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_optical_flow_previous)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_input)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_input_level_1)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_input_level_2)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_input_level_3)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_input_level_4)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_input_level_5)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_input_level_6)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_next_level)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_scd_histogram)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_scd_previous_histogram)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_scd_temp)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_optical_flow_scd_output)
		SHADER_PARAMETER_SAMPLER(SamplerState, s_LinearClamp)
		SHADER_PARAMETER_SAMPLER(SamplerState, s_PointClamp)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
	static void BindParameters(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job, FParameters* Parameters);
};
