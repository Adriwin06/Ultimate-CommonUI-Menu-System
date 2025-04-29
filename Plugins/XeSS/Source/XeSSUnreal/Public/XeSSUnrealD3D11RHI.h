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

class FRHITexture;
struct ID3D11Device;
struct ID3D11Resource;

#if XESS_ENGINE_VERSION_GEQ(5, 1)
struct ID3D11DynamicRHI;
#else
class FD3D11DynamicRHI;
#endif

namespace XeSSUnreal
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	using XD3D11DynamicRHI = ::ID3D11DynamicRHI;
#else
	using XD3D11DynamicRHI = ::FD3D11DynamicRHI;
#endif

	XESSUNREAL_API ID3D11Device* GetDevice(XD3D11DynamicRHI* D3D11DynamicRHI);
	XESSUNREAL_API ID3D11Resource* GetResource(XD3D11DynamicRHI* D3D11DynamicRHI, FRHITexture* Texture);
}
