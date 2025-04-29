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

struct ID3D11Device;
typedef struct _xess_d3d11_execute_params_t xess_d3d11_execute_params_t;
typedef struct _xess_d3d11_init_params_t xess_d3d11_init_params_t;

class FXeSSSDKWrapperD3D11 : public FXeSSSDKWrapperBase
{
public:
	FXeSSSDKWrapperD3D11(const TCHAR* InRHIName) : FXeSSSDKWrapperBase(InRHIName) {}
	bool IsSupported() const override;
public:
	typedef xess_result_t (*xessD3D11CreateContext_t)(ID3D11Device*, xess_context_handle_t*);
	xessD3D11CreateContext_t xessD3D11CreateContext = nullptr;
	typedef xess_result_t (*xessD3D11Execute_t)(xess_context_handle_t, const xess_d3d11_execute_params_t*);
	xessD3D11Execute_t xessD3D11Execute = nullptr;
	typedef xess_result_t (*xessD3D11Init_t)(xess_context_handle_t, const xess_d3d11_init_params_t*);
	xessD3D11Init_t xessD3D11Init = nullptr;
private:
	bool LoadRHISpecificAPIs() override;
	const TCHAR* GetLibraryFileName() const override;
};
