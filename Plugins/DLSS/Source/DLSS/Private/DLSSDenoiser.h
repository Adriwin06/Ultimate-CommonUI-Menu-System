/*
* Copyright (c) 2020 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#include "Runtime/Launch/Resources/Version.h"
#include "ScreenSpaceDenoise.h"
class FDLSSUpscaler;

// wrapper for the default denoiser to add TAA after some passes
class DLSS_API FDLSSDenoiser final : public IScreenSpaceDenoiser
{

public:
	FDLSSDenoiser(const IScreenSpaceDenoiser* InWrappedDenoiser, const FDLSSUpscaler* InUpscaler);

	// Inherited via IScreenSpaceDenoiser
	virtual const TCHAR* GetDebugName() const final override;
	virtual EShadowRequirements GetShadowRequirements(const FViewInfo& View, const FLightSceneInfo& LightSceneInfo, const FShadowRayTracingConfig& RayTracingConfig) const final override;
	virtual void DenoiseShadowVisibilityMasks(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPreviousViewInfo* PreviousViewInfos, const FSceneTextureParameters& SceneTextures, const TStaticArray<FShadowVisibilityParameters, IScreenSpaceDenoiser::kMaxBatchSize>& InputParameters, const int32 InputParameterCount, TStaticArray<FShadowVisibilityOutputs, IScreenSpaceDenoiser::kMaxBatchSize>& Outputs) const final override;
	virtual FPolychromaticPenumbraOutputs DenoisePolychromaticPenumbraHarmonics(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPreviousViewInfo* PreviousViewInfos, const FSceneTextureParameters& SceneTextures, const FPolychromaticPenumbraHarmonics& Inputs) const final override;
	virtual FReflectionsOutputs DenoiseReflections(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPreviousViewInfo* PreviousViewInfos, const FSceneTextureParameters& SceneTextures, const FReflectionsInputs& Inputs, const FReflectionsRayTracingConfig Config) const final override;
	virtual FReflectionsOutputs DenoiseWaterReflections(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPreviousViewInfo* PreviousViewInfos, const FSceneTextureParameters& SceneTextures, const FReflectionsInputs& Inputs, const FReflectionsRayTracingConfig Config) const override;
	virtual FAmbientOcclusionOutputs DenoiseAmbientOcclusion(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPreviousViewInfo* PreviousViewInfos, const FSceneTextureParameters& SceneTextures, const FAmbientOcclusionInputs& Inputs, const FAmbientOcclusionRayTracingConfig Config) const final override;
	virtual FSSDSignalTextures DenoiseDiffuseIndirect(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPreviousViewInfo* PreviousViewInfos, const FSceneTextureParameters& SceneTextures, const FDiffuseIndirectInputs& Inputs, const FAmbientOcclusionRayTracingConfig Config) const final override;
#if defined(ENGINE_HAS_DENOISE_INDIRECT) && ENGINE_HAS_DENOISE_INDIRECT
	virtual FSSDSignalTextures DenoiseIndirect(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPreviousViewInfo* PreviousViewInfos, const FSceneTextureParameters& SceneTextures, const FIndirectInputs& Inputs, const FAmbientOcclusionRayTracingConfig Config) const final override;
#endif
	virtual FSSDSignalTextures DenoiseScreenSpaceDiffuseIndirect(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPreviousViewInfo* PreviousViewInfos, const FSceneTextureParameters& SceneTextures, const FDiffuseIndirectInputs& Inputs, const FAmbientOcclusionRayTracingConfig Config) const final override;
	virtual FSSDSignalTextures DenoiseDiffuseIndirectHarmonic(FRDGBuilder& GraphBuilder,const FViewInfo& View,FPreviousViewInfo* PreviousViewInfos,	const FSceneTextureParameters& SceneTextures,		const FDiffuseIndirectHarmonic& Inputs, const HybridIndirectLighting::FCommonParameters& CommonDiffuseParameters) const final override;
	virtual FDiffuseIndirectOutputs DenoiseSkyLight(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPreviousViewInfo* PreviousViewInfos, const FSceneTextureParameters& SceneTextures, const FDiffuseIndirectInputs& Inputs, const FAmbientOcclusionRayTracingConfig Config) const final override;
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION < 4)
	virtual FDiffuseIndirectOutputs DenoiseReflectedSkyLight(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPreviousViewInfo* PreviousViewInfos, const FSceneTextureParameters& SceneTextures, const FDiffuseIndirectInputs& Inputs, const FAmbientOcclusionRayTracingConfig Config) const final override;
#endif
	virtual bool SupportsScreenSpaceDiffuseIndirectDenoiser(EShaderPlatform Platform) const final override;


	const IScreenSpaceDenoiser* GetWrappedDenoiser() const;
private:

	bool SkipDenoiser(const FViewInfo& View) const;
	bool IsDLSSActive(const FViewInfo& View) const;
	const IScreenSpaceDenoiser* WrappedDenoiser;
	const FDLSSUpscaler* Upscaler;
};