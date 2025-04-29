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

#include "HAL/Platform.h"
#include "Misc/EngineVersionComparison.h"

#if PLATFORM_WINDOWS
	#define FFX_ENABLE_DX12 1
	#include "Windows/AllowWindowsPlatformTypes.h"
	#pragma warning( push )
	#pragma warning( disable : 4191 )
#else
	#define FFX_ENABLE_DX12 0
	#define FFX_GCC
#endif
	THIRD_PARTY_INCLUDES_START

#if UE_VERSION_OLDER_THAN(5, 0, 0)
	#include <initguid.h>
#endif

#if UE_VERSION_NEWER_THAN(5, 2, 1)
	#include <bit>
#endif
	#include "FidelityFX/host/ffx_types.h"
	#include "FidelityFX/host/ffx_error.h"

#if !defined(FFX_GCC)
	#undef FFX_API
	#define FFX_API __declspec(dllexport)
#endif

	#include "ffx_api_loader.h"
	#include "ffx_api_dx12.h"

	FFX_API FfxErrorCode ffxFrameInterpolationUiComposition(const FfxPresentCallbackDescription* params, void*);

	THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
	#pragma warning( pop )
	#include "Windows/HideWindowsPlatformTypes.h"
#else
	#undef FFX_GCC
#endif
