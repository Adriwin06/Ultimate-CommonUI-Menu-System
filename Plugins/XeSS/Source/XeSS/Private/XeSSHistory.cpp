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

#include "XeSSHistory.h"

#if XESS_ENGINE_VERSION_GEQ(5, 3)
#include "XeSSUpscaler.h"

FXeSSHistory::FXeSSHistory(FXeSSUpscaler* InXeSSUpscaler)
{
	XeSSUpscaler = InXeSSUpscaler;
}

FXeSSHistory::~FXeSSHistory()
{
}

const TCHAR* FXeSSHistory::GetDebugName() const
{
	// WORKAROUND: use the same CHART* of the upscaler to pass check in Unreal 5.3 Preview, which is a bug
	return XeSSUpscaler->GetDebugName();
}

uint64 FXeSSHistory::GetGPUSizeBytes() const
{
	// TODO(sunzhuoshi): finish it
	return 0;
}
#endif
