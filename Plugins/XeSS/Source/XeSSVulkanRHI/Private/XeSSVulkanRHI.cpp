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

#include "XeSSVulkanRHI.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

#include "VulkanRHIPrivate.h"  // Required by VulkanDynamicRHI.h
#include "VulkanDevice.h"
#include "VulkanDynamicRHI.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xess/xess_vk.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "XeSSCommonMacros.h"
#include "XeSSSDKWrapperVulkan.h"
#include "XeSSUnrealVulkanRHI.h"
#include "XeSSUtil.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSVulkanRHI, Log, All);

namespace XeSSVulkanRHI
{
	inline VkInstance GetVkInstance(FVulkanDynamicRHI* RHI)
	{
		return (VkInstance)RHI->GetInstance();
	}
	inline VkDevice GetLogicalDevice(FVulkanDynamicRHI* RHI)
	{
		return (VkDevice)RHI->GetDevice()->GetInstanceHandle();
	}
	inline VkPhysicalDevice GetPhysicalDevice(FVulkanDynamicRHI* RHI)
	{
		return (VkPhysicalDevice)RHI->GetDevice()->GetPhysicalHandle();
	}
	inline VkImage GetNativeResource(FRHITexture* Texture)
	{
		return (VkImage)Texture->GetNativeResource();
	}
	inline xess_vk_image_view_info GetVkImageViewInfo(FRHITexture* Texture, const FIntRect& Rect, const VkImageSubresourceRange& ImageSubresourceRange)
	{
		xess_vk_image_view_info VkImageViewInfo{};
		VkImageViewInfo.image = GetNativeResource(Texture);
		VkImageViewInfo.imageView = XeSSUnreal::GetVulkanImageView(Texture);
		VkImageViewInfo.subresourceRange = ImageSubresourceRange;
		VkImageViewInfo.format = XeSSUnreal::GetVulkanFormat(Texture);
		VkImageViewInfo.height = Rect.Height();
		VkImageViewInfo.width = Rect.Width();
		return VkImageViewInfo;
	}
	inline VkImageSubresourceRange MakeSubresourceRange(VkImageAspectFlags AspectMask)
	{
		VkImageSubresourceRange Range;
		Range.aspectMask = AspectMask;
		Range.baseMipLevel = 0;
		Range.levelCount = 1;
		Range.baseArrayLayer = 0;
		Range.layerCount = 1;
		return Range;
	}
}

FXeSSVulkanRHI::FXeSSVulkanRHI()
	: VulkanRHI(static_cast<FVulkanDynamicRHI*>(GDynamicRHI))
{
	check(VulkanRHI);
}

FXeSSVulkanRHI::~FXeSSVulkanRHI()
{
}

void FXeSSVulkanRHI::OnXeSSSDKLoaded()
{
	check(XeSSSDKWrapper);

	XeSSSDKWrapperVulkan = static_cast<FXeSSSDKWrapperVulkan*>(XeSSSDKWrapper);
}

void FXeSSVulkanRHI::RHIInitializeXeSS(const FXeSSInitArguments& InArguments)
{
	if (!IsXeSSInitialized())
	{
		return;
	}
	InitArgs = InArguments;
	xess_vk_init_params_t InitParams = {};
	InitParams.outputResolution.x = InArguments.OutputWidth;
	InitParams.outputResolution.y = InArguments.OutputHeight;
	InitParams.initFlags = InArguments.InitFlags;
	InitParams.qualitySetting = XeSSUtil::ToXeSSQualitySetting(InArguments.QualitySetting);
	xess_result_t Result = XeSSSDKWrapperVulkan->xessVKInit(XeSSContext, &InitParams);
	if (XESS_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeSSVulkanRHI, Error, TEXT("Failed to initialize Intel XeSS, result: %d"), Result);
	}
}

void FXeSSVulkanRHI::RHIExecuteXeSS(const FXeSSExecuteArguments& InArguments, FRHICommandListBase& ExecutingCmdList)
{
	if (!IsXeSSInitialized())
	{
		return;
	}
	VkCommandBuffer VulkanCommandBuffer = XeSSUnreal::GetNativeCommandBuffer(VulkanRHI);
	xess_vk_execute_params_t ExecuteParams{};
	ExecuteParams.colorTexture = XeSSVulkanRHI::GetVkImageViewInfo(
		InArguments.ColorTexture,
		InArguments.SrcViewRect,
		XeSSVulkanRHI::MakeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT)
	);
	ExecuteParams.velocityTexture = XeSSVulkanRHI::GetVkImageViewInfo(
		InArguments.VelocityTexture,
		InArguments.SrcViewRect,
		XeSSVulkanRHI::MakeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT)
	);
	ExecuteParams.outputTexture = XeSSVulkanRHI::GetVkImageViewInfo(
		InArguments.OutputTexture,
		InArguments.DstViewRect,
		XeSSVulkanRHI::MakeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT)
	);
	ExecuteParams.jitterOffsetX = InArguments.JitterOffsetX;
	ExecuteParams.jitterOffsetY = InArguments.JitterOffsetY;
	ExecuteParams.resetHistory = InArguments.bCameraCut;
	ExecuteParams.inputWidth = InArguments.SrcViewRect.Width();
	ExecuteParams.inputHeight = InArguments.SrcViewRect.Height();
	ExecuteParams.inputColorBase.x = InArguments.SrcViewRect.Min.X;
	ExecuteParams.inputColorBase.y = InArguments.SrcViewRect.Min.Y;
	ExecuteParams.outputColorBase.x = InArguments.DstViewRect.Min.X;
	ExecuteParams.outputColorBase.y = InArguments.DstViewRect.Min.Y;
	ExecuteParams.exposureScale = 1.0f;

	xess_result_t Result = XeSSSDKWrapperVulkan->xessVKExecute(XeSSContext, VulkanCommandBuffer, &ExecuteParams);
	if (XESS_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeSSVulkanRHI, Error, TEXT("Failed to execute XeSS, result: %d"), Result);
	}
	XeSSUnreal::RHIFinishExternalComputeWork(VulkanRHI, VulkanCommandBuffer);
}

xess_result_t FXeSSVulkanRHI::CreateXeSSContext(xess_context_handle_t* OutXeSSContext)
{
	return XeSSSDKWrapperVulkan->xessVKCreateContext(
		XeSSVulkanRHI::GetVkInstance(VulkanRHI),
		XeSSVulkanRHI::GetPhysicalDevice(VulkanRHI),
		XeSSVulkanRHI::GetLogicalDevice(VulkanRHI),
		OutXeSSContext
	);
}

xess_result_t FXeSSVulkanRHI::BuildXeSSPipelines(xess_context_handle_t InXeSSContext, uint32_t InInitFlags)
{
	return XeSSSDKWrapperVulkan->xessVKBuildPipelines(InXeSSContext, nullptr, true, InInitFlags);
}
