/*******************************************************************************
 * Copyright 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#include "XeSSMacros.h"

#include "CoreMinimal.h"

#if XESS_ENGINE_VERSION_GEQ(5, 1)
#include "SceneViewExtension.h"
#else // XESS_ENGINE_VERSION_GEQ(5, 1)
#include "CustomStaticScreenPercentage.h"
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)

#if XESS_ENGINE_VERSION_GEQ(5, 3)
#include "TemporalUpscaler.h"		// for ITemporalUpscaler
struct FTemporalAAHistory;
typedef UE::Renderer::Private::ITemporalUpscaler::FInputs XPassInputs;
#else // XESS_ENGINE_VERSION_GEQ(5, 3)
#include "PostProcess/TemporalAA.h"  // for ITemporalUpscaler
typedef ITemporalUpscaler::FPassInputs XPassInputs;
#endif // XESS_ENGINE_VERSION_GEQ(5, 3)

class FXeSSRHI;

/** XeSS configuration parameters */
struct FXeSSPassParameters
{
	// Viewport rectangle of the input and output of XeSS.
	FIntRect InputViewRect;
	FIntRect OutputViewRect;

	// Render resolution input texture
	FRDGTexture* SceneColorTexture = nullptr;
	// Full resolution depth, history and velocity textures to reproject the history.
	FRDGTexture* SceneDepthTexture = nullptr;
	FRDGTexture* SceneVelocityTexture = nullptr;

	FXeSSPassParameters(const FViewInfo& View, const XPassInputs& PassInputs);

	/** Returns the texture resolution that will be output. */
	FIntPoint GetOutputExtent() const;

	/** Validate the settings of XeSS, to make sure there is no issue. */
	bool Validate() const;
};

#if XESS_ENGINE_VERSION_GEQ(5, 3)
class XESSPLUGIN_API FXeSSUpscaler final : public UE::Renderer::Private::ITemporalUpscaler
#elif XESS_ENGINE_VERSION_GEQ(5, 1)
class XESSPLUGIN_API FXeSSUpscaler final : public ITemporalUpscaler
#else // XESS_ENGINE_VERSION_GEQ(5, 1)
class XESSPLUGIN_API FXeSSUpscaler final : public ITemporalUpscaler, public ICustomStaticScreenPercentage
#endif // XESS_ENGINE_VERSION_GEQ(5, 3)
{
public:
	FXeSSUpscaler(FXeSSRHI* InXeSSRHI);
	virtual ~FXeSSUpscaler();

	// Inherited via ITemporalUpscaler
	virtual const TCHAR* GetDebugName() const final;

#if XESS_ENGINE_VERSION_GEQ(5, 3)
	virtual FOutputs AddPasses(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FInputs& PassInputs) const final;
#elif XESS_ENGINE_VERSION_GEQ(5, 0)
	virtual FOutputs AddPasses(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& ViewInfo,
		const FPassInputs& PassInputs) const final;
#else // XESS_ENGINE_VERSION_GEQ(5, 0)
	virtual void AddPasses(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& ViewInfo,
		const FPassInputs& PassInputs,
		FRDGTextureRef* OutSceneColorTexture,
		FIntRect* OutSceneColorViewRect,
		FRDGTextureRef* OutSceneColorHalfResTexture,
		FIntRect* OutSceneColorHalfResViewRect) const final;
#endif // XESS_ENGINE_VERSION_GEQ(5, 3)

	FRDGTextureRef AddMainXeSSPass(
		FRDGBuilder& GraphBuilder,
		const FViewInfo& View,
		const FXeSSPassParameters& Inputs,
		const FTemporalAAHistory& InputHistory,
		FTemporalAAHistory* OutputHistory) const;

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	ITemporalUpscaler* Fork_GameThread(const class FSceneViewFamily& ViewFamily) const final;
	// Called by FXeSSUpscalerViewExtension
	void SetupViewFamily(FSceneViewFamily& ViewFamily);
#else // XESS_ENGINE_VERSION_GEQ(5, 1)
	// Inherited via ICustomStaticScreenPercentage
	virtual void SetupMainGameViewFamily(FSceneViewFamily& ViewFamily) final;

#if XESS_ENGINE_VERSION_GEQ(4, 27)
	// For generic cases where view family isn't a game view family. Example: MovieRenderQueue. 
	virtual void SetupViewFamily(FSceneViewFamily& ViewFamily, TSharedPtr<ICustomStaticScreenPercentageData> InScreenPercentageDataInterface) final;
#endif // XESS_ENGINE_VERSION_GEQ(4, 27)

#endif // XESS_ENGINE_VERSION_GEQ(5, 1)

	virtual float GetMinUpsampleResolutionFraction() const final;
	virtual float GetMaxUpsampleResolutionFraction() const final;

	bool IsXeSSEnabled() const;

#if XESS_ENGINE_VERSION_LSS(5, 1)
	void HandleXeSSEnabledSet(IConsoleVariable* Variable);
private:
	// Used by `r.XeSS.Enabled` console variable `OnChanged` callback
	bool bCurrentXeSSEnabled = false;
#endif // XESS_ENGINE_VERSION_LSS(5, 1)

private:
	static FXeSSRHI* UpscalerXeSSRHI;

#if XESS_ENGINE_VERSION_GEQ(5, 3)
	TRefCountPtr<UE::Renderer::Private::ITemporalUpscaler::IHistory> DummyHistory;
#endif // XESS_ENGINE_VERSION_GEQ(5, 3)
};

#if XESS_ENGINE_VERSION_GEQ(5, 1)
class XESSPLUGIN_API FXeSSUpscalerViewExtension : public FSceneViewExtensionBase
{
public:
	FXeSSUpscalerViewExtension(const FAutoRegister& AutoRegister, FXeSSUpscaler* InXeSSUpscaler) : 
		FSceneViewExtensionBase(AutoRegister),
		XeSSUpscaler(InXeSSUpscaler) {}
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;
	// The only choice for `FSceneViewFamily::SetTemporalUpscalerInterface` (limited by engine code)
	virtual void BeginRenderViewFamily(FSceneViewFamily& ViewFamily) override;
	// Empty implementation for pure virtual
	virtual void SetupView(FSceneViewFamily& ViewFamily, FSceneView& View) override {};
	// Empty implementation for pure virtual
	virtual void SetupViewFamily(FSceneViewFamily& ViewFamily) override {};
private:
	FXeSSUpscaler* XeSSUpscaler = nullptr;
};

#endif // XESS_ENGINE_VERSION_GEQ(5, 1)
