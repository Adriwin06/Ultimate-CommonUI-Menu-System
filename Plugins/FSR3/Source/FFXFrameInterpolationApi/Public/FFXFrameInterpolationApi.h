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

#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
	#define FFX_ENABLE_DX12 1
	#include "Windows/AllowWindowsPlatformTypes.h"
#else
	#define FFX_ENABLE_DX12 0
	#define FFX_GCC
#endif
	THIRD_PARTY_INCLUDES_START

	#include "FidelityFX/host/ffx_frameinterpolation.h"

	#include "FidelityFX/gpu/frameinterpolation/ffx_frameinterpolation_resources.h"

	THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
	#include "Windows/HideWindowsPlatformTypes.h"
#else
	#undef FFX_GCC
#endif
