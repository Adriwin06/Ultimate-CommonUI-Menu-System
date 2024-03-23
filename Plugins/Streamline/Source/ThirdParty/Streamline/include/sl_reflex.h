/*
* Copyright (c) 2022 NVIDIA CORPORATION. All rights reserved
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

namespace sl
{

enum ReflexMode
{
    eOff,
    eLowLatency,
    eLowLatencyWithBoost,
};

// {F03AF81A-6D0B-4902-A651-C4965E215434}
SL_STRUCT(ReflexOptions, StructType({ 0xf03af81a, 0x6d0b, 0x4902, { 0xa6, 0x51, 0xc4, 0x96, 0x5e, 0x21, 0x54, 0x34 } }), kStructVersion1)
    //! Specifies which mode should be used
    ReflexMode mode = ReflexMode::eOff;
    //! Specifies if frame limiting (FPS cap) is enabled (0 to disable, microseconds otherwise).
    //! One benefit of using Reflex's FPS cap over other implementations is the driver would be aware and can provide better optimizations.
    //! This setting is independent of ReflexOptions::mode; it can even be used with eReflexModeOff.
    uint32_t frameLimitUs = 0;
    //! Specifies if markers can be used for optimization or not.  Set to true UNLESS (if any of the below apply, set to false):
    //! - The game is single threaded (i.e. simulation for frame X+1 cannot start until render submission for frame X is done)
    //! - The present call is not called right after render submission
    //! - Simulation does not happen exactly once per render frame
    bool useMarkersToOptimize = false;
    //! Specifies the hot-key which should be used instead of custom message for PC latency marker
    //! Possible values: VK_F13, VK_F14, VK_F15
    uint16_t virtualKey = 0;
    //! ThreadID for reflex messages
    uint32_t idThread = 0;

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};

// {0D569B37-A1C8-4453-BE4D-40F4DE57952B}
SL_STRUCT(ReflexReport, StructType({ 0xd569b37, 0xa1c8, 0x4453, { 0xbe, 0x4d, 0x40, 0xf4, 0xde, 0x57, 0x95, 0x2b } }), kStructVersion1)
    //! Various latency related stats
    uint64_t frameID{};
    uint64_t inputSampleTime{};
    uint64_t simStartTime{};
    uint64_t simEndTime{};
    uint64_t renderSubmitStartTime{};
    uint64_t renderSubmitEndTime{};
    uint64_t presentStartTime{};
    uint64_t presentEndTime{};
    uint64_t driverStartTime{};
    uint64_t driverEndTime{};
    uint64_t osRenderQueueStartTime{};
    uint64_t osRenderQueueEndTime{};
    uint64_t gpuRenderStartTime{};
    uint64_t gpuRenderEndTime{};
    uint32_t gpuActiveRenderTimeUs{};
    uint32_t gpuFrameTimeUs{};

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};

// {F0BB5985-DAF9-4728-B2FD-AE80A2BD7989}
SL_STRUCT(ReflexState, StructType({ 0xf0bb5985, 0xdaf9, 0x4728, { 0xb2, 0xfd, 0xae, 0x80, 0xa2, 0xbd, 0x79, 0x89 } }), kStructVersion1)
    //! Specifies if low-latency mode is available or not
    bool lowLatencyAvailable = false;
    //! Specifies if the frameReport below contains valid data or not
    bool latencyReportAvailable = false;
    //! Specifies low latency Windows message id (if ReflexOptions::virtualKey is 0)
    uint32_t statsWindowMessage;
    //! Reflex report per frame
    ReflexReport frameReport[64];
    //! Specifies ownership of flash indicator toggle (true = driver, false = application)
    bool flashIndicatorDriverControlled = false;

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};

enum ReflexMarker
{
    eSimulationStart,
    eSimulationEnd,
    eRenderSubmitStart,
    eRenderSubmitEnd,
    ePresentStart,
    ePresentEnd,
    eInputSample,
    eTriggerFlash,
    ePCLatencyPing,
    eOutOfBandRenderSubmitStart,
    eOutOfBandRenderSubmitEnd,
    eOutOfBandPresentStart,
    eOutOfBandPresentEnd
};

// {E268B3DC-F963-4C37-9776-AF048E132621}
SL_STRUCT(ReflexHelper, StructType({ 0xe268b3dc, 0xf963, 0x4c37, { 0x97, 0x76, 0xaf, 0x4, 0x8e, 0x13, 0x26, 0x21 } }), kStructVersion1)
    ReflexHelper(ReflexMarker m) : BaseStructure(ReflexHelper::s_structType, kStructVersion1), marker(m) {};
    operator ReflexMarker () const { return marker; };
private:
    ReflexMarker marker;
};


}

//! Provides Reflex settings
//!
//! Call this method to check if Reflex is on, get stats etc.
//!
//! @param state Reference to a structure where states are returned
//! @return sl::ResultCode::eOk if successful, error code otherwise (see sl_result.h for details)
//!
//! This method is NOT thread safe.
using PFun_slReflexGetState = sl::Result(sl::ReflexState& state);

//! Sets Reflex marker
//!
//! Call this method to set specific Reflex marker
//!
//! @param marker Specifies which marker to use
//! @param frame Specifies current frame
//! @return sl::ResultCode::eOk if successful, error code otherwise (see sl_result.h for details)
//!
//! This method is thread safe.
using PFun_slReflexSetMarker = sl::Result(sl::ReflexMarker marker, const sl::FrameToken& frame);

//! Tells reflex to sleep the app
//!
//! Call this method to invoke Reflex sleep in your application.
//!
//! @param frame Specifies current frame
//! @return sl::ResultCode::eOk if successful, error code otherwise (see sl_result.h for details)
//!
//! This method is thread safe.
using PFun_slReflexSleep = sl::Result(const sl::FrameToken& frame);

//! Sets Reflex options
//!
//! Call this method to turn Reflex on/off, change mode etc.
//!
//! @param options Specifies options to use
//! @return sl::ResultCode::eOk if successful, error code otherwise (see sl_result.h for details)
//!
//! This method is NOT thread safe.
using PFun_slReflexSetOptions = sl::Result(const sl::ReflexOptions& options);

//! HELPERS
//! 
inline sl::Result slReflexGetState(sl::ReflexState& state)
{
    SL_FEATURE_FUN_IMPORT_STATIC(sl::kFeatureReflex, slReflexGetState);
    return s_slReflexGetState(state);
}

inline sl::Result slReflexSetMarker(sl::ReflexMarker marker, const sl::FrameToken& frame)
{
    SL_FEATURE_FUN_IMPORT_STATIC(sl::kFeatureReflex, slReflexSetMarker);
    return s_slReflexSetMarker(marker, frame);
}

inline sl::Result slReflexSleep(const sl::FrameToken& frame)
{
    SL_FEATURE_FUN_IMPORT_STATIC(sl::kFeatureReflex, slReflexSleep);
    return s_slReflexSleep(frame);
}

inline sl::Result slReflexSetOptions(const sl::ReflexOptions& options)
{
    SL_FEATURE_FUN_IMPORT_STATIC(sl::kFeatureReflex, slReflexSetOptions);
    return s_slReflexSetOptions(options);
}
