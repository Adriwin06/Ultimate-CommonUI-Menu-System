/*
* Copyright (c) 2022 - 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

DECLARE_LOG_CATEGORY_EXTERN(LogStreamline, Verbose, All);

bool ShouldTagStreamlineBuffersForDLSSFG();
bool ForceTagStreamlineBuffers();
bool HasViewIdOverride();

namespace sl
{
	enum class Result;
}

enum class EStreamlineFeatureSupport;

EStreamlineFeatureSupport TranslateStreamlineResult(sl::Result Result);