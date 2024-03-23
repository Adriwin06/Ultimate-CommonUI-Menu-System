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

#pragma once

#include "CoreMinimal.h"
#include "SceneRendering.h"
#include "FFXFSR3Include.h"
#include "FFXFSR3History.h"

class FFXFSR3TemporalUpscaler;

#define FFX_FSR3UPSCALER_MAX_NUM_BUFFERS 3

//-------------------------------------------------------------------------------------
// The FSR3 state wrapper, deletion is handled by the RHI so that they aren't removed out from under the GPU.
//-------------------------------------------------------------------------------------
struct FFXFSR3State : public FRHIResource
{
	FFXFSR3State(IFFXSharedBackend* InBackend)
	: FRHIResource(RRT_None)
	, Backend(InBackend)
	, Fsr3Resources(nullptr)
	, LastUsedFrame(~0u)
	, Index(0u)
	{
		FMemory::Memzero(Interface);
		FMemory::Memzero(Fsr3ResourceArray);
		Fsr3Resources = &Fsr3ResourceArray[0];
	}
	~FFXFSR3State()
	{
		ReleaseResources();
		ffxFsr3UpscalerContextDestroy(&Fsr3);
		if (Interface.scratchBuffer)
		{
			FMemory::Free(Interface.scratchBuffer);
		}
	}

	FfxErrorCode CreateResources()
	{
		if (!Fsr3ResourceArray[Index].dilatedDepth.Resource.resource || !Fsr3ResourceArray[Index].dilatedMotionVectors.Resource.resource || !Fsr3ResourceArray[Index].reconstructedPrevNearestDepth.Resource.resource)
		{
			FFX_RETURN_ON_ERROR(
				Backend,
				FFX_ERROR_INVALID_POINTER);

			FfxFsr3UpscalerSharedResourceDescriptions internalSurfaceDesc;
			ffxFsr3UpscalerGetSharedResourceDescriptions(&Fsr3, &internalSurfaceDesc);

			Fsr3ResourceArray[Index].dilatedDepth = Backend->CreateResource(Interface, &internalSurfaceDesc.dilatedDepth);
			Fsr3ResourceArray[Index].dilatedMotionVectors = Backend->CreateResource(Interface, &internalSurfaceDesc.dilatedMotionVectors);
			Fsr3ResourceArray[Index].reconstructedPrevNearestDepth = Backend->CreateResource(Interface, &internalSurfaceDesc.reconstructedPrevNearestDepth);
		}
		return FFX_OK;
	}

	void ReleaseResources()
	{
		for (uint32 i = 0; i < FFX_FSR3UPSCALER_MAX_NUM_BUFFERS; i++)
		{
			Backend->ReleaseResource(Interface, Fsr3ResourceArray[i].dilatedDepth);
			Backend->ReleaseResource(Interface, Fsr3ResourceArray[i].dilatedMotionVectors);
			Backend->ReleaseResource(Interface, Fsr3ResourceArray[i].reconstructedPrevNearestDepth);
		}
		Index = 0;
		Fsr3Resources = &Fsr3ResourceArray[0];
	}

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

	IFFXSharedBackend* Backend;
	FfxInterface Interface;
	FfxFsr3UpscalerContextDescription Params;
	FfxFsr3UpscalerContext Fsr3;
	FfxFsr3UpscalerSharedResources* Fsr3Resources;
	FfxFsr3UpscalerSharedResources Fsr3ResourceArray[FFX_FSR3UPSCALER_MAX_NUM_BUFFERS];
	uint64 LastUsedFrame;
	uint32 ViewID;
	uint32 Index;
};
typedef TRefCountPtr<FFXFSR3State> FSR3StateRef;

//-------------------------------------------------------------------------------------
// The ICustomTemporalAAHistory for FSR3, this retains the FSR3 state object.
//-------------------------------------------------------------------------------------
class FFXFSR3TemporalUpscalerHistory final : public IFFXFSR3History, public FRefCountBase
{
public:
	FFXFSR3TemporalUpscalerHistory(FSR3StateRef NewState, FFXFSR3TemporalUpscaler* Upscaler, TRefCountPtr<IPooledRenderTarget> InMotionVectors);

	virtual ~FFXFSR3TemporalUpscalerHistory();

#if UE_VERSION_AT_LEAST(5, 3, 0)
	virtual const TCHAR* GetDebugName() const override;
	virtual uint64 GetGPUSizeBytes() const override;
#endif

	void AdvanceIndex() final;
    FfxFsr3UpscalerSharedResources* GetFSRResources() const final;
    FfxFsr3UpscalerContext* GetFSRContext() const final;
    FfxInterface* GetFFXInterface() const final;
    FfxFsr3UpscalerContextDescription* GetFSRContextDesc() const final;
	TRefCountPtr<IPooledRenderTarget> GetMotionVectors() const final;

	void SetState(FSR3StateRef NewState);

	inline FSR3StateRef const& GetState() const
	{
		return Fsr3;
	}
	
	uint32 AddRef() const final
	{
		return FRefCountBase::AddRef();
	}

	uint32 Release() const final
	{
		return FRefCountBase::Release();
	}

	uint32 GetRefCount() const final
	{
		return FRefCountBase::GetRefCount();
	}

	static TCHAR const* GetUpscalerName();

private:
	static TCHAR const* FfxFsr3DebugName;
	FSR3StateRef Fsr3;
	FFXFSR3TemporalUpscaler* Upscaler;
	TRefCountPtr<IPooledRenderTarget> MotionVectors;
};
