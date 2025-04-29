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

#include "XeSSSDKWrapperBase.h"

typedef struct _xess_vk_execute_params_t xess_vk_execute_params_t;
typedef struct _xess_vk_init_params_t xess_vk_init_params_t;
typedef struct VkCommandBuffer_T* VkCommandBuffer;
typedef struct VkDevice_T* VkDevice;
typedef struct VkInstance_T* VkInstance;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
typedef struct VkPipelineCache_T* VkPipelineCache;

class FXeSSSDKWrapperVulkan : public FXeSSSDKWrapperBase
{
public:
	FXeSSSDKWrapperVulkan(const TCHAR* InRHIName) : FXeSSSDKWrapperBase(InRHIName) {}
	bool IsSupported() const override;
public:
	typedef xess_result_t (*xessVKBuildPipelines_t)(xess_context_handle_t, VkPipelineCache, bool, uint32_t);
	xessVKBuildPipelines_t xessVKBuildPipelines = nullptr;
	typedef xess_result_t (*xessVKCreateContext_t)(VkInstance, VkPhysicalDevice, VkDevice, xess_context_handle_t*);
	xessVKCreateContext_t xessVKCreateContext = nullptr;
	typedef xess_result_t (*xessVKExecute_t)(xess_context_handle_t, VkCommandBuffer, const xess_vk_execute_params_t*);
	xessVKExecute_t xessVKExecute = nullptr;
	typedef xess_result_t (*xessVKGetRequiredInstanceExtensions_t)(uint32_t*, const char* const**, uint32_t*);
	xessVKGetRequiredInstanceExtensions_t xessVKGetRequiredInstanceExtensions = nullptr;
	typedef xess_result_t (*xessVKInit_t)(xess_context_handle_t, const xess_vk_init_params_t*);
	xessVKInit_t xessVKInit = nullptr;
private:
	bool LoadRHISpecificAPIs() override;
	const TCHAR* GetLibraryFileName() const override;
};
