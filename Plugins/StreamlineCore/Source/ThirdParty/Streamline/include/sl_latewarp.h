/*
* Copyright (c) 2022-2024 NVIDIA CORPORATION. All rights reserved
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/ 

#pragma once
#include "sl_core_types.h"
namespace sl
{

// {f9147248-3ebc-4c44-9be9-8dee5e6c05f1}
SL_STRUCT_BEGIN(LatewarpOptions, StructType({ 0xf9147248, 0x3ebc, 0x4c44, { 0x9b, 0xe9, 0x8d, 0xee, 0x5e, 0x6c, 0x05, 0xf1 } }), kStructVersion2)
    unsigned int reserved0 = 0u;
    bool latewarpActive = false;
    //! Optional - if specified Latewarp will return any errors which occur when calling underlying API (DXGI or Vulkan or NGX)
    PFunOnAPIErrorCallback* onErrorCallback{};

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
SL_STRUCT_END()

}

//! Sets Latewarp options
//!
//! Call this method to turn Latewarp on/off, change mode etc.
//!
//! @param options Specifies options to use
//! @return sl::ResultCode::eOk if successful, error code otherwise (see sl_result.h for details)
//!
//! This method is NOT thread safe.
using PFun_slLatewarpSetOptions = sl::Result(const sl::ViewportHandle& viewport, const sl::LatewarpOptions& options);

inline sl::Result slLatewarpSetOptions(const sl::ViewportHandle& viewport, const sl::LatewarpOptions& options)
{
    SL_FEATURE_FUN_IMPORT_STATIC(sl::kFeatureLatewarp, slLatewarpSetOptions);
    return s_slLatewarpSetOptions(viewport, options);
}
