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

#include "XeSSRHI.h"
#include "XeSSUnrealD3D12RHI.h"

class FXeSSSDKWrapperD3D12;

class FXeSSD3D12RHI : public FXeSSRHI
{
public:
	FXeSSD3D12RHI();
	virtual ~FXeSSD3D12RHI();
	void OnXeSSSDKLoaded() override;
	void RHIInitializeXeSS(const FXeSSInitArguments& InArguments) override;
	void RHIExecuteXeSS(const FXeSSExecuteArguments& InArguments, FRHICommandListBase& ExecutingCmdList) override;
	xess_result_t CreateXeSSContext(xess_context_handle_t* OutXeSSContext) override;
	xess_result_t BuildXeSSPipelines(xess_context_handle_t InXeSSContext, uint32_t InInitFlags) override;
private:
	XeSSUnreal::XD3D12DynamicRHI* D3D12RHI = nullptr;
	FXeSSSDKWrapperD3D12* XeSSSDKWrapperD3D12 = nullptr;
};
