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
#include "SceneViewExtension.h"

#include "IFFXFrameInterpolation.h"

//-------------------------------------------------------------------------------------
// Forward declarations.
//-------------------------------------------------------------------------------------
class FRDGBuilder;
class SWindow;
class FViewport;
class FFXFrameInterpolationViewExtension;
class FFXFrameInterpolationCustomPresent;
struct FfxFrameInterpolationContext;

//-------------------------------------------------------------------------------------
// Class that actually implements frame interpolation.
//-------------------------------------------------------------------------------------
class FFXFrameInterpolation : public IFFXFrameInterpolation
{
public:
	FFXFrameInterpolation();
	virtual ~FFXFrameInterpolation();

	void OnPostEngineInit();
	void OnViewportCreatedHandler_SetCustomPresent();
	void OnBeginDrawHandler();
    void SetupView(const FSceneView& View, const FPostProcessingInputs& Inputs);
	void InterpolateFrame(FRDGBuilder& GraphBuilder);
	void OnSlateWindowRendered(SWindow& SlateWindow, void* ViewportRHIPtr);
#if UE_VERSION_AT_LEAST(5, 5, 0)
	void OnBackBufferReadyToPresentCallback(class SWindow& SlateWindow, const FTextureRHIRef& BackBuffer);
#else
	void OnBackBufferReadyToPresentCallback(class SWindow& SlateWindow, const FTexture2DRHIRef& BackBuffer);
#endif

	IFFXFrameInterpolationCustomPresent* CreateCustomPresent(IFFXSharedBackend* Backend, uint32_t Flags, FIntPoint RenderSize, FIntPoint DisplaySize, FfxSwapchain RawSwapChain, FfxCommandQueue Queue, FfxApiSurfaceFormat Format, EFFXBackendAPI Api) final;
	bool GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS) final;

private:
	struct FFXFrameInterpolationView
	{
		FRDGTextureRef ViewFamilyTexture;
		FRDGTextureRef SceneDepth;
		FRDGTextureRef SceneVelocity;
		FIntRect ViewRect;
		FIntPoint InputExtentsQuantized;
		FIntPoint OutputExtents;
		FVector2D TemporalJitterPixels;
		float CameraNear;
		float CameraFOV;
		float GameTimeMs;
		bool bReset;
		bool bEnabled;
	};
	void CalculateFPSTimings();
	bool InterpolateView(FRDGBuilder& GraphBuilder, FFXFrameInterpolationCustomPresent* Presenter, const FSceneView* View, FFXFrameInterpolationView const& ViewDesc, FRDGTextureRef FinalBuffer, FRDGTextureRef InterpolatedRDG, FRDGTextureRef BackBufferRDG, uint32 Index);
	TMap<const FSceneView*, FFXFrameInterpolationView> Views;
	TSharedPtr<FFXFrameInterpolationViewExtension, ESPMode::ThreadSafe> ViewExtension;
    TRefCountPtr<IPooledRenderTarget> BackBufferRT;
	TRefCountPtr<IPooledRenderTarget> InterpolatedRT;
	TRefCountPtr<IPooledRenderTarget> AsyncBufferRT[2];
	TMap<FfxSwapchain, FFXFrameInterpolationCustomPresent*> SwapChains;
	TMap<SWindow*, FRHIViewport*> Windows;
	float GameDeltaTime;
	double LastTime;
	float AverageTime;
	float AverageFPS;
	uint64 InterpolationCount;
	uint64 PresentCount;
	uint32 Index;
	uint32 ResetState;
	bool bInterpolatedFrame;
};
