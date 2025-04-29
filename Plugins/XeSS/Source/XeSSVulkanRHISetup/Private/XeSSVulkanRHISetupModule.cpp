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

#include "XeSSVulkanRHISetupModule.h"

 // `RHI.h` and `RHIResources.h` are pre-required includes for `DynamicRHI.h`
#include "RHI.h"
#include "RHIResources.h"
#include "DynamicRHI.h"
#include "Misc/App.h"
#include "Modules/ModuleManager.h"

#include "XeSSSDKLoader.h"
#include "XeSSSDKWrapperVulkan.h"
#include "XeSSUnrealVulkanRHI.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSVulkanRHISetupModule, Log, All);

void FXeSSVulkanRHISetupModule::StartupModule()
{
	// Required by GetSelectedDynamicRHIModuleName() and false when cooking
	if (!FApp::CanEverRender())
	{
		return;
	}
	const TCHAR* DynamicRHIModuleName = GetSelectedDynamicRHIModuleName(false);

	if (FString("VulkanRHI") == FString(DynamicRHIModuleName))
	{
		uint32_t InstanceExtensionCount = 0;
		const char* const* InstanceExtensions = nullptr;
		uint32_t MinVKApiVersion = 0;

		FXeSSSDKWrapperVulkan* XeSSSDKWrapper = static_cast<FXeSSSDKWrapperVulkan*>(FXeSSSDKLoader::Load(TEXT("Vulkan")));
		check(XeSSSDKWrapper);
		if (!XeSSSDKWrapper->IsLoaded())
		{
			UE_LOG(LogXeSSVulkanRHISetupModule, Log, TEXT("Failed to load XeSS SDK"));
			return;
		}
		xess_result_t Result = XeSSSDKWrapper->xessVKGetRequiredInstanceExtensions(&InstanceExtensionCount, &InstanceExtensions, &MinVKApiVersion);
		if (XESS_RESULT_SUCCESS != Result)
		{
			UE_LOG(LogXeSSVulkanRHISetupModule, Error, TEXT("Failed to get required instance extensions, result: %d"), Result);
			return;
		}
		const TArray<const ANSICHAR*> UnrealInstanceExtensions(InstanceExtensions, InstanceExtensionCount);
		XeSSUnreal::AddEnabledInstanceExtensionsAndLayers(UnrealInstanceExtensions, TArray<const ANSICHAR*>());
		// TODO(sunzhuoshi): set device extensions via XeSSUnreal::AddEnabledDeviceExtensionsAndLayers()
	}
}

IMPLEMENT_MODULE(FXeSSVulkanRHISetupModule, XeSSVulkanRHISetup)
