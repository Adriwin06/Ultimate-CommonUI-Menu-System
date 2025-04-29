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

#include "SceneViewExtension.h"
#include "FFXShared.h"

#if UE_VERSION_AT_LEAST(5, 1, 0)
typedef FRDGBuilder FRenderGraphType;
#else
typedef FRHICommandListImmediate FRenderGraphType;
#endif

class FFXFSR3TEMPORALUPSCALING_API FFXFSR3ViewExtension final : public FSceneViewExtensionBase
{
public:
	FFXFSR3ViewExtension(const FAutoRegister& AutoRegister);

	// ISceneViewExtension interface
	void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}

	void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
#if UE_VERSION_AT_LEAST(5, 0, 0)
	void PreRenderViewFamily_RenderThread(FRenderGraphType& GraphBuilder, FSceneViewFamily& InViewFamily) override;
	void PreRenderView_RenderThread(FRenderGraphType& GraphBuilder, FSceneView& InView) override;
	void PostRenderViewFamily_RenderThread(FRenderGraphType& GraphBuilder, FSceneViewFamily& InViewFamily) override;
#else
	void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
	void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	void PostRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
#endif
	void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;

private:
	int32 PreviousFSR3State;
	int32 PreviousFSR3StateRT;
	int32 CurrentFSR3StateRT;
	float MinAutomaticViewMipBiasMin;
	float MinAutomaticViewMipBiasOffset;
	int32 VertexDeformationOutputsVelocity;
	int32 VelocityEnableLandscapeGrass;
	int32 BasePassForceOutputsVelocity;
	int32 SeparateTranslucency;
	int32 SSRExperimentalDenoiser;
	bool bFSR3Supported;
};