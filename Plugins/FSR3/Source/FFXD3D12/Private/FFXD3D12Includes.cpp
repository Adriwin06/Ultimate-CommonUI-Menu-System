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

#include "FFXD3D12Includes.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#undef InterlockedIncrement
#undef InterlockedDecrement
#define InterlockedIncrement ::_InterlockedIncrement
#define InterlockedDecrement ::_InterlockedDecrement
typedef LONG NTSTATUS;
#include <dwmapi.h>
#pragma warning(push)
#pragma warning(disable:4191)
#else
#define _countof(a) (sizeof(a)/sizeof(*(a)))
#define strcpy_s(a, b) strcpy(a, b)
#define FFX_GCC 1
#endif
THIRD_PARTY_INCLUDES_START

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define FFX_FSR 1
#define FFX_FSR3 1

#ifdef verify
#undef verify
#endif

#if !defined(FFX_BUILD_AS_DLL) || (FFX_BUILD_AS_DLL == 0)
#include "backends/dx12/ffx_dx12.cpp"
#include "backends/shared/blob_accessors/ffx_frameinterpolation_shaderblobs.cpp"
#include "backends/shared/blob_accessors/ffx_fsr1_shaderblobs.cpp"
#include "backends/shared/blob_accessors/ffx_fsr2_shaderblobs.cpp"
#include "backends/shared/blob_accessors/ffx_fsr3upscaler_shaderblobs.cpp"
#include "backends/shared/blob_accessors/ffx_opticalflow_shaderblobs.cpp"
#include "backends/shared/ffx_shader_blobs.cpp"
#include "backends/dx12/FrameInterpolationSwapchain/FrameInterpolationSwapchainDX12.cpp"
#include "backends/dx12/FrameInterpolationSwapchain/FrameInterpolationSwapchainDX12_Helpers.cpp"
#include "backends/dx12/FrameInterpolationSwapchain/FrameInterpolationSwapchainDX12_UiComposition.cpp"
#include "backends/dx12/GPUTimestamps.cpp"
#endif

#undef min
#undef max

#undef FFX_FSR
#undef FFX_FSR3

THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
#pragma warning(pop)
#undef InterlockedIncrement
#undef InterlockedDecrement
#include "Windows/HideWindowsPlatformTypes.h"
#else
#undef _countof
#undef strcpy_s
#undef FFX_GCC
#endif
