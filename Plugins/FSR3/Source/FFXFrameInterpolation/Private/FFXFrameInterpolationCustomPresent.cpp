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

#include "FFXFrameInterpolationCustomPresent.h"
#include "RenderTargetPool.h"

#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "DataDrivenShaderPlatformInfo.h"
#else
#include "RHIDefinitions.h"
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
// Static helper functions
//------------------------------------------------------------------------------------------------------
static FfxErrorCode ffxOpticalflowCreateSharedResources(IFFXSharedBackend* Backend, FfxInterface& Interface, FfxOpticalflowContext* context, FfxOpticalflowSharedResources* SharedResources)
{
	FFX_RETURN_ON_ERROR(
		context,
		FFX_ERROR_INVALID_POINTER);
	FFX_RETURN_ON_ERROR(
		SharedResources,
		FFX_ERROR_INVALID_POINTER);

	FfxOpticalflowSharedResourceDescriptions internalSurfaceDesc;
	ffxOpticalflowGetSharedResourceDescriptions(context, &internalSurfaceDesc);

	SharedResources->opticalFlow = Backend->CreateResource(Interface, &internalSurfaceDesc.opticalFlowVector);
	SharedResources->opticalFlowSCD = Backend->CreateResource(Interface, &internalSurfaceDesc.opticalFlowSCD);
	return FFX_OK;
}

//------------------------------------------------------------------------------------------------------
// Implementation for FFXFrameInterpolationResources
//------------------------------------------------------------------------------------------------------
FFXFrameInterpolationResources::FFXFrameInterpolationResources(IFFXSharedBackend* InBackend, uint32 InUniqueID)
: FRHIResource(RRT_None)
, UniqueID(InUniqueID)
, Backend(InBackend)
, bDebugView(false)
{
    FMemory::Memzero(Interface);
    FMemory::Memzero(OpticalFlowContext);
    FMemory::Memzero(OpticalFlowDesc);
    FMemory::Memzero(Desc);
    FMemory::Memzero(Context);
}

FFXFrameInterpolationResources::~FFXFrameInterpolationResources()
{
    Backend->ReleaseResource(Interface, OpticalFlowResources.opticalFlow);
    Backend->ReleaseResource(Interface, OpticalFlowResources.opticalFlowSCD);

    if (OpticalFlowDesc.backendInterface.device)
    {
        ffxOpticalflowContextDestroy(&OpticalFlowContext);
    }
    if (Desc.backendInterface.device)
    {
        ffxFrameInterpolationContextDestroy(&Context);
    }
	if (Interface.scratchBuffer)
	{
		FMemory::Free(Interface.scratchBuffer);
	}
}

//------------------------------------------------------------------------------------------------------
// Implementation for FFXFrameInterpolationCustomPresent
//------------------------------------------------------------------------------------------------------
FFXFIResourceRef FFXFrameInterpolationCustomPresent::UpdateContexts(FRDGBuilder& GraphBuilder, uint32 UniqueID, FfxFsr3UpscalerContextDescription const& FsrDesc, FIntPoint ViewportSizeXY, FfxSurfaceFormat BackBufferFormat)
{
	bool bResourcesValid = false;
	FFXFIResourceRef Resource;

	FfxDimensions2D ViewportSize;
	ViewportSize.width = FMath::Max(FsrDesc.displaySize.width, (uint32)ViewportSizeXY.X);
	ViewportSize.height = FMath::Max(FsrDesc.displaySize.height, (uint32)ViewportSizeXY.Y);

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
			bResourcesValid &= ((Desc.flags & FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED) != 0) == ((FsrDesc.flags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED) != 0);
			bResourcesValid &= ((Desc.flags & FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INFINITE) != 0) == ((FsrDesc.flags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE) != 0);
			bResourcesValid &= ((Desc.flags & FFX_FRAMEINTERPOLATION_ENABLE_TEXTURE1D_USAGE) != 0) == ((FsrDesc.flags & FFX_FSR3UPSCALER_ENABLE_TEXTURE1D_USAGE) != 0);
			bResourcesValid &= Resource->OpticalFlowDesc.resolution.width == ViewportSize.width;
			bResourcesValid &= Resource->OpticalFlowDesc.resolution.height == ViewportSize.height;
			bResourcesValid &= Resource->Desc.displaySize.width == ViewportSize.width;
			bResourcesValid &= Resource->Desc.displaySize.height == ViewportSize.height;
			bResourcesValid &= Resource->Desc.maxRenderSize.width == FsrDesc.maxRenderSize.width;
			bResourcesValid &= Resource->Desc.maxRenderSize.height == FsrDesc.maxRenderSize.height;
			bResourcesValid &= Resource->Desc.backendInterface.device == FsrDesc.backendInterface.device;
			bResourcesValid &= Resource->Desc.backBufferFormat == BackBufferFormat;
		}
	}
	else
	{
		bResized = false;
	}

	if (!bResourcesValid)
	{
		Resource = new FFXFrameInterpolationResources(Backend, UniqueID);
		Backend->CreateInterface(Resource->Interface, 2);

		Resource->OpticalFlowDesc.backendInterface = Resource->Interface;
		Resource->OpticalFlowDesc.flags = 0;
		Resource->OpticalFlowDesc.resolution.width = ViewportSize.width;
		Resource->OpticalFlowDesc.resolution.height = ViewportSize.height;

		FfxErrorCode Code = ffxOpticalflowContextCreate(&Resource->OpticalFlowContext, &Resource->OpticalFlowDesc);
		if (Code == FFX_OK)
		{
			Code = ffxOpticalflowCreateSharedResources(Backend, Resource->Interface, &Resource->OpticalFlowContext, &Resource->OpticalFlowResources);
			if (Code == FFX_OK)
			{
				Desc.backendInterface = Resource->Interface;
				Desc.displaySize.width = ViewportSize.width;
				Desc.displaySize.height = ViewportSize.height;
				Desc.maxRenderSize.width = FsrDesc.maxRenderSize.width;
				Desc.maxRenderSize.height = FsrDesc.maxRenderSize.height;
				Desc.backBufferFormat = BackBufferFormat;
				Desc.flags = 0;
				Desc.flags |= (FsrDesc.flags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED) ? FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED : 0;
				Desc.flags |= (FsrDesc.flags & FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE) ? FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INFINITE : 0;
				Desc.flags |= (FsrDesc.flags & FFX_FSR3UPSCALER_ENABLE_TEXTURE1D_USAGE) ? FFX_FRAMEINTERPOLATION_ENABLE_TEXTURE1D_USAGE : 0;
				FMemory::Memcpy(Resource->Desc, Desc);
				Code = ffxFrameInterpolationContextCreate(&Resource->Context, &Resource->Desc);
				if (Code != FFX_OK)
				{
					Resource.SafeRelease();
				}
			}
		}
		else
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
, bNeedsNativePresentRT(false)
, bPresentRHI(false)
, bHasValidInterpolatedRT(false)
, bEnabled(false)
, bResized(false)
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

bool FFXFrameInterpolationCustomPresent::InitSwapChain(IFFXSharedBackend* InBackend, uint32_t Flags, FIntPoint RenderSize, FIntPoint DisplaySize, FfxSwapchain RawSwapChain, FfxCommandQueue Queue, FfxSurfaceFormat Format, FfxPresentCallbackFunc CompositionFunc)
{
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
			Backend->UpdateSwapChain(Desc.backendInterface, RHIViewport->GetNativeSwapChain(), false, false, false);
		});
	});

	// Flush the outstanding GPU work and wait for it to complete.
	FlushRenderingCommands();
	FRHICommandListExecutor::CheckNoOutstandingCmdLists();
}

// Called from render thread to see if a native present will be requested for this frame.
// @return	true if native Present will be requested for this frame; false otherwise.  Must
// match value subsequently returned by Present for this frame.
bool FFXFrameInterpolationCustomPresent::NeedsNativePresent()
{
	return true;
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
	return true;
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

void FFXFrameInterpolationCustomPresent::CopyBackBufferRT(FTexture2DRHIRef InBackBuffer)
{
    if (Enabled() && (Status == FFXFrameInterpolationCustomPresentStatus::InterpolateRT || Status == FFXFrameInterpolationCustomPresentStatus::PresentRT))
    {
        FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
        
        FRHICopyTextureInfo Info;
        Info.Size.X = InBackBuffer->GetSizeX();
        Info.Size.Y = InBackBuffer->GetSizeY();
    
        FPooledRenderTargetDesc RTDesc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(Info.Size.X, Info.Size.Y),
            InBackBuffer->GetFormat(),
            FClearValueBinding::Transparent,
			TexCreate_UAV,
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
				check(InBackBuffer->GetDesc().Extent == Dest->GetDesc().Extent);
				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::CopySrc),
					FRHITransitionInfo(Dest->GetRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest)
					});

                RHICmdList.CopyTexture(InBackBuffer, Dest->GetRHI(), Info);

				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::Present),
					FRHITransitionInfo(Dest->GetRHI(), ERHIAccess::Unknown, ERHIAccess::SRVCompute),
					});

				bHasValidInterpolatedRT = true;
                break;
            }
            case FFXFrameInterpolationCustomPresentStatus::PresentRT:
            {
				static IConsoleVariable* CVarFFXFICaptureDebugUIRef = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FI.CaptureDebugUI"));

				RHICmdList.PushEvent(TEXT("FFXFrameInterpolationCustomPresent::CopyBackBufferRT PresentRT"), FColor::White);

				auto& SecondFrameUI = Current.RealFrame;
				GRenderTargetPool.FindFreeElement(RHICmdList, RTDesc, SecondFrameUI, TEXT("RealFrame"));

				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::CopySrc),
					FRHITransitionInfo(SecondFrameUI->GetRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest)
					});

				check(InBackBuffer->GetDesc().Extent == SecondFrameUI->GetDesc().Extent);
				RHICmdList.CopyTexture(InBackBuffer, SecondFrameUI->GetRHI(), Info);

				FfxResource NullResource;
				FMemory::Memzero(NullResource);
				GetBackend()->BindUITexture(RHIViewport->GetNativeSwapChain(), NullResource);
				if (CVarFFXFICaptureDebugUIRef && CVarFFXFICaptureDebugUIRef->GetInt() && bHasValidInterpolatedRT && (Mode == EFFXFrameInterpolationPresentModeRHI))
				{
					auto& FirstFrame = InterpolatedNoUI;
					auto& SecondFrame = RealFrameNoUI;
					auto& FirstFrameUI = Current.Interpolated;
#if UE_VERSION_AT_LEAST(5, 3, 0)
					auto RWSecondFrameUI = FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(SecondFrameUI->GetRHI());
#else
					auto RWSecondFrameUI = RHICreateUnorderedAccessView(SecondFrameUI->GetRHI());
#endif

					TShaderRef<FFXFIAdditionalUICS> ComputeShader = TShaderMapRef<FFXFIAdditionalUICS>(GetGlobalShaderMap(GMaxRHIFeatureLevel));

					RHICmdList.Transition({
						FRHITransitionInfo(RWSecondFrameUI, ERHIAccess::Unknown, ERHIAccess::UAVCompute),
						});

					auto Extent = InBackBuffer->GetDesc().Extent;
					SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
					ComputeShader->SetParameters(RHICmdList, FUintVector2(Extent.X, Extent.Y), FUintVector2(0, 0), FirstFrame->GetRHI(), FirstFrameUI->GetRHI(), SecondFrame->GetRHI(), RWSecondFrameUI);

					RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp(Extent.X, FFXFIAdditionalUICS::ThreadgroupSizeX), FMath::DivideAndRoundUp(Extent.Y, FFXFIAdditionalUICS::ThreadgroupSizeY), 1);

					RHICmdList.Transition({
						FRHITransitionInfo(SecondFrameUI->GetRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc),
						FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::CopyDest)
						});

					check(SecondFrameUI->GetDesc().Extent == InBackBuffer->GetDesc().Extent);

					RHICmdList.CopyTexture(SecondFrameUI->GetRHI(), InBackBuffer, Info);
				}

				RHICmdList.Transition({
					FRHITransitionInfo(InBackBuffer, ERHIAccess::Unknown, ERHIAccess::Present)
					});

				bHasValidInterpolatedRT = false;

				RHICmdList.PopEvent();

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
            Status = Flag;
			bNeedsNativePresentRT = false;
			break;
		}
		case FFXFrameInterpolationCustomPresentStatus::InterpolateRHI:
		{
			bPresentRHI = false;
			break;
		}
		case FFXFrameInterpolationCustomPresentStatus::PresentRT:
		{
            Status = Flag;
			bNeedsNativePresentRT = true;
			break;
		}
		case FFXFrameInterpolationCustomPresentStatus::PresentRHI:
		{
			bPresentRHI = true;
			break;
		}
	}
}
