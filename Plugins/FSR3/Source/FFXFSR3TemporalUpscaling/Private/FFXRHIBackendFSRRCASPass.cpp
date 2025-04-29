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

#include "FFXRHIBackendFSRShaders.h"
#include "FFXRHIBackendSubPass.h"
#include "FFXRHIBackend.h"
#include "ShaderParameterStruct.h"

#include "FFXFSR3.h"
#include "ffx_fsr3upscaler_private.h"

class FFXRHIRCASCS : public FFXFSRGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFXRHIRCASCS);
	SHADER_USE_PARAMETER_STRUCT(FFXRHIRCASCS, FFXFSRGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FFXFSRPassParameters, cbFSR3Upscaler)
		SHADER_PARAMETER_STRUCT_REF(FFXRCASParameters, cbRCAS)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_input_exposure)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, r_rcas_input)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, rw_upscaled_output)
	END_SHADER_PARAMETER_STRUCT()

	using FPermutationDomain = FFXFSRGlobalShader::FPermutationDomain;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FFXFSRGlobalShader::ShouldCompilePermutation(Parameters);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FFXFSRGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("SAMPLE_EASU"), 0);
		OutEnvironment.SetDefine(TEXT("SAMPLE_RCAS"), 1);
		OutEnvironment.SetDefine(TEXT("WIDTH"), 64);
		OutEnvironment.SetDefine(TEXT("HEIGHT"), 1);
		OutEnvironment.SetDefine(TEXT("DEPTH"), 1);
	}

	

	static const wchar_t** GetBoundSRVNames()
	{
		static const wchar_t* SRVs[] = {
			L"r_input_exposure",
			L"r_rcas_input",
		};
		return SRVs;
	}

	static const wchar_t** GetBoundUAVNames()
	{
		static const wchar_t* SRVs[] = {
			L"rw_upscaled_output",
		};
		return SRVs;
	}

	static const wchar_t** GetBoundCBNames()
	{
		static const wchar_t* SRVs[] = {
			L"cbFSR3Upscaler",
			L"cbRCAS",
		};
		return SRVs;
	}

	static uint32* GetBoundSRVs()
	{
		static uint32 SRVs[] = {
			FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_EXPOSURE,                          
			FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RCAS_INPUT,                              
		};
		return SRVs;
	}

	static uint32 GetNumBoundSRVs()
	{
		return 2;
	}

	static uint32* GetBoundUAVs()
	{
		static uint32 UAVs[] = { 
			FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT,                      
		};
		return UAVs;
	}

	static uint32 GetNumBoundUAVs()
	{
		return 1;
	}

	static uint32* GetBoundCBs()
	{
		static uint32 CBs[] = { 
			FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_FSR3UPSCALER,           
			FFX_FSR3UPSCALER_CONSTANTBUFFER_IDENTIFIER_RCAS,           
		};
		return CBs;
	}

	static uint32 GetNumConstants()
	{
		return 2;
	}

	static uint32 GetConstantSizeInDWords(uint32 Index)
	{
		static uint32 Sizes[] = { sizeof(FFXFSRPassParameters) / sizeof(uint32), sizeof(FFXRCASParameters) / sizeof(uint32) };
		return Sizes[Index];
	}

	static void BindParameters(FRDGBuilder& GraphBuilder, FFXBackendState* Context, const FfxGpuJobDescription* job, FParameters* Parameters)
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
				case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_INPUT_EXPOSURE:
					Parameters->r_input_exposure = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].resource.internalIndex);
					break;
				case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_RCAS_INPUT:
					Parameters->r_rcas_input = Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.srvTextures[i].resource.internalIndex);
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
				case FFX_FSR3UPSCALER_RESOURCE_IDENTIFIER_UPSCALED_OUTPUT:
				{
					Parameters->rw_upscaled_output = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Context->GetRDGTexture(GraphBuilder, job->computeJobDescriptor.uavTextures[i].resource.internalIndex), job->computeJobDescriptor.uavTextures[i].mip));
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}
};
IMPLEMENT_GLOBAL_SHADER(FFXRHIRCASCS, "/Plugin/FFX/Private/ffx_fsr3upscaler_rcas_pass.usf", "CS", SF_Compute);

IFFXRHIBackendSubPass* GetRCASPass(FfxPass pass, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxPipelineState* outPipeline, bool bSupportHalf, bool bPreferWave64)
{
	auto* Pipeline = new TFFXRHIBackendSubPass<FFXRHIRCASCS>(TEXT("FidelityFX-FSR3/RCAS (CS)"), desc, outPipeline, bSupportHalf);
	Pipeline->Permutation.template Set<FFX_IsHDR>(desc->contextFlags & FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE);
	Pipeline->Permutation.template Set<FFX_MVLowRes>(!(desc->contextFlags & FFX_FSR3UPSCALER_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS));
	Pipeline->Permutation.template Set<FFX_MVJittered>(desc->contextFlags & FFX_FSR3UPSCALER_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION);
	Pipeline->Permutation.template Set<FFX_DepthInverted>(desc->contextFlags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED);
	Pipeline->Permutation.template Set<FFX_DoSharpening>(pass == FFX_FSR3UPSCALER_PASS_ACCUMULATE_SHARPEN);
	Pipeline->Permutation.template Set<FFX_UseLanczosType>(permutationOptions & FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
	Pipeline->Permutation.template Set<FFX_UseHalf>(false);
	Pipeline->Permutation.template Set<FFX_PreferWave64>(bPreferWave64);
	return Pipeline;
}
