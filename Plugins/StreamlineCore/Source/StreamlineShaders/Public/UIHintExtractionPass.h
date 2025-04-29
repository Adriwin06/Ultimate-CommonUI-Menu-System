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
#include "RendererInterface.h"
#include "ScreenPass.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 4  || ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 1
#define FTextureRHIRef FTexture2DRHIRef
#endif

extern STREAMLINESHADERS_API FRDGTextureRef AddStreamlineUIHintExtractionPass(
	FRDGBuilder& GraphBuilder,
	//	const FViewInfo& View,
	const float InAlphaThresholdValue,
	const FTextureRHIRef& InBackBuffer
	//	FRDGTextureRef InVelocityTexture
);
