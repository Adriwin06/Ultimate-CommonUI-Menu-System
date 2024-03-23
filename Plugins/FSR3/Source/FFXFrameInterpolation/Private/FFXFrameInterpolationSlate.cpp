// This file is part of the FidelityFX Super Resolution 3.0 Unreal Engine Plugin.
//
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include "FFXFrameInterpolationSlate.h"

//------------------------------------------------------------------------------------------------------
// Helper definitions.
//------------------------------------------------------------------------------------------------------
struct FSlateReleaseDrawBufferCommandString
{
	static const TCHAR* TStr() { return TEXT("FSlateReleaseDrawBufferCommand"); }
};

struct FFISlateReleaseDrawBufferCommand final : public FRHICommand < FFISlateReleaseDrawBufferCommand, FSlateReleaseDrawBufferCommandString >
{
	FSlateDrawBuffer* DrawBuffer;

	FFISlateReleaseDrawBufferCommand(FSlateDrawBuffer* InDrawBuffer)
		: DrawBuffer(InDrawBuffer)
	{}

	void Execute(FRHICommandListBase& CmdList)
	{
		DrawBuffer->Unlock();
	}

	static void ReleaseDrawBuffer(FRHICommandListImmediate& RHICmdList, FSlateDrawBuffer* InDrawBuffer)
	{
		if (!RHICmdList.Bypass())
		{
			ALLOC_COMMAND_CL(RHICmdList, FFISlateReleaseDrawBufferCommand)(InDrawBuffer);
		}
		else
		{
			FFISlateReleaseDrawBufferCommand Cmd(InDrawBuffer);
			Cmd.Execute(RHICmdList);
		}
	}
};

//------------------------------------------------------------------------------------------------------
// Implementation for the SlateRenderer override that allows for more draw buffers.
//------------------------------------------------------------------------------------------------------
FFXFrameInterpolationSlateRenderer::FFXFrameInterpolationSlateRenderer(TSharedRef<FSlateRenderer> InUnderlyingRenderer)
: FSlateRenderer(InUnderlyingRenderer->GetFontServices())
, UnderlyingRenderer(InUnderlyingRenderer)
, FreeBufferIndex(0)
, ResourceVersion(0)
{
    InUnderlyingRenderer->OnSlateWindowRendered().AddRaw(this, &FFXFrameInterpolationSlateRenderer::OnSlateWindowRenderedThunk);
    InUnderlyingRenderer->OnSlateWindowDestroyed().AddRaw(this, &FFXFrameInterpolationSlateRenderer::OnSlateWindowDestroyedThunk);
    InUnderlyingRenderer->OnPreResizeWindowBackBuffer().AddRaw(this, &FFXFrameInterpolationSlateRenderer::OnPreResizeWindowBackBufferThunk);
    InUnderlyingRenderer->OnPostResizeWindowBackBuffer().AddRaw(this, &FFXFrameInterpolationSlateRenderer::OnPostResizeWindowBackBufferThunk);
    InUnderlyingRenderer->OnBackBufferReadyToPresent().AddRaw(this, &FFXFrameInterpolationSlateRenderer::OnBackBufferReadyToPresentThunk);
}
FFXFrameInterpolationSlateRenderer::~FFXFrameInterpolationSlateRenderer()
{

}

/** Returns a draw buffer that can be used by Slate windows to draw window elements */
FSlateDrawBuffer& FFXFrameInterpolationSlateRenderer::AcquireDrawBuffer()
{
    FreeBufferIndex = (FreeBufferIndex + 1) % NumDrawBuffers;

    FSlateDrawBuffer* Buffer = &DrawBuffers[FreeBufferIndex];

    while (!Buffer->Lock())
    {
        // If the buffer cannot be locked then the buffer is still in use.  If we are here all buffers are in use
        // so wait until one is free.
        if (IsInSlateThread())
        {
            // We can't flush commands on the slate thread, so simply spinlock until we're done
            // this happens if the render thread becomes completely blocked by expensive tasks when the Slate thread is running
            // in this case we cannot tick Slate.
            FPlatformProcess::Sleep(0.001f);
        }
        else
        {
            FlushCommands();
            UE_LOG(LogSlate, Warning, TEXT("Slate: Had to block on waiting for a draw buffer"));
            FreeBufferIndex = (FreeBufferIndex + 1) % NumDrawBuffers;
        }


        Buffer = &DrawBuffers[FreeBufferIndex];
    }

    // Safely remove brushes by emptying the array and releasing references
    DynamicBrushesToRemove[FreeBufferIndex].Empty();

    Buffer->ClearBuffer();
    Buffer->UpdateResourceVersion(ResourceVersion);
    return *Buffer;
}

void FFXFrameInterpolationSlateRenderer::ReleaseDrawBuffer(FSlateDrawBuffer& InWindowDrawBuffer)
{
#if DO_CHECK
    bool bFound = false;
    for (int32 Index = 0; Index < NumDrawBuffers; ++Index)
    {
        if (&DrawBuffers[Index] == &InWindowDrawBuffer)
        {
            bFound = true;
            break;
        }
    }
    ensureMsgf(bFound, TEXT("It release a DrawBuffer that is not a member of the SlateRHIRenderer"));
#endif

    FSlateDrawBuffer* DrawBuffer = &InWindowDrawBuffer;
    ENQUEUE_RENDER_COMMAND(SlateReleaseDrawBufferCommand)(
        [DrawBuffer](FRHICommandListImmediate& RHICmdList)
        {
            FFISlateReleaseDrawBufferCommand::ReleaseDrawBuffer(RHICmdList, DrawBuffer);
        }
    );
}

bool FFXFrameInterpolationSlateRenderer::Initialize()
{
    // Already initialized
    return true;
}
void FFXFrameInterpolationSlateRenderer::Destroy()
{
    UnderlyingRenderer->Destroy();
}
void FFXFrameInterpolationSlateRenderer::CreateViewport(const TSharedRef<SWindow> InWindow)
{
    UnderlyingRenderer->CreateViewport(InWindow);
}
void FFXFrameInterpolationSlateRenderer::RequestResize(const TSharedPtr<SWindow>& InWindow, uint32 NewSizeX, uint32 NewSizeY)
{
    UnderlyingRenderer->RequestResize(InWindow, NewSizeX, NewSizeY);
}
void FFXFrameInterpolationSlateRenderer::UpdateFullscreenState(const TSharedRef<SWindow> InWindow, uint32 OverrideResX, uint32 OverrideResY)
{
    UnderlyingRenderer->UpdateFullscreenState(InWindow, OverrideResX, OverrideResY);
}
void FFXFrameInterpolationSlateRenderer::SetSystemResolution(uint32 Width, uint32 Height)
{
    UnderlyingRenderer->SetSystemResolution(Width, Height);
}
void FFXFrameInterpolationSlateRenderer::RestoreSystemResolution(const TSharedRef<SWindow> InWindow)
{
    UnderlyingRenderer->RestoreSystemResolution(InWindow);
}
void FFXFrameInterpolationSlateRenderer::DrawWindows(FSlateDrawBuffer& InWindowDrawBuffer)
{
    UnderlyingRenderer->DrawWindows(InWindowDrawBuffer);
}
void FFXFrameInterpolationSlateRenderer::SetColorVisionDeficiencyType(EColorVisionDeficiency Type, int32 Severity, bool bCorrectDeficiency, bool bShowCorrectionWithDeficiency)
{
    UnderlyingRenderer->SetColorVisionDeficiencyType(Type, Severity, bCorrectDeficiency, bShowCorrectionWithDeficiency);
}
FIntPoint FFXFrameInterpolationSlateRenderer::GenerateDynamicImageResource(const FName InTextureName)
{
    return UnderlyingRenderer->GenerateDynamicImageResource(InTextureName);
}
bool FFXFrameInterpolationSlateRenderer::GenerateDynamicImageResource(FName ResourceName, uint32 Width, uint32 Height, const TArray< uint8 >& Bytes)
{
    return UnderlyingRenderer->GenerateDynamicImageResource(ResourceName, Width, Height, Bytes);
}

bool FFXFrameInterpolationSlateRenderer::GenerateDynamicImageResource(FName ResourceName, FSlateTextureDataRef TextureData)
{
    return UnderlyingRenderer->GenerateDynamicImageResource(ResourceName, TextureData);
}

FSlateResourceHandle FFXFrameInterpolationSlateRenderer::GetResourceHandle(const FSlateBrush& Brush, FVector2f LocalSize, float DrawScale)
{
    return UnderlyingRenderer->GetResourceHandle(Brush, LocalSize, DrawScale);
}

FSlateResourceHandle FFXFrameInterpolationSlateRenderer::GetResourceHandle(const FSlateBrush& Brush)
{
    return UnderlyingRenderer->GetResourceHandle(Brush);
}

bool FFXFrameInterpolationSlateRenderer::CanRenderResource(UObject& InResourceObject) const
{
    return UnderlyingRenderer->CanRenderResource(InResourceObject);
}

void FFXFrameInterpolationSlateRenderer::RemoveDynamicBrushResource(TSharedPtr<FSlateDynamicImageBrush> BrushToRemove)
{
    if (BrushToRemove.IsValid())
    {
        DynamicBrushesToRemove[FreeBufferIndex].Add(BrushToRemove);
    }
}

void FFXFrameInterpolationSlateRenderer::ReleaseDynamicResource(const FSlateBrush& Brush)
{
    return UnderlyingRenderer->ReleaseDynamicResource(Brush);
}

void FFXFrameInterpolationSlateRenderer::OnWindowDestroyed(const TSharedRef<SWindow>& InWindow)
{
    return UnderlyingRenderer->OnWindowDestroyed(InWindow);
}

void FFXFrameInterpolationSlateRenderer::OnWindowFinishReshaped(const TSharedPtr<SWindow>& InWindow)
{
    return UnderlyingRenderer->OnWindowFinishReshaped(InWindow);
}

void* FFXFrameInterpolationSlateRenderer::GetViewportResource(const SWindow& Window)
{
    return UnderlyingRenderer->GetViewportResource(Window);
}

void FFXFrameInterpolationSlateRenderer::FlushCommands() const
{
    return UnderlyingRenderer->FlushCommands();
}

void FFXFrameInterpolationSlateRenderer::Sync() const
{
    return UnderlyingRenderer->Sync();
}

void FFXFrameInterpolationSlateRenderer::BeginFrame() const
{
    return UnderlyingRenderer->BeginFrame();
}

void FFXFrameInterpolationSlateRenderer::EndFrame() const
{
    return UnderlyingRenderer->EndFrame();
}

void FFXFrameInterpolationSlateRenderer::ReloadTextureResources()
{
    return UnderlyingRenderer->ReloadTextureResources();
}

void FFXFrameInterpolationSlateRenderer::LoadStyleResources(const ISlateStyle& Style)
{
    return UnderlyingRenderer->LoadStyleResources(Style);
}

bool FFXFrameInterpolationSlateRenderer::AreShadersInitialized() const
{
    return UnderlyingRenderer->AreShadersInitialized();
}

void FFXFrameInterpolationSlateRenderer::InvalidateAllViewports()
{
    return UnderlyingRenderer->InvalidateAllViewports();
}

void FFXFrameInterpolationSlateRenderer::ReleaseAccessedResources(bool bImmediatelyFlush)
{
    UnderlyingRenderer->ReleaseAccessedResources(bImmediatelyFlush);

    if (bImmediatelyFlush)
    {
        // Increment resource version to allow buffers to shrink or cached structures to clean up.
        ResourceVersion++;
    }
}

void FFXFrameInterpolationSlateRenderer::PrepareToTakeScreenshot(const FIntRect& Rect, TArray<FColor>* OutColorData, SWindow* InScreenshotWindow)
{
    return UnderlyingRenderer->PrepareToTakeScreenshot(Rect, OutColorData, InScreenshotWindow);
}

void FFXFrameInterpolationSlateRenderer::SetWindowRenderTarget(const SWindow& Window, class IViewportRenderTargetProvider* Provider)
{
    return UnderlyingRenderer->SetWindowRenderTarget(Window, Provider);
}

FSlateUpdatableTexture* FFXFrameInterpolationSlateRenderer::CreateUpdatableTexture(uint32 Width, uint32 Height)
{
    return UnderlyingRenderer->CreateUpdatableTexture(Width, Height);
}

FSlateUpdatableTexture* FFXFrameInterpolationSlateRenderer::CreateSharedHandleTexture(void* SharedHandle)
{
    return UnderlyingRenderer->CreateSharedHandleTexture(SharedHandle);
}

void FFXFrameInterpolationSlateRenderer::ReleaseUpdatableTexture(FSlateUpdatableTexture* Texture)
{
    return UnderlyingRenderer->ReleaseUpdatableTexture(Texture);
}

ISlateAtlasProvider* FFXFrameInterpolationSlateRenderer::GetTextureAtlasProvider()
{
    return UnderlyingRenderer->GetTextureAtlasProvider();
}

ISlateAtlasProvider* FFXFrameInterpolationSlateRenderer::GetFontAtlasProvider()
{
    return UnderlyingRenderer->GetFontAtlasProvider();
}

void FFXFrameInterpolationSlateRenderer::CopyWindowsToVirtualScreenBuffer(const TArray<FString>& KeypressBuffer)
{
    return UnderlyingRenderer->CopyWindowsToVirtualScreenBuffer(KeypressBuffer);
}

void FFXFrameInterpolationSlateRenderer::MapVirtualScreenBuffer(FMappedTextureBuffer* OutImageData)
{
    return UnderlyingRenderer->MapVirtualScreenBuffer(OutImageData);
}
void FFXFrameInterpolationSlateRenderer::UnmapVirtualScreenBuffer()
{
    return UnderlyingRenderer->UnmapVirtualScreenBuffer();
}

FCriticalSection* FFXFrameInterpolationSlateRenderer::GetResourceCriticalSection()
{
    return UnderlyingRenderer->GetResourceCriticalSection();
}

int32 FFXFrameInterpolationSlateRenderer::RegisterCurrentScene(FSceneInterface* Scene)
{
    return UnderlyingRenderer->RegisterCurrentScene(Scene);
}

int32 FFXFrameInterpolationSlateRenderer::GetCurrentSceneIndex() const
{
    return UnderlyingRenderer->GetCurrentSceneIndex();
}

void FFXFrameInterpolationSlateRenderer::ClearScenes()
{
    return UnderlyingRenderer->ClearScenes();
}

void FFXFrameInterpolationSlateRenderer::DestroyCachedFastPathRenderingData(struct FSlateCachedFastPathRenderingData* VertexData)
{
    return UnderlyingRenderer->DestroyCachedFastPathRenderingData(VertexData);
}
void FFXFrameInterpolationSlateRenderer::DestroyCachedFastPathElementData(struct FSlateCachedElementData* ElementData)
{
    return UnderlyingRenderer->DestroyCachedFastPathElementData(ElementData);
}

bool FFXFrameInterpolationSlateRenderer::HasLostDevice() const
{
    return UnderlyingRenderer->HasLostDevice();
}

void FFXFrameInterpolationSlateRenderer::AddWidgetRendererUpdate(const struct FRenderThreadUpdateContext& Context, bool bDeferredRenderTargetUpdate)
{
    return UnderlyingRenderer->AddWidgetRendererUpdate(Context, bDeferredRenderTargetUpdate);
}

EPixelFormat FFXFrameInterpolationSlateRenderer::GetSlateRecommendedColorFormat()
{
    return UnderlyingRenderer->GetSlateRecommendedColorFormat();
}
