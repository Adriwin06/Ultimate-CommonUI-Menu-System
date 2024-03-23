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

#include "FFXSharedBackend.h"
#include "PixelFormat.h"
#include "RHI.h"

FFXSHARED_API EPixelFormat GetUEFormat(FfxSurfaceFormat Format)
{
	EPixelFormat UEFormat = PF_Unknown;
	switch (Format)
	{
	case FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS:
		UEFormat = PF_R32G32B32A32_UINT;
		break;
	case FFX_SURFACE_FORMAT_R32G32B32A32_UINT:
		UEFormat = PF_R32G32B32A32_UINT;
		break;
	case FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT:
		UEFormat = PF_A32B32G32R32F;
		break;
	case FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT:
		UEFormat = PF_FloatRGBA;
		break;
	case FFX_SURFACE_FORMAT_R10G10B10A2_UNORM:
		UEFormat = PF_A2B10G10R10;
		break;
	case FFX_SURFACE_FORMAT_R32G32_FLOAT:
		UEFormat = PF_G32R32F;
		break;
	case FFX_SURFACE_FORMAT_R32_UINT:
		UEFormat = PF_R32_UINT;
		break;
	case FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS:
		UEFormat = PF_R8G8B8A8_UINT;
		break;
	case FFX_SURFACE_FORMAT_R8G8B8A8_UNORM:
		UEFormat = PF_R8G8B8A8;
		break;
	case FFX_SURFACE_FORMAT_R8G8B8A8_SRGB:
		UEFormat = PF_R8G8B8A8;
		break;
	case FFX_SURFACE_FORMAT_R11G11B10_FLOAT:
		UEFormat = PF_FloatR11G11B10;
		break;
	case FFX_SURFACE_FORMAT_R16G16_FLOAT:
		UEFormat = PF_G16R16F;
		break;
	case FFX_SURFACE_FORMAT_R16G16_UINT:
		UEFormat = PF_R16G16_UINT;
		break;
	case FFX_SURFACE_FORMAT_R16_FLOAT:
		UEFormat = PF_R16F;
		break;
	case FFX_SURFACE_FORMAT_R16_UINT:
		UEFormat = PF_R16_UINT;
		break;
	case FFX_SURFACE_FORMAT_R16_UNORM:
		UEFormat = PF_G16;
		break;
	case FFX_SURFACE_FORMAT_R16_SNORM:
		UEFormat = PF_R16G16B16A16_SNORM;
		break;
	case FFX_SURFACE_FORMAT_R8_UNORM:
		UEFormat = PF_R8;
		break;
	case FFX_SURFACE_FORMAT_R8_UINT:
		UEFormat = PF_R8_UINT;
		break;
	case FFX_SURFACE_FORMAT_R32_FLOAT:
		UEFormat = PF_R32_FLOAT;
		break;
	case FFX_SURFACE_FORMAT_R8G8_UNORM:
		UEFormat = PF_R8G8;
		break;
	case FFX_SURFACE_FORMAT_R16G16_SINT:
		UEFormat = PF_R16G16B16A16_SINT;
		break;
	default:
		check(false);
		break;
	}
	return UEFormat;
}

FFXSHARED_API FfxSurfaceFormat GetFFXFormat(EPixelFormat UEFormat, bool bSRGB)
{
	FfxSurfaceFormat Format = FFX_SURFACE_FORMAT_UNKNOWN;
	switch (UEFormat)
	{
	case PF_R32G32B32A32_UINT:
		Format = FFX_SURFACE_FORMAT_R32G32B32A32_UINT;
		break;
	case PF_A32B32G32R32F:
		Format = FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
		break;
	case PF_FloatRGBA:
		Format = FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
		break;
	case PF_A2B10G10R10:
		Format = FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;
		break;
	case PF_G32R32F:
		Format = FFX_SURFACE_FORMAT_R32G32_FLOAT;
		break;
	case PF_R32_UINT:
		Format = FFX_SURFACE_FORMAT_R32_UINT;
		break;
	case PF_R8G8B8A8_UINT:
		Format = FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
		break;
	case PF_R8G8B8A8:
		if (bSRGB)
		{
			Format = FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
			break;
		}
	case PF_B8G8R8A8:
		Format = FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
		break;
	case PF_FloatR11G11B10:
	case PF_FloatRGB:
		Format = FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
		break;
	case PF_G16R16F:
		Format = FFX_SURFACE_FORMAT_R16G16_FLOAT;
		break;
	case PF_R16G16_UINT:
		Format = FFX_SURFACE_FORMAT_R16G16_UINT;
		break;
	case PF_R16F:
		Format = FFX_SURFACE_FORMAT_R16_FLOAT;
		break;
	case PF_R16_UINT:
		Format = FFX_SURFACE_FORMAT_R16_UINT;
		break;
	case PF_G16:
		Format = FFX_SURFACE_FORMAT_R16_UNORM;
		break;
	case PF_R16G16B16A16_SNORM:
		Format = FFX_SURFACE_FORMAT_R16_SNORM;
		break;
	case PF_R8:
		Format = FFX_SURFACE_FORMAT_R8_UNORM;
		break;
	case PF_R32_FLOAT:
		Format = FFX_SURFACE_FORMAT_R32_FLOAT;
		break;
	case PF_DepthStencil:
		Format = FFX_SURFACE_FORMAT_R32_FLOAT;
		break;
	case PF_R8G8:
		Format = FFX_SURFACE_FORMAT_R8G8_UNORM;
		break;
	case PF_R8_UINT:
		Format = FFX_SURFACE_FORMAT_R8_UINT;
		break;
	case PF_R16G16B16A16_SINT:
		Format = FFX_SURFACE_FORMAT_R16G16_SINT;
		break;
	case PF_A16B16G16R16:
		Format = FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
		break;
	default:
		check(false);
		break;
	}
	return Format;
}

FFXSHARED_API ERHIAccess GetUEAccessState(FfxResourceStates State)
{
	ERHIAccess Access = ERHIAccess::Unknown;

	switch (State)
	{
	case FFX_RESOURCE_STATE_UNORDERED_ACCESS:
		Access = ERHIAccess::UAVMask;
		break;
	case FFX_RESOURCE_STATE_PIXEL_READ:
		Access = ERHIAccess::SRVGraphics;
		break;
	case FFX_RESOURCE_STATE_COMPUTE_READ:
		Access = ERHIAccess::SRVCompute;
		break;
	case FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ:
		Access = ERHIAccess::SRVMask;
		break;
	case FFX_RESOURCE_STATE_COPY_SRC:
		Access = ERHIAccess::CopySrc;
		break;
	case FFX_RESOURCE_STATE_COPY_DEST:
		Access = ERHIAccess::CopyDest;
		break;
	case FFX_RESOURCE_STATE_PRESENT:
		Access = ERHIAccess::Present;
		break;
	case FFX_RESOURCE_STATE_COMMON:
		Access = ERHIAccess::SRVMask;
		break;
	case FFX_RESOURCE_STATE_GENERIC_READ:
		Access = ERHIAccess::ReadOnlyExclusiveComputeMask;
		break;
	case FFX_RESOURCE_STATE_INDIRECT_ARGUMENT:
		Access = ERHIAccess::IndirectArgs;
		break;
	default:
		check(false);
		break;
	}

	return Access;
}
