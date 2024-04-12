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

#include "XeSSUnreal.h"
#include "XeSSUnrealIncludes.h"

namespace XeSSUnreal
{
ID3D12Device* GetDevice(XD3D12DynamicRHI* D3D12DynamicRHI, uint32_t Index)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	return D3D12DynamicRHI->RHIGetDevice(Index);
#else // XESS_ENGINE_VERSION_GEQ(5, 1)
	return D3D12DynamicRHI->GetAdapter(Index).GetD3DDevice();
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)
}

ID3D12Resource* GetResource(XD3D12DynamicRHI* D3D12DynamicRHI, FRHITexture* Texture)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	return D3D12DynamicRHI->RHIGetResource(Texture);
#else // XESS_ENGINE_VERSION_GEQ(5, 1)
	(void)D3D12DynamicRHI;
	return GetD3D12TextureFromRHITexture(Texture)->GetResource()->GetResource();
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)
}
} // namespace XeSSUnreal
