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

class FFXRHIFIInpaintingPyramidCS : public FFXFIGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFXRHIFIInpaintingPyramidCS);
	SHADER_USE_PARAMETER_STRUCT(FFXRHIFIInpaintingPyramidCS, FFXFIGlobalShader);

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
		return 1;
	}

	static uint32 GetNumBoundUAVs()
	{
		return 14;
	}

	static uint32 GetNumConstants()
	{
		return 2;
	}

	static uint32* GetBoundSRVs()
	{
		static uint32 SRVs[] = {
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_OUTPUT
		};
		return SRVs;
	}

	static const wchar_t** GetBoundSRVNames()
	{
		static const wchar_t* SRVs[] = {
			L"r_output"
		};
		return SRVs;
	}

	static const wchar_t** GetBoundUAVNames()
	{
		static const wchar_t* SRVs[] = {
			L"rw_counters",
			L"rw_inpainting_pyramid0",
			L"rw_inpainting_pyramid1",
			L"rw_inpainting_pyramid2",
			L"rw_inpainting_pyramid3",
			L"rw_inpainting_pyramid4",
			L"rw_inpainting_pyramid5",
			L"rw_inpainting_pyramid6",
			L"rw_inpainting_pyramid7",
			L"rw_inpainting_pyramid8",
			L"rw_inpainting_pyramid9",
			L"rw_inpainting_pyramid10",
			L"rw_inpainting_pyramid11",
			L"rw_inpainting_pyramid12"
		};
		return SRVs;
	}

	static const wchar_t** GetBoundCBNames()
	{
		static const wchar_t* SRVs[] = {
			L"cbFI",
			L"cbInpaintingPyramid",
		};
		return SRVs;
	}

	static uint32* GetBoundUAVs()
	{
		static uint32 UAVs[] = { 
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNTERS,
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_0,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_1,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_2,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_3,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_4,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_5,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_6,                      
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_7,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_8,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_9,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_10,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_11,           
			FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_INPAINTING_PYRAMID_MIPMAP_12
		};
		return UAVs;
	}

	static uint32* GetBoundCBs()
	{
		static uint32 CBs[] = { 
			FFX_FRAMEINTERPOLATION_CONSTANTBUFFER_IDENTIFIER,
			FFX_FRAMEINTERPOLATION_INPAINTING_PYRAMID_CONSTANTBUFFER_IDENTIFIER
		};
		return CBs;
	}

	static uint32 GetConstantSizeInDWords(uint32 Index)
	{
		static uint32 Sizes[] = { sizeof(FFXFrameInterpolationParameters) / sizeof(uint32), sizeof(FFXInpaintingPyramidParameters) / sizeof(uint32) };
		return Sizes[Index];
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXRHIFIInpaintingPyramidCS, "/Plugin/FFX/Private/ffx_frameinterpolation_compute_inpainting_pyramid_pass.usf", "CS", SF_Compute);

IFFXRHIBackendSubPass* GetFIInpaintingPyramidPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64)
{
	auto* Pipeline = new TFFXRHIBackendSubPass<FFXRHIFIInpaintingPyramidCS>(TEXT("FidelityFX-FSR3/FI Inpainting Pyramid (CS)"), desc, outPipeline, bSupportHalf);
	Pipeline->Permutation.template Set<FFXFI_DepthInverted>(desc->contextFlags & FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED);
	Pipeline->Permutation.template Set<FFX_UseHalf>(bSupportHalf);
	Pipeline->Permutation.template Set<FFX_PreferWave64>(bPreferWave64);
	return Pipeline;
}
