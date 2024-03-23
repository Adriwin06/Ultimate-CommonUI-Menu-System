// This file is part of the FidelityFX Super Resolution 3.0 Unreal Engine Plugin.
//
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "Modules/ModuleManager.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/DeveloperSettings.h"
#include "HAL/IConsoleManager.h"
#include "Engine/EngineTypes.h"

#include "FFXFSR3Settings.generated.h"

//-------------------------------------------------------------------------------------
// The official FSR3 quality modes.
//-------------------------------------------------------------------------------------
UENUM()
enum class EFFXFSR3QualityMode : int32
{
	NativeAA UMETA(DisplayName = "Native AA"),
	Quality UMETA(DisplayName = "Quality"),
	Balanced UMETA(DisplayName = "Balanced"),
	Performance UMETA(DisplayName = "Performance"),
	UltraPerformance UMETA(DisplayName = "Ultra Performance"),
};

//-------------------------------------------------------------------------------------
// The support texture formats for the FSR3 history data.
//-------------------------------------------------------------------------------------
UENUM()
enum class EFFXFSR3HistoryFormat : int32
{
	FloatRGBA UMETA(DisplayName = "PF_FloatRGBA"),
	FloatR11G11B10 UMETA(DisplayName = "PF_FloatR11G11B10"),
};

//-------------------------------------------------------------------------------------
// The support texture formats for the FSR3 history data.
//-------------------------------------------------------------------------------------
UENUM()
enum class EFFXFSR3DeDitherMode : int32
{
	Off UMETA(DisplayName = "Off"),
	Full UMETA(DisplayName = "Full"),
	HairOnly UMETA(DisplayName = "Hair Only"),
};

//-------------------------------------------------------------------------------------
// The modes for forcing Landscape Hierachical Instance Static Model to not be Static.
//-------------------------------------------------------------------------------------
UENUM()
enum class EFFXFSR3LandscapeHISMMode : int32
{
	Off UMETA(DisplayName = "Off"),
	AllStatic UMETA(DisplayName = "All Instances"),
	StaticWPO UMETA(DisplayName = "Instances with World-Position-Offset"),
};

//------------------------------------------------------------------------------------------------------
// Console variables that control how FSR3 operates.
//------------------------------------------------------------------------------------------------------
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3Sharpness;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3AutoExposure;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3HistoryFormat;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3CreateReactiveMask;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveMaskReflectionScale;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveMaskRoughnessScale;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveMaskRoughnessBias;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveMaskRoughnessMaxDistance;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3ReactiveMaskRoughnessForceMaxDistance;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveMaskReflectionLumaBias;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveHistoryTranslucencyBias;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveHistoryTranslucencyLumaBias;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveMaskTranslucencyBias;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveMaskTranslucencyLumaBias;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveMaskTranslucencyMaxDistance;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<float> CVarFSR3ReactiveMaskForceReactiveMaterialValue;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3ReactiveMaskReactiveShadingModelID;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3UseExperimentalSSRDenoiser;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3DeDitherMode;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3QualityMode;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3QuantizeInternalTextures;

//------------------------------------------------------------------------------------------------------
// Console variables for Frame Interpolation.
//------------------------------------------------------------------------------------------------------
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarEnableFFXFI;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFFXFICaptureDebugUI;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFFXFIUpdateGlobalFrameTime;
#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFFXFIShowDebugTearLines;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFFXFIShowDebugView;
#endif

//-------------------------------------------------------------------------------------
// Console variables for the RHI backend.
//-------------------------------------------------------------------------------------
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3UseRHI;

//-------------------------------------------------------------------------------------
// Console variables for the D3D12 backend.
//-------------------------------------------------------------------------------------
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3UseNativeDX12;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3OverrideSwapChainDX12;
extern FFXFSR3SETTINGS_API TAutoConsoleVariable<int32> CVarFSR3AllowAsyncWorkloads;

//-------------------------------------------------------------------------------------
// Settings for FSR3 exposed through the Editor UI.
//-------------------------------------------------------------------------------------
UCLASS(Config = Engine, DefaultConfig, DisplayName = "FidelityFX Super Resolution 3.0")
class FFXFSR3SETTINGS_API UFFXFSR3Settings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()
public:
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

	virtual void PostInitProperties() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.Enabled", DisplayName = "Enabled"))
		bool bEnabled;

	UPROPERTY(Config, EditAnywhere, Category = "General Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.AutoExposure", DisplayName = "Auto Exposure", ToolTip = "Enable to use FSR3's own auto-exposure, otherwise the engine's auto-exposure value is used."))
		bool bAutoExposure;

	UPROPERTY(Config, EditAnywhere, Category = "General Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.EnabledInEditorViewport", DisplayName = "Enabled in Editor Viewport", ToolTip = "When enabled use FSR3 by default in the Editor viewports."))
		bool bEnabledInEditorViewport;

	UPROPERTY(Config, EditAnywhere, Category = "General Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.UseSSRExperimentalDenoiser", DisplayName = "Use SSR Experimental Denoiser", ToolTip = "Set to 1 to use r.SSR.ExperimentalDenoiser when FSR3 is enabled. This is required when r.FidelityFX.FSR3.CreateReactiveMask is enabled as the FSR3 plugin sets r.SSR.ExperimentalDenoiser to 1 in order to capture reflection data to generate the reactive mask."))
		bool bUseSSRExperimentalDenoiser;

	UPROPERTY(Config, EditAnywhere, Category = "Backend Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.UseRHI", DisplayName = "RHI Backend", ToolTip = "True to enable FSR3's default RHI backend, when false a native backend must be enabled."))
		bool bRHIBackend;

	UPROPERTY(Config, EditAnywhere, Category = "Backend Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.UseNativeDX12", DisplayName = "D3D12 Backend", ToolTip = "True to use FSR3's native & optimised D3D12 backend, when false the RHI backend must be enabled."))
		bool bD3D12Backend;

	UPROPERTY(Config, EditAnywhere, Category = "Frame Generation Settings", meta = (ConsoleVariable = "r.FidelityFX.FI.Enabled", DisplayName = "Frame Generation Enabled"))
		bool bFrameGenEnabled;

	UPROPERTY(Config, EditAnywhere, Category = "Frame Generation Settings", meta = (ConsoleVariable = "r.FidelityFX.FI.CaptureDebugUI", DisplayName = "Capture Debug UI", ToolTip = "Force FidelityFX Frame Generation to detect and copy any debug UI which only renders on the first invocation of Slate's DrawWindow command."))
		bool bCaptureDebugUI;

	UPROPERTY(Config, EditAnywhere, Category = "Frame Generation Settings", meta = (ConsoleVariable = "r.FidelityFX.FI.UpdateGlobalFrameTime", DisplayName = "Update Global Frame Time", ToolTip = "Update the GAverageMS and GAverageFPS engine globals with the frame time & FPS including frame generation."))
		bool bUpdateGlobalFrameTime;

	UPROPERTY(Config, EditAnywhere, Category = "Frame Generation Settings", meta = (ConsoleVariable = "r.FidelityFX.FI.AllowAsyncWorkloads", DisplayName = "D3D12 Async. Interpolation", ToolTip = "True to use async. execution of Frame Interpolation, false to run Frame Interpolation synchronously with the game."))
		bool bD3D12AsyncInterpolation;

	UPROPERTY(Config, EditAnywhere, Category = "Frame Generation Settings", meta = (ConsoleVariable = "r.FidelityFX.FI.OverrideSwapChainDX12", DisplayName = "D3D12 Async. Present", ToolTip = "True to use FSR3's D3D12 swap-chain override that improves frame pacing, false to use the fallback implementation based on Unreal's RHI."))
		bool bD3D12AsyncPresent;

	UPROPERTY(Config, EditAnywhere, Category = "Quality Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.QualityMode", DisplayName = "Quality Mode", ToolTip = "Selects the default quality mode to be used when upscaling with FSR3."))
		EFFXFSR3QualityMode QualityMode;

	UPROPERTY(Config, EditAnywhere, Category = "Quality Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.HistoryFormat", DisplayName = "History Format", ToolTip = "Selects the bit-depth for the FSR3 history texture format, defaults to PF_FloatRGBA but can be set to PF_FloatR11G11B10 to reduce bandwidth at the expense of quality."))
		EFFXFSR3HistoryFormat HistoryFormat;

	UPROPERTY(Config, EditAnywhere, Category = "Quality Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.DeDither", DisplayName = "De-Dither Rendering", ToolTip = "Enable an extra pass to de-dither rendering before handing over to FSR3 to avoid over-thinning, defaults to Off but can be set to Full for all pixels or to Hair Only for just around Hair (requires Deffered Renderer)."))
		EFFXFSR3DeDitherMode DeDither;

	UPROPERTY(Config, EditAnywhere, Category = "Quality Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.Sharpness", DisplayName = "Sharpness", ClampMin = 0, ClampMax = 1, ToolTip = "When greater than 0.0 this enables Robust Contrast Adaptive Sharpening Filter to sharpen the output image."))
		float Sharpness;

	UPROPERTY(Config, EditAnywhere, Category = "Quality Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.AdjustMipBias", DisplayName = "Adjust Mip Bias & Offset", ToolTip = "Applies negative MipBias to material textures, improving results."))
		bool bAdjustMipBias;

	UPROPERTY(Config, EditAnywhere, Category = "Quality Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ForceVertexDeformationOutputsVelocity", DisplayName = "Force Vertex Deformation To Output Velocity", ToolTip = "Force enables materials with World Position Offset and/or World Displacement to output velocities during velocity pass even when the actor has not moved."))
		bool bForceVertexDeformationOutputsVelocity;

	UPROPERTY(Config, EditAnywhere, Category = "Quality Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ForceLandscapeHISMMobility", DisplayName = "Force Landscape HISM Mobility", ToolTip = "Allow FSR3 to force the mobility of Landscape actors Hierarchical Instance Static Mesh components that use World-Position-Offset materials so they render valid velocities.\nSetting 'All Instances' is faster on the CPU, 'Instances with World-Position-Offset' is faster on the GPU."))
		EFFXFSR3LandscapeHISMMode ForceLandscapeHISMMobility;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.CreateReactiveMask", DisplayName = "Reactive Mask", ToolTip = "Enable to generate a mask from the SceneColor, GBuffer, SeparateTranslucency & ScreenspaceReflections that determines how reactive each pixel should be."))
		bool bReactiveMask;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskReflectionScale", DisplayName = "Reflection Scale", ClampMin = 0, ClampMax = 1, ToolTip = "Scales the Unreal engine reflection contribution to the reactive mask, which can be used to control the amount of aliasing on reflective surfaces."))
		float ReflectionScale;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskReflectionLumaBias", DisplayName = "Reflection Luminance Bias", ClampMin = 0, ClampMax = 1, ToolTip = "Biases the reactive mask by the luminance of the reflection. Use to balance aliasing against ghosting on brightly lit reflective surfaces."))
		float ReflectionLuminanceBias;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskRoughnessScale", DisplayName = "Roughness Scale", ClampMin = 0, ClampMax = 1, ToolTip = "Scales the GBuffer roughness to provide a fallback value for the reactive mask when screenspace & planar reflections are disabled or don't affect a pixel."))
		float RoughnessScale;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskRoughnessBias", DisplayName = "Roughness Bias", ClampMin = 0, ClampMax = 1, ToolTip = "Biases the reactive mask value when screenspace/planar reflections are weak with the GBuffer roughness to account for reflection environment captures."))
		float RoughnessBias;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskRoughnessMaxDistance", DisplayName = "Roughness Max Distance", ClampMin = 0, ToolTip = "Maximum distance in world units for using material roughness to contribute to the reactive mask, the maximum of this value and View.FurthestReflectionCaptureDistance will be used."))
		float RoughnessMaxDistance;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskRoughnessForceMaxDistance", DisplayName = "Force Roughness Max Distance", ToolTip = "Enable to force the maximum distance in world units for using material roughness to contribute to the reactive mask rather than using View.FurthestReflectionCaptureDistance."))
		bool bReactiveMaskRoughnessForceMaxDistance;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskTranslucencyBias", DisplayName = "Translucency Bias", ClampMin = 0, ClampMax = 1, ToolTip = "Scales how much contribution translucency makes to the reactive mask. Higher values will make translucent materials less reactive which can reduce smearing."))
		float TranslucencyBias;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskTranslucencyLumaBias", DisplayName = "Translucency Luminance Bias", ClampMin = 0, ClampMax = 1, ToolTip = "Biases the translucency contribution to the reactive mask by the luminance of the transparency."))
		float TranslucencyLuminanceBias;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskTranslucencyMaxDistance", DisplayName = "Translucency Max Distance", ClampMin = 0, ToolTip = "Maximum distance in world units for using translucency to contribute to the reactive mask. This is another way to remove sky-boxes and other back-planes from the reactive mask, at the expense of nearer translucency not being reactive."))
		float TranslucencyMaxDistance;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskReactiveShadingModelID", DisplayName = "Reactive Shading Model", ToolTip = "Treat the specified shading model as reactive, taking the CustomData0.x value as the reactive value to write into the mask. Default is MSM_NUM (Off)."))
		TEnumAsByte<enum EMaterialShadingModel> ReactiveShadingModelID;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveMaskForceReactiveMaterialValue", DisplayName = "Force value for Reactive Shading Model", ClampMin = 0, ClampMax = 1, ToolTip = "Force the reactive mask value for Reactive Shading Model materials, when > 0 this value can be used to override the value supplied in the Material Graph."))
		float ForceReactiveMaterialValue;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveHistoryTranslucencyBias", DisplayName = "Translucency Bias", ClampMin = 0, ClampMax = 1, ToolTip = "Scales how much contribution translucency makes to suppress history via the reactive mask. Higher values will make translucent materials less reactive which can reduce smearing."))
		float ReactiveHistoryTranslucencyBias;

	UPROPERTY(Config, EditAnywhere, Category = "Reactive Mask Settings", meta = (ConsoleVariable = "r.FidelityFX.FSR3.ReactiveHistoryTranslucencyLumaBias", DisplayName = "Translucency Luminance Bias", ClampMin = 0, ClampMax = 1, ToolTip = "Biases the translucency contribution to suppress history via the reactive mask by the luminance of the transparency."))
		float ReactiveHistoryTranslucencyLumaBias;
};

class FFXFSR3SettingsModule final : public IModuleInterface
{
public:
	// IModuleInterface implementation
	void StartupModule() override;
	void ShutdownModule() override;
};