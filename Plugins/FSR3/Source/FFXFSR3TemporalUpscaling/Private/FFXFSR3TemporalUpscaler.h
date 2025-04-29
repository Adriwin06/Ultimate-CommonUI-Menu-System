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

#include "Engine/Engine.h"
#include "PostProcess/PostProcessing.h"
#include "PostProcess/PostProcessUpscale.h"
#include "PostProcess/TemporalAA.h"
#include "ScreenSpaceDenoise.h"
#include "Containers/LockFreeList.h"
#include "FFXFSR3TemporalUpscalerHistory.h"
#include "FFXSharedBackend.h"

#if UE_VERSION_AT_LEAST(5, 3, 0)
#include "TemporalUpscaler.h"
using IFFXFSR3TemporalUpscaler = UE::Renderer::Private::ITemporalUpscaler;
using FFXFSR3PassInput = UE::Renderer::Private::ITemporalUpscaler::FInputs;
using FFXFSR3View = FSceneView;
#else
using IFFXFSR3TemporalUpscaler = ITemporalUpscaler;
using FFXFSR3PassInput = ITemporalUpscaler::FPassInputs;
using FFXFSR3View = FViewInfo;
#endif

#ifndef ENGINE_HAS_DENOISE_INDIRECT
#define ENGINE_HAS_DENOISE_INDIRECT 0
#endif

namespace FFXFSR3Strings
{
	static constexpr auto D3D12 = TEXT("D3D12");
}

struct FPostProcessingInputs;

#if UE_VERSION_OLDER_THAN(5, 0, 0)
//-------------------------------------------------------------------------------------
// Simplifies cross engine support.
//-------------------------------------------------------------------------------------
typedef IScreenSpaceDenoiser::FDiffuseIndirectOutputs FSSDSignalTextures;
#endif

//-------------------------------------------------------------------------------------
// The core upscaler implementation for FSR3.
// Implements IScreenSpaceDenoiser in order to access the reflection texture data.
//-------------------------------------------------------------------------------------
class FFXFSR3TemporalUpscaler final : public IFFXFSR3TemporalUpscaler, public IScreenSpaceDenoiser
{
	friend class FFXFSR3FXSystem;
public:
	FFXFSR3TemporalUpscaler();
	virtual ~FFXFSR3TemporalUpscaler();

	void Initialize() const;

	const TCHAR* GetDebugName() const override;

	void ReleaseState(FSR3StateRef State);

	static class IFFXSharedBackend* GetApiAccessor(EFFXBackendAPI& Api);
	static float GetResolutionFraction(uint32 Mode);

#if DO_CHECK || DO_GUARD_SLOW || DO_ENSURE
	static void OnFSRMessage(uint32 type, const wchar_t* message);
#endif // DO_CHECK || DO_GUARD_SLOW || DO_ENSURE

	static void SaveScreenPercentage();
	static void UpdateScreenPercentage();
	static void RestoreScreenPercentage();

	static void OnChangeFFXFSR3Enabled(IConsoleVariable* Var);
	static void OnChangeFFXFSR3QualityMode(IConsoleVariable* Var);

	class FRDGBuilder* GetGraphBuilder();

#if UE_VERSION_AT_LEAST(5, 0, 0)
	IFFXFSR3TemporalUpscaler::FOutputs AddPasses(
		FRDGBuilder& GraphBuilder,
		const FFXFSR3View& View,
		const FFXFSR3PassInput& PassInputs) const override;
#else		
	void AddPasses(
		FRDGBuilder& GraphBuilder,
		const FFXFSR3View& View,
		const FFXFSR3PassInput& PassInputs,
		FRDGTextureRef* OutSceneColorTexture,
		FIntRect* OutSceneColorViewRect,
		FRDGTextureRef* OutSceneColorHalfResTexture,
		FIntRect* OutSceneColorHalfResViewRect) const override;
#endif

#if UE_VERSION_AT_LEAST(5, 1, 0)
	IFFXFSR3TemporalUpscaler* Fork_GameThread(const class FSceneViewFamily& InViewFamily) const override;
#endif

	float GetMinUpsampleResolutionFraction() const override;
	float GetMaxUpsampleResolutionFraction() const override;

	void SetSSRShader(FGlobalShaderMap* GlobalMap);

	void CopyOpaqueSceneColor(FRHICommandListImmediate& RHICmdList, FRHIUniformBuffer* ViewUniformBuffer, const class FShaderParametersMetadata* SceneTexturesUniformBufferStruct, FRHIUniformBuffer* SceneTexturesUniformBuffer);

	void SetLumenReflections(FSceneView& InView);

	void SetPostProcessingInputs(FPostProcessingInputs const& Inputs);

	void EndOfFrame();

	void UpdateDynamicResolutionState();

#if WITH_EDITOR
	bool IsEnabledInEditor() const;
	void SetEnabledInEditor(bool bEnabled);
#endif

	FReflectionsOutputs DenoiseReflections(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FReflectionsInputs& ReflectionInputs,
		const FReflectionsRayTracingConfig RayTracingConfig) const override;

	EShadowRequirements GetShadowRequirements(
		const FViewInfo& View,
		const FLightSceneInfo& LightSceneInfo,
		const FShadowRayTracingConfig& RayTracingConfig) const override;

	void DenoiseShadowVisibilityMasks(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const TStaticArray<FShadowVisibilityParameters, IScreenSpaceDenoiser::kMaxBatchSize>& InputParameters,
		const int32 InputParameterCount,
		TStaticArray<FShadowVisibilityOutputs, IScreenSpaceDenoiser::kMaxBatchSize>& Outputs) const override;

	FPolychromaticPenumbraOutputs DenoisePolychromaticPenumbraHarmonics(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FPolychromaticPenumbraHarmonics& Inputs) const override;

	FReflectionsOutputs DenoiseWaterReflections(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FReflectionsInputs& ReflectionInputs,
		const FReflectionsRayTracingConfig RayTracingConfig) const override;

	FAmbientOcclusionOutputs DenoiseAmbientOcclusion(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FAmbientOcclusionInputs& ReflectionInputs,
		const FAmbientOcclusionRayTracingConfig RayTracingConfig) const override;

	FSSDSignalTextures DenoiseDiffuseIndirect(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FDiffuseIndirectInputs& Inputs,
		const FAmbientOcclusionRayTracingConfig Config) const override;

#if ENGINE_HAS_DENOISE_INDIRECT
	FSSDSignalTextures DenoiseIndirect(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FIndirectInputs& Inputs,
		const FAmbientOcclusionRayTracingConfig Config) const override;
#endif

	FDiffuseIndirectOutputs DenoiseSkyLight(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FDiffuseIndirectInputs& Inputs,
		const FAmbientOcclusionRayTracingConfig Config) const override;

#if UE_VERSION_OLDER_THAN(5, 4, 0)
	FDiffuseIndirectOutputs DenoiseReflectedSkyLight(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FDiffuseIndirectInputs& Inputs,
		const FAmbientOcclusionRayTracingConfig Config) const override;
#endif

#if UE_VERSION_AT_LEAST(5, 0, 0)
	FSSDSignalTextures DenoiseDiffuseIndirectHarmonic(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FDiffuseIndirectHarmonic& Inputs,
		const HybridIndirectLighting::FCommonParameters& CommonDiffuseParameters) const override;
#else
	FDiffuseIndirectHarmonic DenoiseDiffuseIndirectHarmonic(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FDiffuseIndirectHarmonic& Inputs,
		const FAmbientOcclusionRayTracingConfig Config) const override;
#endif

	bool SupportsScreenSpaceDiffuseIndirectDenoiser(EShaderPlatform Platform) const override;

	FSSDSignalTextures DenoiseScreenSpaceDiffuseIndirect(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		FPreviousViewInfo* PreviousViewInfos,
		const FSceneTextureParameters& SceneTextures,
		const FDiffuseIndirectInputs& Inputs,
		const FAmbientOcclusionRayTracingConfig Config) const override;

	inline bool IsApiSupported() const
	{
		return Api != EFFXBackendAPI::Unknown && Api != EFFXBackendAPI::Unsupported;
	}

private:
	void DeferredCleanup(uint64 FrameNum) const;

	mutable FPostProcessingInputs PostInputs;
	FDynamicResolutionStateInfos DynamicResolutionStateInfos;
	mutable FCriticalSection Mutex;
	mutable TSet<FSR3StateRef> AvailableStates;
	mutable EFFXBackendAPI Api;
	mutable class IFFXSharedBackend* ApiAccessor;
	mutable class FRDGBuilder* CurrentGraphBuilder;
	mutable const IScreenSpaceDenoiser* WrappedDenoiser;
	mutable FRDGTextureRef ReflectionTexture;
	mutable FTextureRHIRef SceneColorPreAlpha;
	mutable TRefCountPtr<IPooledRenderTarget> SceneColorPreAlphaRT;
	mutable FTextureRHIRef CustomStencil;
	mutable TRefCountPtr<IPooledRenderTarget> CustomStencilRT;
	mutable TRefCountPtr<IPooledRenderTarget> MotionVectorRT;
	mutable TRefCountPtr<IPooledRenderTarget> LumenReflections;
	mutable FRDGTextureMSAA PreAlpha;
#if WITH_EDITOR
	bool bEnabledInEditor;
#endif
	static float SavedScreenPercentage;
#if UE_VERSION_AT_LEAST(5, 2, 0)
	mutable TRefCountPtr<IPooledRenderTarget> ReactiveExtractedTexture;
	mutable TRefCountPtr<IPooledRenderTarget> CompositeExtractedTexture;
#endif
};