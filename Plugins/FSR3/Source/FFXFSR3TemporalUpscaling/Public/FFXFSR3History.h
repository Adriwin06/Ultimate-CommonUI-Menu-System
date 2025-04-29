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
#include "SceneRendering.h"
#include "FFXSharedBackend.h"

typedef void* ffxContext;
struct ffxCreateContextDescUpscale;

#if UE_VERSION_AT_LEAST(5, 3, 0)
using IFFXFSR3CustomTemporalAAHistory = UE::Renderer::Private::ITemporalUpscaler::IHistory;
#else
using IFFXFSR3CustomTemporalAAHistory = ICustomTemporalAAHistory;
#endif

class IFFXFSR3History : public IFFXFSR3CustomTemporalAAHistory
{
public:
    virtual ffxContext* GetFSRContext() const = 0;
    virtual ffxCreateContextDescUpscale* GetFSRContextDesc() const = 0;
    virtual TRefCountPtr<IPooledRenderTarget> GetMotionVectors() const = 0;
};
