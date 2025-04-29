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

#include "FFXRHIBackendShaders.h"

#include "ShaderCompilerCore.h"
#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "DataDrivenShaderPlatformInfo.h"
#endif

bool FFXGlobalShader::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
#if UE_VERSION_AT_LEAST(5, 1, 0)
	bool const bWaveOps = FDataDrivenShaderPlatformInfo::GetSupportsWaveOperations(Parameters.Platform) == ERHIFeatureSupport::RuntimeGuaranteed;
#elif UE_VERSION_AT_LEAST(5, 0, 0)
	bool const bWaveOps = RHISupportsWaveOperations(Parameters.Platform);
#else
	bool const bWaveOps = FDataDrivenShaderPlatformInfo::GetSupportsWaveOperations(Parameters.Platform) || (Parameters.Platform == SP_PCD3D_SM5);
#endif
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) && bWaveOps && IsPCPlatform(Parameters.Platform);
}

void FFXGlobalShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment, bool bPreferWave64)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	OutEnvironment.CompilerFlags.Add(CFLAG_WaveOperations);
	if (Parameters.Platform == SP_PCD3D_SM5)
	{
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceDXC);
	}

	OutEnvironment.SetDefine(TEXT("FFX_GPU"), 1);
	OutEnvironment.SetDefine(TEXT("FFX_HLSL"), 1);

#if UE_VERSION_AT_LEAST(5, 0, 0)	
	OutEnvironment.SetDefine(TEXT("UNREAL_VERSION"), 5);
#else
	OutEnvironment.SetDefine(TEXT("UNREAL_VERSION"), 4);
#endif

#if UE_VERSION_AT_LEAST(5, 1, 0)
	if (bPreferWave64 
#if UE_VERSION_AT_LEAST(5, 4, 0)
		&& FDataDrivenShaderPlatformInfo::GetSupportsWave64(Parameters.Platform))
#else
		&& IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6))
#endif
	{
		OutEnvironment.SetDefine(TEXT("FFX_PREFER_WAVE64"), TEXT("[WaveSize(64)]"));
	}
#endif
}
