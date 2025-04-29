/*
* Copyright (c) 2022 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/
#pragma once

#include "CoreMinimal.h"
#include "Slate/SceneViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "RenderGraphBuilder.h"

DECLARE_LOG_CATEGORY_EXTERN(LogStreamline, Verbose, All);

bool ShouldTagStreamlineBuffers();
bool ForceTagStreamlineBuffers();
bool NeedStreamlineViewIdOverride();

namespace sl
{
	enum class Result;
}
namespace Streamline
{
	enum class EStreamlineFeatureSupport;
}

Streamline::EStreamlineFeatureSupport TranslateStreamlineResult(sl::Result Result);



BEGIN_SHADER_PARAMETER_STRUCT(FSLSetStateShaderParameters, )
END_SHADER_PARAMETER_STRUCT()

template<typename StateOnRenderThreadLambda, typename RHIThreadLambda >
void AddStreamlineStateRenderPass(const TCHAR* FeatureName, FRDGBuilder& GraphBuilder, uint32 ViewID, const FIntRect& SecondaryViewRect, StateOnRenderThreadLambda&& StateOnRenderThread, RHIThreadLambda&& OnRHIThread)
{
	FSLSetStateShaderParameters* PassParameters = GraphBuilder.AllocParameters<FSLSetStateShaderParameters>();

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("Streamline %s State ViewID = % u", FeatureName, ViewID),
		PassParameters,
		ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass | ERDGPassFlags::NeverCull,
		[PassParameters, ViewID, SecondaryViewRect, &OnRHIThread, &StateOnRenderThread](FRHICommandListImmediate& RHICmdList) mutable
		{
			auto Options = StateOnRenderThread(ViewID, SecondaryViewRect);

			RHICmdList.EnqueueLambda(
				[ViewID, SecondaryViewRect, Options, &OnRHIThread](FRHICommandListImmediate& Cmd) mutable
				{
					OnRHIThread(Cmd, ViewID, SecondaryViewRect, Options);
				});
		});
}