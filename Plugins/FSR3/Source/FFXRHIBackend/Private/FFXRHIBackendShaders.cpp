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

#include "FFXRHIBackendShaders.h"

#include "ShaderCompilerCore.h"
#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "DataDrivenShaderPlatformInfo.h"
#endif

bool FFXGlobalShader::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) && IsPCPlatform(Parameters.Platform);
}

void FFXGlobalShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment, bool bPreferWave64)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

	OutEnvironment.SetDefine(TEXT("FFX_GPU"), 1);
	OutEnvironment.SetDefine(TEXT("FFX_HLSL"), 1);
	
	// Remove the unorm attribute when compiling with DX to avoid an fxc error - should be irrelevant for DXC.
	if (IsD3DPlatform(Parameters.Platform) && !IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6))
	{
		OutEnvironment.SetDefine(TEXT("unorm"), TEXT(" "));
	}

	if (bPreferWave64 && IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6))
	{
		OutEnvironment.SetDefine(TEXT("FFX_PREFER_WAVE64"), TEXT("[WaveSize(64)]"));
	}
}
