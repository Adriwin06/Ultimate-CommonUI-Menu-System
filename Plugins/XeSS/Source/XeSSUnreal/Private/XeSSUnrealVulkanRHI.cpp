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

#include "XeSSUnrealVulkanRHI.h"

#include "XeSSCommonMacros.h"

#include "VulkanRHIPrivate.h"  // Required by VulkanDynamicRHI.h
#include "VulkanContext.h"
#include "VulkanDynamicRHI.h"
#include "VulkanPendingState.h"

#if XESS_ENGINE_VERSION_GEQ(5, 1)
#include "IVulkanDynamicRHI.h"
#else
#include "VulkanRHIBridge.h"
#endif

namespace XeSSUnreal
{

	void AddEnabledInstanceExtensionsAndLayers(const TArray<const ANSICHAR*>& InInstanceExtensions, const TArray<const ANSICHAR*>& InInstanceLayers)
	{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
		IVulkanDynamicRHI::AddEnabledInstanceExtensionsAndLayers(InInstanceExtensions, InInstanceLayers);
#else
		VulkanRHIBridge::AddEnabledInstanceExtensionsAndLayers(InInstanceExtensions, InInstanceLayers);
#endif
	}

	void AddEnabledDeviceExtensionsAndLayers(const TArray<const ANSICHAR*>& InDeviceExtensions, const TArray<const ANSICHAR*>& InDeviceLayers)
	{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
		IVulkanDynamicRHI::AddEnabledDeviceExtensionsAndLayers(InDeviceExtensions, InDeviceLayers);
#else
		VulkanRHIBridge::AddEnabledDeviceExtensionsAndLayers(InDeviceExtensions, InDeviceLayers);
#endif
	}

	VkCommandBuffer GetNativeCommandBuffer(FVulkanDynamicRHI* RHI)
	{
		return (VkCommandBuffer)RHI->GetDevice()->GetImmediateContext().GetCommandBufferManager()->GetActiveCmdBufferDirect()->GetHandle();
	}

	XESSUNREAL_API VkFormat GetVulkanFormat(FRHITexture* Texture)
	{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
		return static_cast<VkFormat>(static_cast<FVulkanTexture*>(Texture->GetTexture2D())->ViewFormat);
#else
		return static_cast<VkFormat>(static_cast<FVulkanTexture2D*>(Texture->GetTexture2D())->Surface.ViewFormat);
#endif
	}

	VkImageView GetVulkanImageView(FRHITexture* Texture)
	{
#if XESS_ENGINE_VERSION_GEQ(5, 3)
		return static_cast<FVulkanTexture*>(Texture->GetTexture2D())->DefaultView->GetTextureView().View;
#elif XESS_ENGINE_VERSION_GEQ(5, 1)
		return static_cast<VkImageView>(static_cast<FVulkanTexture*>(Texture->GetTexture2D())->DefaultView.View);
#else
		return static_cast<VkImageView>(static_cast<FVulkanTexture2D*>(Texture->GetTexture2D())->DefaultView.View);
#endif
	}

	void RHIFinishExternalComputeWork(FVulkanDynamicRHI* RHI, VkCommandBuffer InCommandBuffer)
	{
		FVulkanCommandListContextImmediate& ImmediateContext = RHI->GetDevice()->GetImmediateContext();
		check(InCommandBuffer == ImmediateContext.GetCommandBufferManager()->GetActiveCmdBuffer()->GetHandle());

		ImmediateContext.GetPendingComputeState()->Reset();
		ImmediateContext.GetPendingGfxState()->Reset();
	}
}
