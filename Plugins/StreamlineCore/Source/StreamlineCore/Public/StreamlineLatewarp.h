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
#include "StreamlineCore.h"


class FStreamlineRHI;

bool IsLatewarpActive();

enum class Streamline::EStreamlineFeatureSupport;

class FRHICommandListImmediate;
struct FRHIStreamlineArguments;
class FSceneViewFamily;
class FRDGBuilder;

extern STREAMLINECORE_API Streamline::EStreamlineFeatureSupport QueryStreamlineLatewarpSupport();
extern STREAMLINECORE_API bool IsStreamlineLatewarpSupported();
void RegisterStreamlineLatewarpHooks(FStreamlineRHI* InStreamlineRHI);
void UnregisterStreamlineLatewarpHooks();
void AddStreamlineLatewarpStateRenderPass(FRDGBuilder& GraphBuilder, uint32 ViewID, const FIntRect& SecondaryViewRect);
