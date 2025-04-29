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

#include "FFXRHIBackendOpticalFlowShaders.h"
#include "FFXRHIBackendSubPass.h"
#include "FFXRHIBackend.h"
#include "ShaderParameterStruct.h"

#include "FFXOpticalFlowApi.h"

class FFXRHIOpticalFlowComputeLumaPyramidCS : public FFXOpticalFlowGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFXRHIOpticalFlowComputeLumaPyramidCS);
	SHADER_USE_PARAMETER_STRUCT(FFXRHIOpticalFlowComputeLumaPyramidCS, FFXOpticalFlowGlobalShader);

	using FParameters = FFXOpticalFlowGlobalShader::FParameters;

	static void BindParameters(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job, FParameters* Parameters)
	{
		FFXOpticalFlowGlobalShader::BindParameters(GraphBuilder, Context, job, Parameters);
	}

	using FPermutationDomain = FFXOpticalFlowGlobalShader::FPermutationDomain;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FFXOpticalFlowGlobalShader::ShouldCompilePermutation(Parameters);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FFXOpticalFlowGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("FFX_OPTICALFLOW_TEST_PARTICLE_SPEED"), 0);
	}

	static const wchar_t** GetBoundSRVNames()
	{
		return nullptr;
	}

	static const wchar_t** GetBoundUAVNames()
	{
		static const wchar_t* SRVs[] = {
			L"rw_optical_flow_input",
			L"rw_optical_flow_input_level_1",
			L"rw_optical_flow_input_level_2",
			L"rw_optical_flow_input_level_3",
			L"rw_optical_flow_input_level_4",
			L"rw_optical_flow_input_level_5",
			L"rw_optical_flow_input_level_6",
		};
		return SRVs;
	}

	static const wchar_t** GetBoundCBNames()
	{
		static const wchar_t* SRVs[] = {
			L"cbOF",
			L"cbOF_SPD",
		};
		return SRVs;
	}

	static uint32* GetBoundSRVs()
	{
		return nullptr;
	}

	static uint32 GetNumBoundSRVs()
	{
		return 0;
	}

	static uint32* GetBoundUAVs()
	{
		static uint32 UAVs[] = {                      
			FFX_OF_RESOURCE_IDENTIFIER_OPTICAL_FLOW_INPUT_1,
			FFX_OF_RESOURCE_IDENTIFIER_OPTICAL_FLOW_INPUT_1_LEVEL_1,
			FFX_OF_RESOURCE_IDENTIFIER_OPTICAL_FLOW_INPUT_1_LEVEL_2,
			FFX_OF_RESOURCE_IDENTIFIER_OPTICAL_FLOW_INPUT_1_LEVEL_3,
			FFX_OF_RESOURCE_IDENTIFIER_OPTICAL_FLOW_INPUT_1_LEVEL_4,
			FFX_OF_RESOURCE_IDENTIFIER_OPTICAL_FLOW_INPUT_1_LEVEL_5,
			FFX_OF_RESOURCE_IDENTIFIER_OPTICAL_FLOW_INPUT_1_LEVEL_6,
		};
		return UAVs;
	}

	static uint32 GetNumBoundUAVs()
	{
		return 7;
	}

	static uint32* GetBoundCBs()
	{
		static uint32 UAVs[] = {                      
			FFX_OPTICALFLOW_CONSTANTBUFFER_IDENTIFIER,
			FFX_OPTICALFLOW_CONSTANTBUFFER_IDENTIFIER_SPD,
		};
		return UAVs;
	}

	static uint32 GetNumConstants()
	{
		return 2;
	}

	static uint32 GetConstantSizeInDWords(uint32 Index)
	{
		static uint32 Sizes[] = { sizeof(FFXOpticalFlowParameters) / sizeof(uint32) };
		return Sizes[Index];
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXRHIOpticalFlowComputeLumaPyramidCS, "/Plugin/FFX/Private/ffx_opticalflow_compute_luminance_pyramid_pass.usf", "CS", SF_Compute);

IFFXRHIBackendSubPass* GetOpticalFlowComputeLumaPyramidPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64)
{
	auto* Pipeline = new TFFXRHIBackendSubPass<FFXRHIOpticalFlowComputeLumaPyramidCS>(TEXT("FidelityFX-FSR3/OpticalFlow ComputeLumaPyramid (CS)"), desc, outPipeline, bSupportHalf);
	Pipeline->Permutation.template Set<FFX_UseHalf>(bSupportHalf);
	Pipeline->Permutation.template Set<FFX_PreferWave64>(bPreferWave64);
	return Pipeline;
}
