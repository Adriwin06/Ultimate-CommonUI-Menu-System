/*******************************************************************************
 * Copyright 2023 Intel Corporation
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

#if XESS_ENGINE_VERSION_GEQ(5, 3)
#include "CoreMinimal.h"
#include "TemporalUpscaler.h"

class FXeSSUpscaler;

class FXeSSHistory final : public UE::Renderer::Private::ITemporalUpscaler::IHistory
{
public:
	FXeSSHistory(FXeSSUpscaler* InXeSSUpscaler);
	virtual ~FXeSSHistory();
	const TCHAR* GetDebugName() const final;
	uint64 GetGPUSizeBytes() const final;
	uint32 Release() const final { return --RefCount; }
	uint32 AddRef() const final { return ++RefCount; }
	uint32 GetRefCount() const final { return RefCount; }
private:
	mutable uint32 RefCount = 0;
	FXeSSUpscaler* XeSSUpscaler = nullptr;
};
#endif
