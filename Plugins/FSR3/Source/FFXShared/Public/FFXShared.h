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

// Variant of UE_VERSION_NEWER_THAN that is true if the engine version is at or later than the specified, used to better handle version differences in the codebase.
#define UE_VERSION_AT_LEAST(MajorVersion, MinorVersion, PatchVersion)	\
	UE_GREATER_SORT(ENGINE_MAJOR_VERSION, MajorVersion, UE_GREATER_SORT(ENGINE_MINOR_VERSION, MinorVersion, UE_GREATER_SORT(ENGINE_PATCH_VERSION, PatchVersion, true)))

#if PLATFORM_WINDOWS
	#define FFX_ENABLE_DX12 1
	#include "Windows/AllowWindowsPlatformTypes.h"
#else
	#define FFX_ENABLE_DX12 0
	#define FFX_GCC
#endif
	THIRD_PARTY_INCLUDES_START

#if UE_VERSION_AT_LEAST(5, 3, 0)
	#include <bit>
#endif
	#include "ffx_api_types.h"
	#include "ffx_api.h"

	#include "FidelityFX/host/ffx_types.h"

#if !defined(FFX_GCC)
	#undef FFX_API
	#define FFX_API __declspec(dllexport)
#endif

	#include "FidelityFX/host/ffx_assert.h"
	#include "FidelityFX/host/ffx_error.h"
	#include "FidelityFX/host/ffx_interface.h"
	#include "FidelityFX/host/ffx_util.h"

	THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
	#include "Windows/HideWindowsPlatformTypes.h"
#else
	#undef FFX_GCC
#endif

#if defined(FFX_RENDER_TESTS)
	#include "IFFXRenderTest.h"
#else
	#ifndef FFX_RENDER_TEST_CAPTURE_PASS_BEGIN
		#define FFX_RENDER_TEST_CAPTURE_PASS_BEGIN(Name, GraphBuilder, MinDiff)  
	#endif
	#ifndef FFX_RENDER_TEST_CAPTURE_PASS_ADD
		#define FFX_RENDER_TEST_CAPTURE_PASS_ADD(TextureName, GraphBuilder, MinDiff) 
	#endif
	#ifndef FFX_RENDER_TEST_CAPTURE_PASS_PARAM
		#define FFX_RENDER_TEST_CAPTURE_PASS_PARAM(TextureName, Texture, GraphBuilder, MinDiff) 
	#endif
	#ifndef FFX_RENDER_TEST_CAPTURE_PASS_PARAMS
		#define FFX_RENDER_TEST_CAPTURE_PASS_PARAMS(TypeName, Parameters, GraphBuilder, MinDiff) 
	#endif
	#ifndef FFX_RENDER_TEST_CAPTURE_PASS_END
		#define FFX_RENDER_TEST_CAPTURE_PASS_END(GraphBuilder) 
	#endif
	#ifndef FFX_RENDER_TEST_CAPTURE_PASS_BEGIN_DX12
		#define FFX_RENDER_TEST_CAPTURE_PASS_BEGIN_DX12(Name)	
	#endif
	#ifndef FFX_RENDER_TEST_CAPTURE_PASS_ADD_DX12
		#define FFX_RENDER_TEST_CAPTURE_PASS_ADD_DX12(Dev, List, Tex, State, Frames, Name)	
	#endif
	#ifndef FFX_RENDER_TEST_CAPTURE_PASS_END_DX12
		#define FFX_RENDER_TEST_CAPTURE_PASS_END_DX12	
	#endif
#endif
