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

#include "FFXFrameInterpolationCustomPresent.h"
#include "RenderTargetPool.h"
#include "FFXFSR3Settings.h"
#include "GlobalShader.h"
#include "ShaderCompilerCore.h"
#include "PipelineStateCache.h"
#if UE_VERSION_AT_LEAST(5, 0, 0)
#include "ShaderCompilerCore.h"
#else
#include "ShaderParameterUtils.h"
#endif

#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "DataDrivenShaderPlatformInfo.h"
#else
#include "RHIDefinitions.h"
#endif

#if UE_VERSION_OLDER_THAN(5, 0, 0)
typedef FIntPoint FUintVector2;
#endif

//------------------------------------------------------------------------------------------------------
// Unreal shader to copy additional UI that only renders on the first invocation of Slate such as debug UI.
//------------------------------------------------------------------------------------------------------
class FFXFIAdditionalUICS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFXFIAdditionalUICS, Global);
public:
	static const int ThreadgroupSizeX = 8;
	static const int ThreadgroupSizeY = 8;
	static const int ThreadgroupSizeZ = 1;

	FFXFIAdditionalUICS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		FirstFrame.Bind(Initializer.ParameterMap, TEXT("FirstFrame"));
		FirstFrameWithUI.Bind(Initializer.ParameterMap, TEXT("FirstFrameWithUI"));
		SecondFrame.Bind(Initializer.ParameterMap, TEXT("SecondFrame"));
		SecondFrameWithUI.Bind(Initializer.ParameterMap, TEXT("SecondFrameWithUI"));
		ViewSize.Bind(Initializer.ParameterMap, TEXT("ViewSize"));
		ViewMin.Bind(Initializer.ParameterMap, TEXT("ViewMin"));
	}
	FFXFIAdditionalUICS() {}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
#if UE_VERSION_AT_LEAST(5, 0, 0)	
		OutEnvironment.SetDefine(TEXT("UNREAL_VERSION"), 5);
#else
		OutEnvironment.SetDefine(TEXT("UNREAL_VERSION"), 4);
#endif
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), ThreadgroupSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), ThreadgroupSizeY);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEZ"), ThreadgroupSizeZ);
		OutEnvironment.SetDefine(TEXT("COMPUTE_SHADER"), 1);
		OutEnvironment.SetDefine(TEXT("UNREAL_ENGINE_MAJOR_VERSION"), ENGINE_MAJOR_VERSION);
	}

	void SetParameters(FRHICommandList& RHICmdList, FUintVector2 InViewSize, FUintVector2 InViewMin, FRHITexture* InFirstFrame, FRHITexture* InFirstFrameWithUI, FRHITexture* InSecondFrame, FRHIUnorderedAccessView* InSecondFrameWithUI)
	{
#if UE_VERSION_AT_LEAST(5, 3, 0)
		FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
		SetShaderValue(BatchedParameters, ViewSize, InViewSize, 0);
		SetShaderValue(BatchedParameters, ViewMin, InViewMin, 0);
		SetTextureParameter(BatchedParameters, FirstFrame, InFirstFrame);
		SetTextureParameter(BatchedParameters, FirstFrameWithUI, InFirstFrameWithUI);
		SetTextureParameter(BatchedParameters, SecondFrame, InSecondFrame);
		SetUAVParameter(BatchedParameters, SecondFrameWithUI, InSecondFrameWithUI);
		RHICmdList.SetBatchedShaderParameters(RHICmdList.GetBoundComputeShader(), BatchedParameters);
#else
		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), ViewSize, InViewSize);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), ViewMin, InViewMin);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundComputeShader(), FirstFrame, InFirstFrame);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundComputeShader(), FirstFrameWithUI, InFirstFrameWithUI);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundComputeShader(), SecondFrame, InSecondFrame);
		SetUAVParameter(RHICmdList, RHICmdList.GetBoundComputeShader(), SecondFrameWithUI, InSecondFrameWithUI);
#endif
	}

	static const TCHAR* GetSourceFilename()
	{
		return TEXT("/Plugin/FSR3/Private/PostProcessFFX_FIAdditionalUI.usf");
	}

	static const TCHAR* GetFunctionName()
	{
		return TEXT("MainCS");
	}

private:
	LAYOUT_FIELD(FShaderResourceParameter, FirstFrame);
	LAYOUT_FIELD(FShaderResourceParameter, FirstFrameWithUI);
	LAYOUT_FIELD(FShaderResourceParameter, SecondFrame);
	LAYOUT_FIELD(FShaderResourceParameter, SecondFrameWithUI);
	LAYOUT_FIELD(FShaderParameter, ViewSize);
	LAYOUT_FIELD(FShaderParameter, ViewMin);
};
IMPLEMENT_SHADER_TYPE(, FFXFIAdditionalUICS, TEXT("/Plugin/FSR3/Private/PostProcessFFX_FIAdditionalUI.usf"), TEXT("MainCS"), SF_Compute);

//------------------------------------------------------------------------------------------------------
// Implementation for FFXFrameInterpolationResources
//------------------------------------------------------------------------------------------------------
FFXFrameInterpolationResources::FFXFrameInterpolationResources(IFFXSharedBackend* InBackend, uint32 InUniqueID)
: FRHIResource(RRT_None)
, UniqueID(InUniqueID)
, Context(nullptr)
, Backend(InBackend)
, bDebugView(false)
{
    FMemory::Memzero(Desc);
}

FFXFrameInterpolationResources::~FFXFrameInterpolationResources()
{
	Backend->ffxDestroyContext(&Context);
}

//------------------------------------------------------------------------------------------------------
// Implementation for FFXFrameInterpolationCustomPresent
//------------------------------------------------------------------------------------------------------
FFXFIResourceRef FFXFrameInterpolationCustomPresent::UpdateContexts(FRDGBuilder& GraphBuilder, uint32 UniqueID, ffxDispatchDescFrameGenerationPrepare const& FsrDesc, ffxCreateContextDescFrameGeneration const& FgDesc)
{
	bool bResourcesValid = false;
	FFXFIResourceRef Resource;

	if (!bResized)
	{
		for (auto& Existing : OldResources)
		{
			bResourcesValid = Existing->UniqueID == UniqueID;
			if (bResourcesValid)
			{
				Resource = Existing;
				break;
			}
		}

		if (bResourcesValid)
		{
			bResourcesValid &= Resource->Desc.displaySize.width == FgDesc.displaySize.width;
			bResourcesValid &= Resource->Desc.displaySize.height == FgDesc.displaySize.height;
			bResourcesValid &= Resource->Desc.maxRenderSize.width == FgDesc.maxRenderSize.width;
			bResourcesValid &= Resource->Desc.maxRenderSize.height == FgDesc.maxRenderSize.height;
			bResourcesValid &= Resource->Desc.backBufferFormat == FgDesc.backBufferFormat;
			bResourcesValid &= Resource->Desc.flags == FgDesc.flags;
		}
	}
	else
	{
		bResized = false;
	}

	if (!bResourcesValid)
	{
		Resource = new FFXFrameInterpolationResources(Backend, UniqueID);
		Resource->Desc = FgDesc;

		auto Code = Backend->ffxCreateContext(&Resource->Context, &Resource->Desc.header);
		if (Code != FFX_API_RETURN_OK)
		{
			Resource.SafeRelease();
		}
	}
	CurrentResource = Resource;
	if (CurrentResource.IsValid())
	{
		Resources.Add(CurrentResource);
	}
	check(CurrentResource.IsValid() && Resources.Num() > 0);
	return Resource;
}

FFXFrameInterpolationCustomPresent::FFXFrameInterpolationCustomPresent()
: Backend(nullptr)
, Viewport(nullptr)
, RHIViewport(nullptr)
, Status(FFXFrameInterpolationCustomPresentStatus::PresentRT)
, Mode(EFFXFrameInterpolationPresentModeRHI)
, Api(EFFXBackendAPI::Unknown)
, bNeedsNativePresentRT(false)
, bPresentRHI(false)
, bHasValidInterpolatedRT(false)
, bEnabled(false)
, bResized(false)
, bUseFFXSwapchain(false)
, bHasInterpolatedRT(false)
, bHasInterpolatedRHI(false)
{
	FMemory::Memzero(Desc);
}

FFXFrameInterpolationCustomPresent::~FFXFrameInterpolationCustomPresent()
{
}

void FFXFrameInterpolationCustomPresent::InitViewport(FViewport* InViewport, FViewportRHIRef ViewportRHI)
{
	Viewport = InViewport;
    RHIViewport = ViewportRHI;
	RHIViewport->SetCustomPresent(this);
}

bool FFXFrameInterpolationCustomPresent::InitSwapChain(IFFXSharedBackend* InBackend, uint32_t Flags, FIntPoint RenderSize, FIntPoint DisplaySize, FfxSwapchain RawSwapChain, FfxCommandQueue Queue, FfxApiSurfaceFormat Format, EFFXBackendAPI InApi)
{
	Api = InApi;

    FfxErrorCode Result = FFX_OK;
    if (Backend != InBackend || Desc.flags != Flags || Desc.maxRenderSize.width != RenderSize.X || Desc.maxRenderSize.height != RenderSize.Y || Desc.displaySize.width != DisplaySize.X || Desc.displaySize.height != DisplaySize.Y || Format != Desc.backBufferFormat)
    {
		Desc.flags = Flags;
		Desc.maxRenderSize.width = RenderSize.X;
		Desc.maxRenderSize.height = RenderSize.Y;
		Desc.displaySize.width = DisplaySize.X;
		Desc.displaySize.height = DisplaySize.Y;
		Desc.backBufferFormat = Format;

		Backend = InBackend;
    }

    return (Result == FFX_OK);
}

// Called when viewport is resized.
void FFXFrameInterpolationCustomPresent::OnBackBufferResize()
{
	bResized = true;

	ENQUEUE_RENDER_COMMAND(FFXFrameInterpolationCustomPresentOnBackBufferResize)(
	[this](FRHICommandListImmediate& RHICmdList)
	{
		RHICmdList.EnqueueLambda([this](FRHICommandListImmediate& cmd) mutable
		{
			ffxConfigureDescFrameGeneration ConfigDesc;
			FMemory::Memzero(ConfigDesc);
			ConfigDesc.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
			ConfigDesc.swapChain = Backend->GetSwapchain(RHIViewport->GetNativeSwapChain());
			ConfigDesc.frameGenerationEnabled = false;
			ConfigDesc.allowAsyncWorkloads = false;

			Backend->UpdateSwapChain(GetContext(), ConfigDesc);
		});
	});

	// Flush the outstanding GPU work and wait for it to complete.
	FlushRenderingCommands();
#if UE_VERSION_OLDER_THAN(5, 5, 0)
	FRHICommandListExecutor::CheckNoOutstandingCmdLists();
#endif
}

// Called from render thread to see if a native present will be requested for this frame.
// @return	true if native Present will be requested for this frame; false otherwise.  Must
// match value subsequently returned by Present for this frame.
bool FFXFrameInterpolationCustomPresent::NeedsNativePresent()
{
	if (Status == FFXFrameInterpolationCustomPresentStatus::PresentRT && (!bUseFFXSwapchain || bNeedsNativePresentRT))
	{
		if (!bHasInterpolatedRT)
		{
			SetEnabled(false);
		}
		bHasInterpolatedRT = false;
	}

	return bUseFFXSwapchain ? bNeedsNativePresentRT : true;
}
// In come cases we want to use custom present but still let the native environment handle 
// advancement of the backbuffer indices.
// @return true if backbuffer index should advance independently from CustomPresent.
bool FFXFrameInterpolationCustomPresent::NeedsAdvanceBackbuffer()
{
	return false;
}

// Called from RHI thread when the engine begins drawing to the viewport.
void FFXFrameInterpolationCustomPresent::BeginDrawing()
{
}

// Called from RHI thread to perform custom present.
// @param InOutSyncInterval - in out param, indicates if vsync is on (>0) or off (==0).
// @return	true if native Present should be also be performed; false otherwise. If it returns
// true, then InOutSyncInterval could be modified to switch between VSync/NoVSync for the normal 
// Present.  Must match value previously returned by NeedsNativePresent for this frame.
bool FFXFrameInterpolationCustomPresent::Present(int32& InOutSyncInterval)
{
	bool bDrawDebugView = false;
#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT || UE_BUILD_TEST)
	bDrawDebugView = CVarFFXFIShowDebugView.GetValueOnAnyThread() != 0;
#endif

	if (bPresentRHI)
	{
		if (!bHasInterpolatedRHI && GetContext())
		{
			ffxConfigureDescFrameGeneration ConfigDesc;
			FMemory::Memzero(ConfigDesc);
			ConfigDesc.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
			ConfigDesc.swapChain = GetBackend()->GetSwapchain(RHIViewport->GetNativeSwapChain());
			ConfigDesc.frameGenerationEnabled = false;
			ConfigDesc.allowAsyncWorkloads = false;
			GetBackend()->UpdateSwapChain(GetContext(), ConfigDesc);
		}
		bHasInterpolatedRHI = false;
	}

	if (bUseFFXSwapchain && !bPresentRHI && !bDrawDebugView && Current.Interpolated.GetReference())
	{
		FfxSwapchain SwapChain = GetBackend()->GetSwapchain(RHIViewport->GetNativeSwapChain());
		FfxApiResource OutputRes = GetBackend()->GetInterpolationOutput(SwapChain);
#if UE_VERSION_AT_LEAST(5, 0, 0)
		FfxApiResource Interpolated = GetBackend()->GetNativeResource(Current.Interpolated->GetRHI(), CVarFFXFICaptureDebugUI.GetValueOnAnyThread() ? FFX_API_RESOURCE_STATE_COMPUTE_READ : FFX_API_RESOURCE_STATE_COPY_DEST);
#else
		FfxApiResource Interpolated = GetBackend()->GetNativeResource(Current.Interpolated->GetRHI(ERenderTargetTexture::Targetable), CVarFFXFICaptureDebugUI.GetValueOnAnyThread() ? FFX_API_RESOURCE_STATE_COMPUTE_READ : FFX_API_RESOURCE_STATE_COPY_DEST);
#endif
		FfxCommandList CmdList = GetBackend()->GetInterpolationCommandList(SwapChain);
		FIntPoint Size = FIntPoint(OutputRes.description.width, OutputRes.description.height);
		if (CmdList)
		{
			GetBackend()->CopySubRect(CmdList, Interpolated, OutputRes, Size, FIntPoint(0, 0));
		}
	}

	int32 PaceRHIFrames = CVarFSR3PaceRHIFrames.GetValueOnAnyThread();
	if (!bUseFFXSwapchain && (Api == EFFXBackendAPI::Unreal) && (PaceRHIFrames != 0) && bPresentRHI && !bDrawDebugView && Current.Interpolated.GetReference())
	{
		InOutSyncInterval = 1;
	}

	return (!bUseFFXSwapchain || bDrawDebugView || bPresentRHI);
}

// Called from RHI thread after native Present has been called
void FFXFrameInterpolationCustomPresent::PostPresent()
{
}

// Called when rendering thread is acquired
void FFXFrameInterpolationCustomPresent::OnAcquireThreadOwnership()
{
}

// Called when rendering thread is released
void FFXFrameInterpolationCustomPresent::OnReleaseThreadOwnership()
{
}

void FFXFrameInterpolationCustomPresent::CopyBackBufferRT(FTextureRHIRef InBackBuffer)
{
    if (Enabled() && (Status == FFXFrameInterpolationCustomPresentStatus::InterpolateRT || Status == FFXFrameInterpolationCustomPresentStatus::PresentRT))
    {
        FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
        
        FRHICopyTextureInfo Info;
        Info.Size.X = InBackBuffer->GetSizeX();
        Info.Size.Y = InBackBuffer->GetSizeY();
    
#if UE_VERSION_AT_LEAST(5, 0, 0)
		ETextureCreateFlags DescFlags = TexCreate_UAV;
#else
		ETextureCreateFlags DescFlags = TexCreate_None;
#endif
        FPooledRenderTargetDesc RTDesc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(Info.Size.X, Info.Size.Y),
            InBackBuffer->GetFormat(),
            FClearValueBinding::Transparent,
			DescFlags,
			TexCreate_UAV|TexCreate_ShaderResource,
            false,
            1,
            true,
            true));

        switch(Status)
        {
            case FFXFrameInterpolationCustomPresentStatus::InterpolateRT:
            {
				check(Mode == EFFXFrameInterpolationPresentModeRHI);
				auto& Dest = Current.Interpolated;
                GRenderTargetPool.FindFreeElement(RHICmdList, RTDesc, Dest, TEXT("Interpolated"));
				check(FIntPoint(InBackBuffer->GetSizeXYZ().X, InBackBuffer->GetSizeXYZ().Y) == Dest->GetDesc().Extent);
#if UE_VERSION_AT_LEAST(5, 0, 0)				
				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::CopySrc),
					FRHITransitionInfo(Dest->GetRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest)
					});

                RHICmdList.CopyTexture(InBackBuffer, Dest->GetRHI(), Info);

				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::Present),
					FRHITransitionInfo(Dest->GetRHI(), ERHIAccess::Unknown, ERHIAccess::SRVCompute),
					});
#else
				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::CopySrc),
					FRHITransitionInfo(Dest->GetRHI(ERenderTargetTexture::Targetable), ERHIAccess::Unknown, ERHIAccess::CopyDest)
					});

                RHICmdList.CopyTexture(InBackBuffer, Dest->GetRHI(ERenderTargetTexture::Targetable), Info);

				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::Present),
					FRHITransitionInfo(Dest->GetRHI(ERenderTargetTexture::Targetable), ERHIAccess::Unknown, ERHIAccess::SRVCompute),
					});
#endif
				bHasValidInterpolatedRT = true;
                break;
            }
            case FFXFrameInterpolationCustomPresentStatus::PresentRT:
            {


#if UE_VERSION_AT_LEAST(5, 5, 0)
				SCOPED_DRAW_EVENT(RHICmdList, FFXFrameInterpolationCustomPresent::CopyBackBufferRT PresentRT)
#else
				RHICmdList.PushEvent(TEXT("FFXFrameInterpolationCustomPresent::CopyBackBufferRT PresentRT"), FColor::White);
#endif

				auto& SecondFrameUI = Current.RealFrame;
				GRenderTargetPool.FindFreeElement(RHICmdList, RTDesc, SecondFrameUI, TEXT("RealFrame"));
#if UE_VERSION_AT_LEAST(5, 0, 0)
				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::CopySrc),
					FRHITransitionInfo(SecondFrameUI->GetRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest)
					});

				check(FIntPoint(InBackBuffer->GetSizeXYZ().X, InBackBuffer->GetSizeXYZ().Y) == SecondFrameUI->GetDesc().Extent);
				RHICmdList.CopyTexture(InBackBuffer, SecondFrameUI->GetRHI(), Info);
#else
				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::CopySrc),
					FRHITransitionInfo(SecondFrameUI->GetRHI(ERenderTargetTexture::Targetable), ERHIAccess::Unknown, ERHIAccess::CopyDest)
					});

				check(FIntPoint(InBackBuffer->GetSizeXYZ().X, InBackBuffer->GetSizeXYZ().Y) == SecondFrameUI->GetDesc().Extent);
				RHICmdList.CopyTexture(InBackBuffer, SecondFrameUI->GetRHI(ERenderTargetTexture::Targetable), Info);
#endif
				if (CVarFFXFICaptureDebugUI.GetValueOnAnyThread() && bHasValidInterpolatedRT && (Mode == EFFXFrameInterpolationPresentModeRHI))
				{
					auto& FirstFrame = InterpolatedNoUI;
					auto& SecondFrame = RealFrameNoUI;
					auto& FirstFrameUI = Current.Interpolated;
#if UE_VERSION_AT_LEAST(5, 3, 0)
					auto RWSecondFrameUI = FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(SecondFrameUI->GetRHI());
#elif UE_VERSION_AT_LEAST(5, 0, 0)
					auto RWSecondFrameUI = RHICreateUnorderedAccessView(SecondFrameUI->GetRHI());
#else
					auto RWSecondFrameUI = RHICreateUnorderedAccessView(SecondFrameUI->GetRHI(ERenderTargetTexture::Targetable));
#endif

					TShaderRef<FFXFIAdditionalUICS> ComputeShader = TShaderMapRef<FFXFIAdditionalUICS>(GetGlobalShaderMap(GMaxRHIFeatureLevel));

					RHICmdList.Transition({
						FRHITransitionInfo(RWSecondFrameUI, ERHIAccess::Unknown, ERHIAccess::UAVCompute),
						});

					FIntPoint Extent(InBackBuffer->GetSizeXYZ().X, InBackBuffer->GetSizeXYZ().Y);
					SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
#if UE_VERSION_AT_LEAST(5, 0, 0)
					ComputeShader->SetParameters(RHICmdList, FUintVector2(Extent.X, Extent.Y), FUintVector2(0, 0), FirstFrame->GetRHI(), FirstFrameUI->GetRHI(), SecondFrame->GetRHI(), RWSecondFrameUI);

					RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp(Extent.X, FFXFIAdditionalUICS::ThreadgroupSizeX), FMath::DivideAndRoundUp(Extent.Y, FFXFIAdditionalUICS::ThreadgroupSizeY), 1);

					RHICmdList.Transition({
						FRHITransitionInfo(SecondFrameUI->GetRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc),
						FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::CopyDest)
						});

					check(SecondFrameUI->GetDesc().Extent == FIntPoint(InBackBuffer->GetSizeXYZ().X, InBackBuffer->GetSizeXYZ().Y));

					RHICmdList.CopyTexture(SecondFrameUI->GetRHI(), InBackBuffer, Info);
#else
					ComputeShader->SetParameters(RHICmdList, FIntPoint(Extent.X, Extent.Y), FIntPoint(0, 0), FirstFrame->GetRHI(ERenderTargetTexture::Targetable), FirstFrameUI->GetRHI(ERenderTargetTexture::Targetable), SecondFrame->GetRHI(ERenderTargetTexture::Targetable), RWSecondFrameUI);

					RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp(Extent.X, FFXFIAdditionalUICS::ThreadgroupSizeX), FMath::DivideAndRoundUp(Extent.Y, FFXFIAdditionalUICS::ThreadgroupSizeY), 1);

					RHICmdList.Transition({
						FRHITransitionInfo(SecondFrameUI->GetRHI(ERenderTargetTexture::Targetable), ERHIAccess::Unknown, ERHIAccess::CopySrc),
						FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::CopyDest)
						});

					check(SecondFrameUI->GetDesc().Extent == FIntPoint(InBackBuffer->GetSizeXYZ().X, InBackBuffer->GetSizeXYZ().Y));

					RHICmdList.CopyTexture(SecondFrameUI->GetRHI(ERenderTargetTexture::Targetable), InBackBuffer, Info);
#endif
				}

				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::Present)
					});

				bHasValidInterpolatedRT = false;

#if UE_VERSION_OLDER_THAN(5, 5, 0)
				RHICmdList.PopEvent();
#endif

                break;
            }
            default:
            {
                break;
            }
        }
    }
}

void FFXFrameInterpolationCustomPresent::SetMode(EFFXFrameInterpolationPresentMode InMode)
{
	Mode = InMode;
}

void FFXFrameInterpolationCustomPresent::SetEnabled(bool const bInEnabled)
{
	bEnabled = bInEnabled;
}

void FFXFrameInterpolationCustomPresent::SetCustomPresentStatus(FFXFrameInterpolationCustomPresentStatus Flag)
{
	switch (Flag)
	{
		case FFXFrameInterpolationCustomPresentStatus::InterpolateRT:
		{
			bHasInterpolatedRT = true;
			if (GetMode() != EFFXFrameInterpolationPresentModeNative)
			{
				Status = Flag;
				bNeedsNativePresentRT = false;
				break;
			}
			// Else is deliberately falling through
		}
		case FFXFrameInterpolationCustomPresentStatus::PresentRT:
		{
            Status = FFXFrameInterpolationCustomPresentStatus::PresentRT;
			bNeedsNativePresentRT = true;
			break;
		}
		case FFXFrameInterpolationCustomPresentStatus::InterpolateRHI:
		{
			bHasInterpolatedRHI = true;
			if (GetMode() != EFFXFrameInterpolationPresentModeNative)
			{
				bPresentRHI = false;
				break;
			}
			// Else is deliberately falling through
		}
		case FFXFrameInterpolationCustomPresentStatus::PresentRHI:
		{
			bPresentRHI = true;
			break;
		}
	}
}

void FFXFrameInterpolationCustomPresent::SetUseFFXSwapchain(bool const bToggle)
{ 
	bUseFFXSwapchain = bToggle;
}
