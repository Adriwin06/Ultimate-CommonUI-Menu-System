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

struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12PipelineLibrary;
typedef struct _xess_d3d12_execute_params_t xess_d3d12_execute_params_t;
typedef struct _xess_d3d12_init_params_t xess_d3d12_init_params_t;

class FXeSSSDKWrapperD3D12 : public FXeSSSDKWrapperBase
{
public:
	FXeSSSDKWrapperD3D12(const TCHAR* InRHIName) : FXeSSSDKWrapperBase(InRHIName) {}
	bool IsSupported() const override;
public:
	typedef xess_result_t(*xessD3D12BuildPipelines_t)(xess_context_handle_t, ID3D12PipelineLibrary*, bool, uint32_t);
	xessD3D12BuildPipelines_t xessD3D12BuildPipelines = nullptr;
	typedef xess_result_t (*xessD3D12CreateContext_t)(ID3D12Device*, xess_context_handle_t*);
	xessD3D12CreateContext_t xessD3D12CreateContext = nullptr;
	typedef xess_result_t (*xessD3D12Execute_t)(xess_context_handle_t, ID3D12GraphicsCommandList*, const xess_d3d12_execute_params_t*);
	xessD3D12Execute_t xessD3D12Execute = nullptr;
	typedef xess_result_t (*xessD3D12Init_t)(xess_context_handle_t, const xess_d3d12_init_params_t*);
	xessD3D12Init_t xessD3D12Init = nullptr;
private:
	bool LoadRHISpecificAPIs() override;
	const TCHAR* GetLibraryFileName() const override;
};
