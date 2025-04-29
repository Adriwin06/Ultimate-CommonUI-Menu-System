/*******************************************************************************
 * Copyright 2024 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#include "XeSSCommonMacros.h"

#include "CoreMinimal.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d12.h>  // NOLINT(build/include_order)
#include "Windows/HideWindowsPlatformTypes.h"

class FD3D12DynamicRHI;
class FRHICommandListBase;
class FRHITexture;
struct ID3D12DynamicRHI;

namespace XeSSUnreal
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	using XD3D12DynamicRHI = ::ID3D12DynamicRHI;
#else
	using XD3D12DynamicRHI = ::FD3D12DynamicRHI;
#endif

	XESSUNREAL_API ID3D12Device* GetDevice(XD3D12DynamicRHI* D3D12DynamicRHI);
	XESSUNREAL_API ID3D12Resource* GetResource(XD3D12DynamicRHI* D3D12DynamicRHI, FRHITexture* Texture);

	XESSUNREAL_API ID3D12CommandQueue* RHIGetCommandQueue(XD3D12DynamicRHI* D3D12DynamicRHI);
	XESSUNREAL_API ID3D12GraphicsCommandList* RHIGetGraphicsCommandList(XD3D12DynamicRHI* D3D12DynamicRHI, FRHICommandListBase& ExecutingCmdList);
	XESSUNREAL_API void RHIFinishExternalComputeWork(XD3D12DynamicRHI* D3D12DynamicRHI, FRHICommandListBase& ExecutingCmdList, ID3D12GraphicsCommandList* CommandList);
}
