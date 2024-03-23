
Streamline - Reflex
=======================

>The focus of this guide is on using Streamline to integrate Reflex into an application.  For more information about Reflex itself, please visit the [NVIDIA Developer Reflex Page](https://developer.nvidia.com/performance-rendering-tools/reflex)

Version 2.2.1
=======

Here is an overview list of sub-features in the Reflex plugin:

| Feature | GPU Vendor | GPU HW | Driver Version | Support Check | Key Setting/Marker |
| ------ | ------ | ------ | ------ | ------ | ------ |
| **Reflex Low Latency** | NVDA only | GeForce 900 Series and newer | 456.38+ | `sl::ReflexSettings::lowLatencyAvailable` | `sl::ReflexConstants::mode` |
| **Auto-Configure Reflex Analyzer** | NVDA only | GeForce 900 Series and newer | 521.60+ | `sl::ReflexSettings::flashIndicatorDriverControlled` | `sl::ReflexMarker::eReflexMarkerTriggerFlash` |
| **Frame Rate Limiter** | NVDA only | All | All | Always | `sl::ReflexConstants::frameLimitUs` |
| **PC Latency Stats** | All | All | All | Always | `sl::ReflexMarker::eReflexMarkerPCLatencyPing` |

> **NOTE:**
> The sub-features are distinct to each other without any cross-dependencies. Everything is abstracted within the plugin and is transparent to the application. The application should not explicitly check for GPU HW, vendor, and driver version. The application should do everything the same regardless of sub-feature support and enablement. The only 2 exceptions are Reflex UI and Reflex Flash Indicator (RFI). The application must disable Reflex UI based on `sl::ReflexSettings::lowLatencyAvailable`. And the application must not send `sl::ReflexMarker::eReflexMarkerTriggerFlash` when `sl::ReflexSettings::flashIndicatorDriverControlled` is false.

### 1.0 INITIALIZE AND SHUTDOWN

Call `slInit` as early as possible (before any dxgi/d3d11/d3d12 APIs are invoked)

```cpp

#include <sl.h>
#include <sl_reflex.h>

sl::Preferences pref{};
pref.showConsole = true; // for debugging, set to false in production
pref.logLevel = sl::eLogLevelDefault;
pref.pathsToPlugins = {}; // change this if Streamline plugins are not located next to the executable
pref.numPathsToPlugins = 0; // change this if Streamline plugins are not located next to the executable
pref.pathToLogsAndData = {}; // change this to enable logging to a file
pref.logMessageCallback = myLogMessageCallback; // highly recommended to track warning/error messages in your callback
pref.applicationId = myId; // Provided by NVDA, required if using NGX components (DLSS 2/3)
pref.engineType = myEngine; // If using UE or Unity
pref.engineVersion = myEngineVersion; // Optional version
pref.projectId = myProjectId; // Optional project id
if(SL_FAILED(res, slInit(pref)))
{
    // Handle error, check the logs
    if(res == sl::Result::eErrorDriverOutOfDate) { /* inform user */}
    // and so on ...
}
```

For more details please see [preferences](ProgrammingGuide.md#221-preferences)

Call `slShutdown()` before destroying dxgi/d3d11/d3d12/vk instances, devices and other components in your engine.

```cpp
if(SL_FAILED(res, slShutdown()))
{
    // Handle error, check the logs
}
```

#### 1.1 SET THE CORRECT DEVICE

Once the main device is created call `slSetD3DDevice` or `slSetVulkanInfo`:

```cpp
if(SL_FAILED(res, slSetD3DDevice(nativeD3DDevice)))
{
    // Handle error, check the logs
}
```

### 2.0 CHECK IF REFLEX IS SUPPORTED

As soon as SL is initialized, you can check if Reflex is available for the specific adapter you want to use:

```cpp
Microsoft::WRL::ComPtr<IDXGIFactory> factory;
if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory)))
{
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter{};
    uint32_t i = 0;
    while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc{};
        if (SUCCEEDED(adapter->GetDesc(&desc)))
        {
            sl::AdapterInfo adapterInfo{};
            adapterInfo.deviceLUID = (uint8_t*)&desc.AdapterLuid;
            adapterInfo.deviceLUIDSizeInBytes = sizeof(LUID);
            if (SL_FAILED(result, slIsFeatureSupported(sl::kFeatureReflex, adapterInfo)))
            {
                // Requested feature is not supported on the system, fallback to the default method
                switch (result)
                {
                    case sl::Result::eErrorOSOutOfDate:         // inform user to update OS
                    case sl::Result::eErrorDriverOutOfDate:     // inform user to update driver
                    case sl::Result::eErrorNoSupportedAdapter:  // cannot use this adapter (older or non-NVDA GPU etc)
                    // and so on ...
                };
            }
            else
            {
                // Feature is supported on this adapter!
            }
        }
        i++;
    }
}
```

> **IMPORTANT:**
> The Reflex plugin includes two distinct functionalities: Reflex Low Latency and PC Latency (PCL) Stats. While the support for Reflex Low Latency is dependent on the GPU hardware, vendor, and driver version, **PCL Stats is supported on all GPU hardwares, vendors, and driver versions**. The plugin handles all such abstractions; As long as `sl::kFeatureReflex` is supported, the application should always make the same `slReflexSetMarker` and `slReflexSleep` calls without any explicit GPU hardware, vendor, and driver version checks.

### 3.0 CHECK REFLEX STATE AND CAPABILITIES

To find out what latency modes are available, currently active or to obtain the latest latency stats you can do the following:

```cpp

// Using helpers from sl_reflex.h

sl::ReflexState state{};
if(SL_FAILED(res, slReflexGetState(state)))
{
    // Handle error here, check the logs
}
if(state.lowLatencyAvailable)
{
    //
    // Reflex Low Latency is available, on NVDA hardware this would be done through Reflex.
    //
    // The application can show the Reflex Low Latency UI. (Otherwise hide/disable the UI.)
    // This is for UI only. Do everything else the same, even when this is false.
    //
}
if(state.flashIndicatorDriverControlled)
{
    //
    // Reflex Flash Indicator (RFI) is controlled by the driver. This means
    // the application should always check for left mouse button clicks and
    // send the trigger flash markers accordingly. The driver will decide
    // whether to show the RFI on screen based on user preference.
    //
}
```

### 4.0 SET REFLEX OPTIONS

To configure Reflex please do the following:

```cpp

// Using helpers from sl_reflex.h

sl::ReflexOptions reflexOptions = {};
reflexOptions.mode = eReflexModeLowLatency; // to disable Reflex simply set this to off
reflexOptions.frameLimitUs = myFrameLimit; // See docs for ReflexConstants
reflexOptions.useMarkersToOptimize = myUseMarkers; // See docs for ReflexConstants
if(SL_FAILED(res, slReflexSetOptions(reflexOptions)))
{
    // Handle error here, check the logs
}
```

> **NOTE:**
> To turn off Reflex set `sl::ReflexOptions.mode` to `sl::ReflexMode::eOff`. `slReflexSetMarker` must always be called even when Reflex low-latency mode is Off. These markers are also used by PCL Stats to measure latency.

> **NOTE:**
> `slReflexSetOptions` needs to be called at least once, even when Reflex Low Latency is Off and there is no Reflex UI. If options do not change there is no need to call this method every frame.

### 5.0 ADD SL REFLEX TO THE RENDERING PIPELINE

Call `slReflexSetMarker` or `slReflexSleep` at the appropriate location where Reflex markers need to be injected or where your application should sleep.

Here is some pseudo code:

```cpp

// Using helpers from sl_reflex.h

// Make sure Reflex is enabled (Reflex is ALWAYS available at least as latency tracker)
if(useReflex) 
{
    // Mark the section where specific activity is happening.
    //
    // Here for example we will make the simulation code.
    //
    
    // Starting new frane, grab handle from SL
    sl::FrameToken* currentFrame{};
    if(SL_FAILED(res, slGetNewFrameToken(&currentFrame))
    {
        // Handle error
    }

    // Simulation start
    if(SL_FAILED(res, slReflexSetMarker(sl::ReflexMarker::eSimulationStart, *currentFrame)))
    {
        // Handle error
    }

    // Simulation code goes here

    // Simulation end
    if(SL_FAILED(res, slReflexSetMarker(sl::ReflexMarker::eSimulationEnd, *currentFrame)))
    {
        // Handle error
    }   
}

// When checking for custom low latency messages inside the Windows message loop
//
if(useReflex && reflexState.statsWindowMessage == msgId) 
{
    // Reflex ping based on custom message

    // First scenario, using current frame
    if(SL_FAILED(res, slReflexSetMarker(sl::ReflexMarker::ePCLatencyPing, *currentFrame)))
    {
        // Handle error
    }
    
    // Second scenario, sending ping BEFORE simulation started, need to advance to the next frame
    sl::FrameToken* nextFrame{};
    auto nextIndex = *currentFrame + 1;
    if(SL_FAILED(res, slGetNewFrameToken(&nextFrame, &nextIndex))
    {
        // Handle error
    }
    if(SL_FAILED(res, slReflexSetMarker(sl::ReflexMarker::ePCLatencyPing, *nextFrame)))
    {
        // Handle error
    }
}

// When your application should sleep to achieve optimal low-latency mode
//
if(useReflex) 
{
    // Sleep
    if(SL_FAILED(res, slReflexSleep(*currentFrame)))
    {
        // Handle error
    }
}

```

### 6.0 PCL STATS

PCL Stats enables real time measurement of per-frame PC latency (PCL) during gameplay​. E2E system latency = PCL + peripheral latency + display latency​. And PCL = input sampling latency + simulation start to present + present to displayed.

Windows messages with ID == `sl::ReflexState::statsWindowMessage` are sent to the application periodically. The time this ping message is sent is recorded and is used to measure the latency between the simulated input and the application picking up the input. This is the input sampling latency.

Typically, the application's message pump would process keyboard and mouse input messages into a queue to be read in the upcoming frame. On seeing the ping message, the application must either call `slReflexSetMarker` for `sl::ReflexMarker::ePCLatencyPing` to send the ping marker right away, or put it in the queue, or set a flag for the next simulation to send the ping marker.

> **NOTE:**
> The exact timing of the ping marker does not matter. It is the frame index parameter that identify which frame picks up the simulated input. For example, since the frame index is usually incremented at simulation start, in the case of the ping marker being sent at message pump before simulation start, use current frame index +1.

### 7.0 HOW TO TRANSITION FROM NVAPI REFLEX TO SL REFLEX

Existing Reflex integrations can be easily converted to use SL Reflex by following these steps:

* Remove NVAPI from your application
* Remove `reflexstats.h` from your application
* There is no longer any need to provide a native D3D/VK device when making Reflex calls - SL takes care of that, hence making the integrations easier
* `NvAPI_D3D_SetSleepMode` is replaced with [set reflex options](#40-set-reflex-constants)
* `NvAPI_D3D_GetSleepStatus` is replaced with [get reflex state](#30-check-reflex-settings-and-capabilities) - see `sl::ReflexState::lowLatencyAvailable`
* `NvAPI_D3D_GetLatency` is replaced with [get reflex state](#30-check-reflex-settings-and-capabilities) - see `sl::ReflexReport`
* `NvAPI_D3D_SetLatencyMarker` is replaced with `slReflexSetMarker`
* `NvAPI_D3D_Sleep` is replaced with `slReflexSleep`
* `NVSTATS*` calls are handled automatically by SL Reflex plugin and are GPU agnostic
* `NVSTATS_IS_PING_MSG_ID` is replaced with [get reflex options](#30-check-reflex-settings-and-capabilities) - see `sl::ReflexOptions::statsWindowMessage`

### 8.0 NVIDIA REFLEX QA CHECKLIST

**Checklist**

Please use this checklist to confirm that your Reflex integration has been completed successfully. While this list is not comprehensive and does not replace rigorous testing, it should help to identify obvious issues.

Checklist Item | Pass/Fail
---|---
Reflex Low Latency’s default state is On |
All Reflex modes (Off, On, On + Boost) function correctly |
PC Latency (PCL) in the Reflex Test Utility is not 0.0 |
Reflex Test Utility Report passed without warning with DLSS Frame Generation |
Reflex Test Utility Report passed without warning with Reflex On |
Reflex does not significantly impact FPS (more than 4%) when Reflex is On <br> (On + Boost is expect to have some FPS hit for lowest latency) |
Reflex Test Utility Report passed without warning with Reflex On + Boost |
Reflex Markers are always sent regardless of Reflex Low Latency mode state |
Reflex Flash Indicator appears when left mouse button is pressed |
Reflex UI settings are following the UI Guidelines |
Keybinding menus work properly (no F13) |
PC Latency (PCL) is not 0.0 on other IHV Hardware |
Reflex UI settings are disabled or not available on other IHV Hardware |

**Steps**

1. Download/uncompress Reflex Verification tools
    * <https://developer.download.nvidia.com/assets/gamedev/files/ReflexVerification.zip>
2. Install FrameView SDK
    * Double click the FrameView SDK Installer (`FVSDKSetup.exe`)
    * Restart the system
3. Run `ReflexTestSetup.bat` from an administrator mode command prompt
    * This will force the Reflex Flash Indicator to enable, enable the Verification HUD, set up the Reflex Test framework, and start ReflexTest.exe.
4. Check Reflex Low Latency modes
    1. Run game
        * Make sure game is running in fullscreen exclusive
        * Make sure VSYNC is disabled
        * Make sure MSHybrid mode is not enabled
    2. Check Reflex Low Latency’s default state is On in UI and in the Verification HUD
        * Use the Reset / Default button in UI if it exists
    3. Cycle through the Reflex modes in UI and check that it matches with “Reflex Mode” in the Verification HUD
5. Running Reflex Tests
    * For titles with DLSS Frame Generation (FG), turn FG to On
        1. Press `Alt + t` in game to start the test (2 beeps)
        2. Analyze results after the test is done (3 beeps)
            * Test should take approximately 5 minutes
            * Check for warnings in the ReflexTest.exe output
    * Turn DLSS FG to Off, Reflex to On
        1. Press `Alt + t` in game to start the test (2 beeps)
        2. Analyze results after the test is done (3 beeps)
            * Test should take approximately 5 minutes
            * Check for warnings in the ReflexTest.exe output
    * Turn Reflex to On + Boost
        1. Press `Alt + t` in game to start the test (2 beeps)
        2. Analyze results after the test is done (3 beeps)
            * Test should take approximately 5 minutes
            * Check for warnings in the ReflexTest.exe output
    7. Press `Ctrl + c` in the command prompt to exit ReflexTest.exe
6. Test that markers are always sent even when Reflex is Off
    1. Turn Reflex to Off
    2. Look at the Verification HUD to ensure that the markers are still updating
7. Test the Reflex Flash Indicator
    1. Verify the Reflex Flash Indicator is showing
        * Notice the gray square that flashes when the left mouse button is pressed
        * Verify that Flash Indicator in the Verification HUD increments by 1 when the left mouse button is pressed
8. Check UI
    1. Verify UI follows the Guidelines
9. Check keybinding
    1. Run `capturePclEtw.bat` in administrator mode command prompt
    2. Go back to the game.  Start game play
    3. Check the Keybinding menu to make sure F13 is not being automatically applied when selecting a key
    4. Go back to the command prompt and press any key to exit the bat
10.  Run `ReflexTestCleanUp.bat` in administrator mode command prompt
    * This disables the Reflex Flash Indicator and the Reflex Test framework
11. Test on other IHV (if available)
    1. Install other IHV hardware
    2. Install FrameView SDK and restart the system
    3. Run `PrintPCL.exe` in administrator mode command prompt
    4. Run game
    5. Press `Alt + t` in game
        * Look at the PCL value in the command prompt. If the value is not 0.0, then PCL is working
    6. Press `Ctrl + c` in the command prompt to exit PrintPCL.exe
    7. Check to make sure Reflex UI is not available
12. Send NVIDIA Reflex Test report and Checklist results
    * Send to NVIDIA alias: reflex-sdk-support@nvidia.com

**ReflexTestResults.txt**

The Reflex Test Utility iterates through (up to) 10 FPS points, measuring PCL. Here is an example of the report summary:
```
*** Summary [647e5c14] -    Reflex ON,  Frame Gen x2,                                       I>S  S>Q  Q>R  R>FG FG>D 
 252.5 FPS: PCL  23.4 ms is PASS at 5.91 FT!  -0.4% FPS impact.  34.0% latency reduction. {0.96 1.42 1.95 0.43 0.66} 
 236.4 FPS: PCL  24.1 ms is PASS at 5.71 FT!  -0.5% FPS impact.  36.0% latency reduction. {0.91 1.33 1.93 0.42 0.62} 
 213.3 FPS: PCL  26.0 ms is PASS at 5.54 FT!   0.1% FPS impact.  39.7% latency reduction. {1.00 1.16 1.90 0.42 0.57} 
 197.2 FPS: PCL  27.3 ms is PASS at 5.39 FT!  -0.4% FPS impact.  40.3% latency reduction. {0.99 0.99 1.91 0.41 0.58} 
 177.4 FPS: PCL  28.9 ms is PASS at 5.13 FT!  -0.9% FPS impact.  42.8% latency reduction. {0.96 0.85 1.89 0.41 0.52} 
 160.5 FPS: PCL  32.6 ms is PASS at 5.24 FT!   0.2% FPS impact.  42.8% latency reduction. {1.11 0.74 1.95 0.41 0.53} 
 145.0 FPS: PCL  33.5 ms is PASS at 4.85 FT!  -0.1% FPS impact.  46.7% latency reduction. {0.97 0.70 1.84 0.41 0.43} 
 121.3 FPS: PCL  38.8 ms is PASS at 4.70 FT!  -1.2% FPS impact.  47.5% latency reduction. {1.10 0.47 1.87 0.40 0.36} 
  59.6 FPS: PCL  68.0 ms is PASS at 4.06 FT!  -0.4% FPS impact.  55.1% latency reduction. {0.93 0.21 1.78 0.40 0.23} 
  44.1 FPS: PCL  94.9 ms is PASS at 4.18 FT!  -1.3% FPS impact.  52.6% latency reduction. {1.29 0.16 1.73 0.40 0.16} 
```


Field | Description 
-|-
FPS | Average frames per second when Reflex On (+ Boost). 
PCL | Average PC latency when Reflex On (+ Boost). 
PASS/OKAY/WARN | This is a sanity check on PCL. WARN indicates a possible issue with the integration. 
FT | PCL expressed in average frame times. (E.g., 23.4 / 1000 * 252.5 = 5.91 FT) 
FPS impact | Average FPS difference between Reflex On (+ Boost) vs Off.  <br/>A negative value means Reflex lowers/worsens FPS. 
Latency reduction | Average PCL difference between Reflex On (+ Boost) vs Off.  <br/>A positive value means Reflex lowers/improves latency. 
I>S | Input to simulation start latency in average frame times. 
S>Q | Simulation start to queue start latency in average frame times. 
Q>R | Queue start to GPU end (or DLSS FG start) latency in average frame times. 
R>FG | DLSS FG start to GPU end latency in average frame times. 
R>D/FG>D | GPU end to displayed latency in average frame times. 


**Reflex Verification HUD**

The Reflex Verification HUD can be used to help validate your integration for correctness.
- ReflexTestEnable.exe 1 [xpos ypos]
  * Display the Reflex Verification HUD in your game. Optional arguments xpos and ypos specify the on-screen position of the HUD.
- ReflexTestEnable.exe 0
  * Hide the Reflex Verification HUD.

For the commands to take effect, a restart of the game is required.

> **NOTE:**
> Reflex Verification HUD requires 531.18 driver or newer.  
> If the Verification HUD does not appear, you will need to add the game to the app profile: NVIDIA Control Panel -> 3D Settings -> Program Settings -> Select Program to Customize -> Add -> Select a program -> Add Selected Program -> Apply.

Fields are as follows:
Field | Value | Details
------|-------|--------|
Reflex Mode | Off, enabled,<br/> enabled + boost |
App Called Sleep | 0/1 | Is the game calling NvAPI_D3D_Sleep / NvAPI_VK_Sleep <br/> function?
Flash Indicator | Counter | Number of flash trigger marks seen
Total Render Time | Duration | Time spent by GPU rendering
Simulation Interval | Duration | Simulation start time of frame X - simulation start time of <br/> frame X-1 <br/> All other timestamps are relative to simulation start time
Simulation End Time | Timestamp | Simulation end marker time
Render Start/End Time | Timestamp | Render submit start/end marker time
Present Start/End Time | Timestamp | Present start/end marker time
Driver Start/End Time | Timestamp | Start = first buffer submission <br/> End = last present submission
OS Queue Start/End Time | Timestamp | Start = first significant buffer submission <br/> End = GPU end time
GPU Start/End Time | Timestamp | Start = GPU rendering starts; End = GPU rendering ends

> **NOTE:**
> All durations and timestamps should be non-zero. Start timestamps must be less than end values (I.e., start must come before end).
