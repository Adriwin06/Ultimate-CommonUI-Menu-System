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

#include "FFXSharedBackend.h"
#include "PixelFormat.h"
#include "RHI.h"
#if UE_VERSION_OLDER_THAN(5, 0, 0)
#include "RenderGraphBuilder.h"

FRDGBuilder* IFFXSharedBackend::GraphBuilder;
#endif

FFXSHARED_API FfxApiSurfaceFormat GetFFXApiFormat(EPixelFormat UEFormat, bool bSRGB)
{
	FfxApiSurfaceFormat Format = FFX_API_SURFACE_FORMAT_UNKNOWN;
	switch (UEFormat)
	{
	case PF_R32G32B32A32_UINT:
		Format = FFX_API_SURFACE_FORMAT_R32G32B32A32_UINT;
		break;
	case PF_A32B32G32R32F:
		Format = FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT;
		break;
	case PF_FloatRGBA:
		Format = FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT;
		break;
	case PF_A2B10G10R10:
		Format = FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM;
		break;
	case PF_G32R32F:
		Format = FFX_API_SURFACE_FORMAT_R32G32_FLOAT;
		break;
	case PF_R32_UINT:
		Format = FFX_API_SURFACE_FORMAT_R32_UINT;
		break;
	case PF_R8G8B8A8_UINT:
		Format = FFX_API_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
		break;
	case PF_R8G8B8A8:
		if (bSRGB)
		{
			Format = FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB;
			break;
		}
	case PF_B8G8R8A8:
		Format = FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM;
		break;
	case PF_FloatR11G11B10:
	case PF_FloatRGB:
		Format = FFX_API_SURFACE_FORMAT_R11G11B10_FLOAT;
		break;
	case PF_G16R16F:
		Format = FFX_API_SURFACE_FORMAT_R16G16_FLOAT;
		break;
	case PF_R16G16_UINT:
		Format = FFX_API_SURFACE_FORMAT_R16G16_UINT;
		break;
	case PF_R16F:
		Format = FFX_API_SURFACE_FORMAT_R16_FLOAT;
		break;
	case PF_R16_UINT:
		Format = FFX_API_SURFACE_FORMAT_R16_UINT;
		break;
	case PF_G16:
		Format = FFX_API_SURFACE_FORMAT_R16_UNORM;
		break;
	case PF_R16G16B16A16_SNORM:
		Format = FFX_API_SURFACE_FORMAT_R16_SNORM;
		break;
	case PF_R8:
		Format = FFX_API_SURFACE_FORMAT_R8_UNORM;
		break;
	case PF_R32_FLOAT:
		Format = FFX_API_SURFACE_FORMAT_R32_FLOAT;
		break;
	case PF_DepthStencil:
		Format = FFX_API_SURFACE_FORMAT_R32_FLOAT;
		break;
	case PF_R8G8:
		Format = FFX_API_SURFACE_FORMAT_R8G8_UNORM;
		break;
	case PF_R8_UINT:
		Format = FFX_API_SURFACE_FORMAT_R8_UINT;
		break;
	case PF_R16G16B16A16_SINT:
		Format = FFX_API_SURFACE_FORMAT_R16G16_SINT;
		break;
	case PF_A16B16G16R16:
		Format = FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT;
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
#if UE_VERSION_AT_LEAST(5, 1, 0)
		Access = ERHIAccess::ReadOnlyExclusiveComputeMask;
#else
		Access = ERHIAccess::ReadOnlyExclusiveMask;
#endif
		break;
	case FFX_RESOURCE_STATE_INDIRECT_ARGUMENT:
		Access = ERHIAccess::IndirectArgs;
		break;
	default:
		Access = ERHIAccess::Unknown;
		break;
	}

	return Access;
}
