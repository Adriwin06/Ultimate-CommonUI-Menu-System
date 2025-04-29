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

#include "CoreMinimal.h"
#include "FFXFrameInterpolationApi.h"
#include "FFXSharedBackend.h"
#include "IFFXFrameInterpolation.h"
#include "RendererInterface.h"

//-------------------------------------------------------------------------------------
// Enumeration of the present status inside the custom present object.
//-------------------------------------------------------------------------------------
enum class FFXFrameInterpolationCustomPresentStatus : uint8
{
	InterpolateRT = 0,
	InterpolateRHI = 1,
	PresentRT = 2,
	PresentRHI = 3
};

//-------------------------------------------------------------------------------------
// RHI resource that ensures the lifetime of Frame Interpolation resources & contexts.
//-------------------------------------------------------------------------------------
struct FFXFrameInterpolationResources : public FRHIResource
{
	FFXFrameInterpolationResources(IFFXSharedBackend* InBackend, uint32 UniqueID);
	~FFXFrameInterpolationResources();

	uint32 AddRef() const
	{
		return FRHIResource::AddRef();
	}

	uint32 Release() const
	{
		return FRHIResource::Release();
	}

	uint32 GetRefCount() const
	{
		return FRHIResource::GetRefCount();
	}

	uint32 UniqueID;
	ffxCreateContextDescFrameGeneration Desc;
	ffxContext Context;
	TRefCountPtr<IPooledRenderTarget> Color;
	TRefCountPtr<IPooledRenderTarget> Hud;
	TRefCountPtr<IPooledRenderTarget> Inter;
	TRefCountPtr<IPooledRenderTarget> MotionVectorRT;
	FTextureRHIRef Distortion;
	IFFXSharedBackend* Backend;
	bool bDebugView;
};
typedef TRefCountPtr<FFXFrameInterpolationResources> FFXFIResourceRef;

//-------------------------------------------------------------------------------------
// Custom present implementation that handles frame interpolation.
//-------------------------------------------------------------------------------------
class FFXFrameInterpolationCustomPresent : public IFFXFrameInterpolationCustomPresent
{
    struct FFXFrameInterpolationFrame
    {
        TRefCountPtr<IPooledRenderTarget> RealFrame;
        TRefCountPtr<IPooledRenderTarget> Interpolated;
    };

	ffxCreateContextDescFrameGeneration Desc;
	TRefCountPtr<IPooledRenderTarget> RealFrameNoUI;
	TRefCountPtr<IPooledRenderTarget> InterpolatedNoUI;
    FFXFrameInterpolationFrame Current;
	IFFXSharedBackend* Backend;
	FViewport* Viewport;
	FRHIViewport* RHIViewport;
	FTextureRHIRef BackBuffer;
	FFXFIResourceRef CurrentResource;
	TArray<FFXFIResourceRef> Resources;
	TArray<FFXFIResourceRef> OldResources;
	FFXFrameInterpolationCustomPresentStatus Status;
	EFFXFrameInterpolationPresentMode Mode;
	EFFXBackendAPI Api;
	bool bNeedsNativePresentRT;
	bool bPresentRHI;
	bool bHasValidInterpolatedRT;
	bool bEnabled;
	bool bResized;
	bool bUseFFXSwapchain;
	bool bHasInterpolatedRT;
	bool bHasInterpolatedRHI;
public:
	FFXFrameInterpolationCustomPresent();
	virtual ~FFXFrameInterpolationCustomPresent();

    // Called by our custom code for intialising the swap chain.
    bool InitSwapChain(IFFXSharedBackend* InBackend, uint32_t Flags, FIntPoint RenderSize, FIntPoint DisplaySize, FfxSwapchain RawSwapChain, FfxCommandQueue Queue, FfxApiSurfaceFormat Format, EFFXBackendAPI Api);

    void InitViewport(FViewport* InViewport, FViewportRHIRef ViewportRHI) final;

	IFFXSharedBackend* GetBackend() const { return Backend; }

	ffxContext* GetContext() const
	{
		ffxContext* Resource = nullptr;
		if (CurrentResource.IsValid())
		{
			Resource = &CurrentResource->Context;
		}
		return Resource;
	}

	bool Enabled() const { return bEnabled; }

	// Called when viewport is resized.
	void OnBackBufferResize() override final;

	// Called from render thread to see if a native present will be requested for this frame.
	// @return	true if native Present will be requested for this frame; false otherwise.  Must
	// match value subsequently returned by Present for this frame.
	bool NeedsNativePresent() override final;
	// In come cases we want to use custom present but still let the native environment handle 
	// advancement of the backbuffer indices.
	// @return true if backbuffer index should advance independently from CustomPresent.
	bool NeedsAdvanceBackbuffer()  override final;

	// Called from RHI thread when the engine begins drawing to the viewport.
	void BeginDrawing() override final;

	// Called from RHI thread to perform custom present.
	// @param InOutSyncInterval - in out param, indicates if vsync is on (>0) or off (==0).
	// @return	true if native Present should be also be performed; false otherwise. If it returns
	// true, then InOutSyncInterval could be modified to switch between VSync/NoVSync for the normal 
	// Present.  Must match value previously returned by NeedsNativePresent for this frame.
	bool Present(int32& InOutSyncInterval) override final;

	// Called from RHI thread after native Present has been called
	void PostPresent() override final;

	// Called when rendering thread is acquired
	void OnAcquireThreadOwnership() override final;
	// Called when rendering thread is released
	void OnReleaseThreadOwnership() override final;

	void CopyBackBufferRT(FTextureRHIRef InBackBuffer);
	void SetEnabled(bool const bEnabled);
	void SetMode(EFFXFrameInterpolationPresentMode Mode) override final;
	void SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus Flag);
	EFFXFrameInterpolationPresentMode GetMode() const { return Mode; }
	bool GetUseFFXSwapchain() const { return bUseFFXSwapchain; }
	void SetUseFFXSwapchain(bool const bEnabled) override final;;

	FFXFIResourceRef UpdateContexts(FRDGBuilder& GraphBuilder, uint32 UniqueID, ffxDispatchDescFrameGenerationPrepare const& FsrDesc, ffxCreateContextDescFrameGeneration const& FgDesc);

	void SetPreUITextures(TRefCountPtr<IPooledRenderTarget> InRealFrameNoUI, TRefCountPtr<IPooledRenderTarget> InInterpolatedNoUI)
	{
		RealFrameNoUI = InRealFrameNoUI;
		InterpolatedNoUI = InInterpolatedNoUI;
	}

	void BeginFrame()
	{
		OldResources = Resources;
		check(Resources.Num() == 0 || OldResources.Num() > 0);
		Resources.Empty();
	}
	void EndFrame()
	{
		OldResources.Empty();
	}

	bool Resized()
	{
		return bResized;
	}
};
