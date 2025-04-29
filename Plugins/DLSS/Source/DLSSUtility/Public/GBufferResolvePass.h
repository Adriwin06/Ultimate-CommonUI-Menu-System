/*
* Copyright (c) 2023 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#include "Runtime/Launch/Resources/Version.h"
#include "ScreenPass.h"
#include "SceneTexturesConfig.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "TemporalUpscaler.h"
using ITemporalUpscaler = UE::Renderer::Private::ITemporalUpscaler;
#endif

#ifndef SUPPORT_GUIDE_GBUFFER
#define SUPPORT_GUIDE_GBUFFER 0
#endif

struct FGBufferResolveOutputs
{
	FRDGTextureRef DiffuseAlbedo = nullptr;
	FRDGTextureRef SpecularAlbedo = nullptr;
	
	FRDGTextureRef Normals = nullptr;
	FRDGTextureRef Roughness = nullptr;
	
	FRDGTextureRef LinearDepth = nullptr;

#if SUPPORT_GUIDE_GBUFFER
	FRDGTextureRef ReflectionHitDistance = nullptr;
#endif
};

extern DLSSUTILITY_API FGBufferResolveOutputs AddGBufferResolvePass(
	FRDGBuilder& GraphBuilder,
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	const FSceneView& View,
	const ITemporalUpscaler::FInputs& PassInputs,
#else
	const FViewInfo& View,
#endif
	FIntRect InputViewRect,
	const bool bComputeDiffuseSpecularAlbedo
);
