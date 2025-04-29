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

#include "FFXRHIBackendFIShaders.h"
#include "FFXRHIBackendSubPass.h"
#include "FFXRHIBackend.h"
#include "FFXRHIBackendFIShaders.h"
#include "ShaderParameterStruct.h"

#include "FFXFrameInterpolationApi.h"

class FFXRHIFIFrameInterpolationCS : public FFXFIGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFXRHIFIFrameInterpolationCS);
	SHADER_USE_PARAMETER_STRUCT(FFXRHIFIFrameInterpolationCS, FFXFIGlobalShader);

	using FParameters = FFXFIGlobalShader::FParameters;
	using FPermutationDomain = FFXFIGlobalShader::FPermutationDomain;

	static void BindParameters(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job, FParameters* Parameters)
	{
		FFXFIGlobalShader::BindParameters(GraphBuilder, Context, job, Parameters);
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FFXFIGlobalShader::ShouldCompilePermutation(Parameters);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FFXFIGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	static uint32 GetNumBoundSRVs()
	{
		return 9;
	}

	static uint32 GetNumBoundUAVs()
	{
		return 1;
	}

	static uint32 GetNumConstants()
	{
		return 1;
	}

	static uint32* GetBoundSRVs()
	{
		static uint32 SRVs[] = {
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_X,
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_GAME_MOTION_VECTOR_FIELD_Y,
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_X,
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OPTICAL_FLOW_MOTION_VECTOR_FIELD_Y,
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_PREVIOUS_INTERPOLATION_SOURCE,
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_CURRENT_INTERPOLATION_SOURCE,
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_DISOCCLUSION_MASK,                     
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID,
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNTERS,
		};
		return SRVs;
	}

	static const wchar_t** GetBoundSRVNames()
	{
		static const wchar_t* SRVs[] = {
			L"r_game_motion_vector_field_x",
			L"r_game_motion_vector_field_y",
			L"r_optical_flow_motion_vector_field_x",
			L"r_optical_flow_motion_vector_field_y",
			L"r_previous_interpolation_source",
			L"r_current_interpolation_source",
			L"r_disocclusion_mask",
			L"r_inpainting_pyramid",
			L"r_counters",
		};
		return SRVs;
	}

	static const wchar_t** GetBoundUAVNames()
	{
		static const wchar_t* SRVs[] = {
			L"rw_output",
		};
		return SRVs;
	}

	static const wchar_t** GetBoundCBNames()
	{
		static const wchar_t* SRVs[] = {
			L"cbFI",
		};
		return SRVs;
	}

	static uint32* GetBoundUAVs()
	{
		static uint32 UAVs[] = { 
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT,                   
		};
		return UAVs;
	}

	static uint32* GetBoundCBs()
	{
		static uint32 CBs[] = { 
			FFX_FRAMEINTERPOLATION_CONSTANTBUFFER_IDENTIFIER,
		};
		return CBs;
	}

	static uint32 GetConstantSizeInDWords(uint32 Index)
	{
		static uint32 Sizes[] = { sizeof(FFXFrameInterpolationParameters) / sizeof(uint32) };
		return Sizes[Index];
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXRHIFIFrameInterpolationCS, "/Plugin/FFX/Private/ffx_frameinterpolation_pass.usf", "CS", SF_Compute);

IFFXRHIBackendSubPass* GetFIFrameInterpolationPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64)
{
	auto* Pipeline = new TFFXRHIBackendSubPass<FFXRHIFIFrameInterpolationCS>(TEXT("FidelityFX-FSR3/FI Frame Interpolation (CS)"), desc, outPipeline, bSupportHalf);
	Pipeline->Permutation.template Set<FFXFI_DepthInverted>(desc->contextFlags & FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED);
	Pipeline->Permutation.template Set<FFX_UseHalf>(bSupportHalf);
	Pipeline->Permutation.template Set<FFX_PreferWave64>(bPreferWave64);
	return Pipeline;
}
