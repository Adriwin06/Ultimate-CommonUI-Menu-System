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
#include "RenderGraphDefinitions.h"
#include "StreamlineCore.h"
class FStreamlineRHI;

bool IsDeepDVCActive();

enum class Streamline::EStreamlineFeatureSupport;
extern STREAMLINECORE_API Streamline::EStreamlineFeatureSupport QueryStreamlineDeepDVCSupport();
extern STREAMLINECORE_API bool IsStreamlineDeepDVCSupported();



class FRHICommandListImmediate;
struct FRHIStreamlineArguments;
class FSceneViewFamily;
class FRDGBuilder;
void AddStreamlineDeepDVCStateRenderPass(FRDGBuilder& GraphBuilder, uint32 ViewID, const FIntRect& SecondaryViewRect);
void AddStreamlineDeepDVCEvaluateRenderPass(FStreamlineRHI* StreamlineRHIExtensions, FRDGBuilder& GraphBuilder, uint32 ViewID, const FIntRect& SecondaryViewRect, FRDGTextureRef SLSceneColorWithoutHUD);
void BeginRenderViewFamilyDeepDVC(FSceneViewFamily& InViewFamily);
void GetDeepDVCStatusFromStreamline();
