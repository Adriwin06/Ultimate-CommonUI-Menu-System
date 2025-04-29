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

#include "FFXFSR3TemporalUpscalerHistory.h"
#include "FFXFSR3TemporalUpscaler.h"
#include "FFXFSR3TemporalUpscaling.h"

const TCHAR* FFXFSR3TemporalUpscalerHistory::FfxFsr3DebugName = TEXT("FFXFSR3TemporalUpscaler");

TCHAR const* FFXFSR3TemporalUpscalerHistory::GetUpscalerName()
{
	return FfxFsr3DebugName;
}

FFXFSR3TemporalUpscalerHistory::FFXFSR3TemporalUpscalerHistory(FSR3StateRef NewState, FFXFSR3TemporalUpscaler* _Upscaler, TRefCountPtr<IPooledRenderTarget> InMotionVectors)
{
	MotionVectors = InMotionVectors;
	Upscaler = _Upscaler;
	SetState(NewState);
}

FFXFSR3TemporalUpscalerHistory::~FFXFSR3TemporalUpscalerHistory()
{
	if (FFXFSR3TemporalUpscalingModule::IsInitialized())
	{
		Upscaler->ReleaseState(Fsr3);
	}
}

#if UE_VERSION_AT_LEAST(5, 3, 0)
const TCHAR* FFXFSR3TemporalUpscalerHistory::GetDebugName() const {
	// this has to match FFXFSR3TemporalUpscalerHistory::GetDebugName()
	return FfxFsr3DebugName;
}

uint64 FFXFSR3TemporalUpscalerHistory::GetGPUSizeBytes() const {
	// 5.3 not done
	return 0;
}
#endif

void FFXFSR3TemporalUpscalerHistory::SetState(FSR3StateRef NewState)
{
	Upscaler->ReleaseState(Fsr3);
	Fsr3 = NewState;
}

ffxContext* FFXFSR3TemporalUpscalerHistory::GetFSRContext() const
{
	return Fsr3.IsValid() ? &Fsr3->Fsr3 : nullptr;
}

ffxCreateContextDescUpscale* FFXFSR3TemporalUpscalerHistory::GetFSRContextDesc() const
{
	return Fsr3.IsValid() ? &Fsr3->Params : nullptr;
}

TRefCountPtr<IPooledRenderTarget> FFXFSR3TemporalUpscalerHistory::GetMotionVectors() const
{
	return Fsr3.IsValid() ? MotionVectors : nullptr;
}