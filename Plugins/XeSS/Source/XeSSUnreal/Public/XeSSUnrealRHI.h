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
// NOTE: keep following order for Unreal 4.27 and below
#include "RHI.h"
#include "RHIResources.h"
#include "DynamicRHI.h"

#include "ShaderParameterMacros.h"

class FRHICommandListImmediate;

#if ENGINE_MAJOR_VERSION >= 5
class FRHIBuffer;
#define ETEXTURE_CREATE_FLAGS(Flag) (ETextureCreateFlags::Flag)
#else
#define ETEXTURE_CREATE_FLAGS(Flag) (ETextureCreateFlags::TexCreate_##Flag)
class FRHIStructuredBuffer;
#endif

namespace XeSSUnreal
{
// NOTE: FTextureRHIRef also defined in Engine versions below 5.5, use it when possible
#if XESS_ENGINE_VERSION_GEQ(5, 5)
	using XTextureRHIRef = ::FTextureRHIRef;
#else
	using XTextureRHIRef = ::FTexture2DRHIRef;
#endif

#if ENGINE_MAJOR_VERSION >= 5
	using XRHIBuffer = ::FRHIBuffer;
#else
	using XRHIBuffer = ::FRHIStructuredBuffer;
#endif
	XESSUNREAL_API XRHIBuffer* GetRHIBuffer(TRDGBufferAccess<ERHIAccess::UAVCompute> BufferAccess);
	XESSUNREAL_API void* LockRHIBuffer(FRHICommandListImmediate& CommandList, XRHIBuffer* Buffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode);
	XESSUNREAL_API void UnlockRHIBuffer(FRHICommandListImmediate& CommandList, XRHIBuffer* Buffer);

	inline bool IsRHIInterfaceD3D12()
	{
		check(GDynamicRHI);
#if XESS_ENGINE_VERSION_GEQ(5, 1)
		return RHIGetInterfaceType() == ERHIInterfaceType::D3D12;
#else
		return (0 == FCString::Strcmp(GDynamicRHI->GetName(), TEXT("D3D12")));
#endif
	}
	inline bool IsRHIInterfaceVulkan()
	{
		check(GDynamicRHI);
#if XESS_ENGINE_VERSION_GEQ(5, 1)
		return RHIGetInterfaceType() == ERHIInterfaceType::Vulkan;
#else
		return (0 == FCString::Strcmp(GDynamicRHI->GetName(), TEXT("Vulkan")));
#endif
	}
	inline bool IsRHIInterfaceD3D11()
	{
		check(GDynamicRHI);
#if XESS_ENGINE_VERSION_GEQ(5, 1)
		return RHIGetInterfaceType() == ERHIInterfaceType::D3D11;
#else
		return (0 == FCString::Strcmp(GDynamicRHI->GetName(), TEXT("D3D11")));
#endif
	}
}
