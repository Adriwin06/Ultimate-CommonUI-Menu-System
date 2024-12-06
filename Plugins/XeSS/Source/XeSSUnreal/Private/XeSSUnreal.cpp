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

#if XESS_ENGINE_VERSION_GEQ(5, 1)
#include "Misc/ConfigUtilities.h"
#else
#include "Misc/ConfigCacheIni.h"
#endif

#include "RenderGraphResources.h"
#include "XeSSUnrealCore.h"
#include "XeSSUnrealD3D12RHI.h"
#include "XeSSUnrealD3D12RHIIncludes.h"
#include "XeSSUnrealRHI.h"

namespace XeSSUnreal
{

void ApplyCVarSettingsFromIni(const TCHAR* InSectionBaseName, const TCHAR* InIniFilename, uint32 SetBy, bool bAllowCheating)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	UE::ConfigUtilities::ApplyCVarSettingsFromIni(InSectionBaseName, InIniFilename, SetBy, bAllowCheating);
#else
	::ApplyCVarSettingsFromIni(InSectionBaseName, InIniFilename, SetBy, bAllowCheating);
#endif
}

ID3D12Device* GetDevice(XD3D12DynamicRHI* D3D12DynamicRHI)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	// No SLI/Crossfire support required, only 1 GPU node
	return D3D12DynamicRHI->RHIGetDevice(0);
#else
	return D3D12DynamicRHI->GetAdapter().GetD3DDevice();
#endif
}

ID3D12Resource* GetResource(XD3D12DynamicRHI* D3D12DynamicRHI, FRHITexture* Texture)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	return D3D12DynamicRHI->RHIGetResource(Texture);
#else
	(void)D3D12DynamicRHI;
	return GetD3D12TextureFromRHITexture(Texture)->GetResource()->GetResource();
#endif
}

ID3D12GraphicsCommandList* RHIGetGraphicsCommandList(XD3D12DynamicRHI* D3D12DynamicRHI)
{
	// No SLI/Crossfire support required, only 1 GPU node
	int DeviceIndex = 0;
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	return D3D12DynamicRHI->RHIGetGraphicsCommandList(DeviceIndex);
#else
	return D3D12DynamicRHI->GetAdapter().GetDevice(DeviceIndex)->GetCommandContext().CommandListHandle.GraphicsCommandList();
#endif
}
void RHIFinishExternalComputeWork(XD3D12DynamicRHI* D3D12DynamicRHI, ID3D12GraphicsCommandList* CommandList)
{
	// No SLI/Crossfire support required, only 1 GPU node
	int DeviceIndex = 0;
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	D3D12DynamicRHI->RHIFinishExternalComputeWork(DeviceIndex, CommandList);
#else
	(void)D3D12DynamicRHI;
	(void)CommandList;
	FD3D12CommandContext& CommandContext = D3D12DynamicRHI->GetAdapter().GetDevice(DeviceIndex)->GetCommandContext();
	FD3D12StateCache& StateCache = CommandContext.StateCache;
	StateCache.ForceSetComputeRootSignature();
	StateCache.GetDescriptorCache()->SetCurrentCommandList(CommandContext.CommandListHandle);
#endif
}

XRHIBuffer* GetRHIBuffer(TRDGBufferAccess<ERHIAccess::UAVCompute> BufferAccess)
{
#if ENGINE_MAJOR_VERSION >= 5
	return BufferAccess->GetRHI();
#else
	return BufferAccess->GetRHIStructuredBuffer();
#endif
}

void* LockRHIBuffer(FRHICommandListImmediate& CommandList, XRHIBuffer* Buffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode)
{
#if ENGINE_MAJOR_VERSION >= 5
	return CommandList.LockBuffer(Buffer, Offset, SizeRHI, LockMode);
#else
	return CommandList.LockStructuredBuffer(Buffer, Offset, SizeRHI, LockMode);
#endif
}

void UnlockRHIBuffer(FRHICommandListImmediate& CommandList, XRHIBuffer* Buffer)
{
#if ENGINE_MAJOR_VERSION >= 5
	CommandList.UnlockBuffer(Buffer);
#else
	CommandList.UnlockStructuredBuffer(Buffer);
#endif
}

}
