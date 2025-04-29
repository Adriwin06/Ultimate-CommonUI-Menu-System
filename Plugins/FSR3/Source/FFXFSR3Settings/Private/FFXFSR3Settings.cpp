// This file is part of the FidelityFX Super Resolution 3.1 Unreal Engine Plugin.
//
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "FFXFSR3Settings.h"

#include "Misc/EngineVersionComparison.h"
// Variant of UE_VERSION_NEWER_THAN that is true if the engine version is at or later than the specified, used to better handle version differences in the codebase.
#define UE_VERSION_AT_LEAST(MajorVersion, MinorVersion, PatchVersion)	\
	UE_GREATER_SORT(ENGINE_MAJOR_VERSION, MajorVersion, UE_GREATER_SORT(ENGINE_MINOR_VERSION, MinorVersion, UE_GREATER_SORT(ENGINE_PATCH_VERSION, PatchVersion, true)))

#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"
#if UE_VERSION_AT_LEAST(5, 0, 0)
#include "Misc/ConfigCacheIni.h"
#endif
#if UE_VERSION_AT_LEAST(5, 1, 0)
#include "Misc/ConfigUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "FFXFSR3Module"

IMPLEMENT_MODULE(FFXFSR3SettingsModule, FFXFSR3Settings)

//------------------------------------------------------------------------------------------------------
// Console variables that control how FSR3 operates.
//------------------------------------------------------------------------------------------------------
TAutoConsoleVariable<int32> CVarEnableFSR3(
	TEXT("r.FidelityFX.FSR3.Enabled"),
	1,
	TEXT("Enable FidelityFX Super Resolution for Temporal Upscale"),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarEnableFSR3InEditor(
	TEXT("r.FidelityFX.FSR3.EnabledInEditorViewport"),
	0,
	TEXT("Enable FidelityFX Super Resolution for Temporal Upscale in the Editor viewport by default."),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarFSR3AdjustMipBias(
	TEXT("r.FidelityFX.FSR3.AdjustMipBias"),
	1,
	TEXT("Allow FSR3 to adjust the minimum global texture mip bias (r.ViewTextureMipBias.Min & r.ViewTextureMipBias.Offset)"),
	ECVF_ReadOnly);

TAutoConsoleVariable<int32> CVarFSR3ForceVertexDeformationOutputsVelocity(
	TEXT("r.FidelityFX.FSR3.ForceVertexDeformationOutputsVelocity"),
	1,
	TEXT("Allow FSR3 to enable r.Velocity.EnableVertexDeformation to ensure that materials that use World-Position-Offset render valid velocities."),
	ECVF_ReadOnly);

TAutoConsoleVariable<int32> CVarFSR3ForceLandscapeHISMMobility(
	TEXT("r.FidelityFX.FSR3.ForceLandscapeHISMMobility"),
	0,
	TEXT("Allow FSR3 to force the mobility of Landscape actors Hierarchical Instance Static Mesh components that use World-Position-Offset materials so they render valid velocities.\nSetting 1/'All Instances' is faster on the CPU, 2/'Instances with World-Position-Offset' is faster on the GPU."),
	ECVF_ReadOnly);

TAutoConsoleVariable<float> CVarFSR3Sharpness(
	TEXT("r.FidelityFX.FSR3.Sharpness"),
	0.0f,
	TEXT("Range from 0.0 to 1.0, when greater than 0 this enables Robust Contrast Adaptive Sharpening Filter to sharpen the output image. Default is 0."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3AutoExposure(
	TEXT("r.FidelityFX.FSR3.AutoExposure"),
	0,
	TEXT("True to use FSR3's own auto-exposure, otherwise the engine's auto-exposure value is used."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3HistoryFormat(
	TEXT("r.FidelityFX.FSR3.HistoryFormat"),
	0,
	TEXT("Selects the bit-depth for the FS32 history texture format, defaults to PF_FloatRGBA but can be set to PF_FloatR11G11B10 to reduce bandwidth at the expense of quality.\n")
	TEXT(" 0 - PF_FloatRGBA\n")
	TEXT(" 1 - PF_FloatR11G11B10\n"),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarFSR3CreateReactiveMask(
	TEXT("r.FidelityFX.FSR3.CreateReactiveMask"),
	1,
	TEXT("Enable to generate a mask from the SceneColor, GBuffer & ScreenspaceReflections that determines how reactive each pixel should be. Defaults to 1 (Enabled)."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskReflectionScale(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskReflectionScale"),
	0.4f,
	TEXT("Range from 0.0 to 1.0 (Default 0.4), scales the Unreal engine reflection contribution to the reactive mask, which can be used to control the amount of aliasing on reflective surfaces."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskRoughnessScale(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskRoughnessScale"),
	0.15f,
	TEXT("Range from 0.0 to 1.0 (Default 0.15), scales the GBuffer roughness to provide a fallback value for the reactive mask when screenspace & planar reflections are disabled or don't affect a pixel."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskRoughnessBias(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskRoughnessBias"),
	0.25f,
	TEXT("Range from 0.0 to 1.0 (Default 0.25), biases the reactive mask value when screenspace/planar reflections are weak with the GBuffer roughness to account for reflection environment captures."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskRoughnessMaxDistance(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskRoughnessMaxDistance"),
	6000.0f,
	TEXT("Maximum distance in world units for using material roughness to contribute to the reactive mask, the maximum of this value and View.FurthestReflectionCaptureDistance will be used. Default is 6000.0."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3ReactiveMaskRoughnessForceMaxDistance(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskRoughnessForceMaxDistance"),
	0,
	TEXT("Enable to force the maximum distance in world units for using material roughness to contribute to the reactive mask rather than using View.FurthestReflectionCaptureDistance. Defaults to 0."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskReflectionLumaBias(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskReflectionLumaBias"),
	0.0f,
	TEXT("Range from 0.0 to 1.0 (Default: 0.0), biases the reactive mask by the luminance of the reflection. Use to balance aliasing against ghosting on brightly lit reflective surfaces."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveHistoryTranslucencyBias(
	TEXT("r.FidelityFX.FSR3.ReactiveHistoryTranslucencyBias"),
	0.5f,
	TEXT("Range from 0.0 to 1.0 (Default: 1.0), scales how much translucency suppresses history via the reactive mask. Higher values will make translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveHistoryTranslucencyLumaBias(
	TEXT("r.FidelityFX.FSR3.ReactiveHistoryTranslucencyLumaBias"),
	0.0f,
	TEXT("Range from 0.0 to 1.0 (Default 0.0), biases how much the translucency suppresses history via the reactive mask by the luminance of the transparency. Higher values will make bright translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskTranslucencyBias(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskTranslucencyBias"),
	1.0f,
	TEXT("Range from 0.0 to 1.0 (Default: 1.0), scales how much contribution translucency makes to the reactive mask. Higher values will make translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskTranslucencyLumaBias(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskTranslucencyLumaBias"),
	0.0f,
	TEXT("Range from 0.0 to 1.0 (Default 0.0), biases the translucency contribution to the reactive mask by the luminance of the transparency. Higher values will make bright translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskTranslucencyMaxDistance(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskTranslucencyMaxDistance"),
	500000.0f,
	TEXT("Maximum distance in world units for using translucency to contribute to the reactive mask. This is a way to remove sky-boxes and other back-planes from the reactive mask, at the expense of nearer translucency not being reactive. Default is 500000.0."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskForceReactiveMaterialValue(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskForceReactiveMaterialValue"),
	0.0f,
	TEXT("Force the reactive mask value for Reactive Shading Model materials, when > 0 this value can be used to override the value supplied in the Material Graph. Default is 0 (Off)."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3ReactiveMaskReactiveShadingModelID(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskReactiveShadingModelID"),
	MSM_NUM,
	TEXT("Treat the specified shading model as reactive, taking the CustomData0.x value as the reactive value to write into the mask. Default is MSM_NUM (Off)."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3UseExperimentalSSRDenoiser(
	TEXT("r.FidelityFX.FSR3.UseSSRExperimentalDenoiser"),
	0,
	TEXT("Set to 1 to use r.SSR.ExperimentalDenoiser when FSR3 is enabled. This is required when r.FidelityFX.FSR3.CreateReactiveMask is enabled as the FSR3 plugin sets r.SSR.ExperimentalDenoiser to 1 in order to capture reflection data to generate the reactive mask. Default is 0."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3DeDitherMode(
	TEXT("r.FidelityFX.FSR3.DeDither"),
	2,
	TEXT("Adds an extra pass to de-dither and avoid dithered/thin appearance. Default is 0 - Off. \n")
	TEXT(" 0 - Off. \n")
	TEXT(" 1 - Full. Attempts to de-dither the whole scene. \n")
	TEXT(" 2 - Hair only. Will only de-dither around Hair shading model pixels - requires the Deferred Renderer. \n"),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3QualityMode(
	TEXT("r.FidelityFX.FSR3.QualityMode"),
	1,
	TEXT("FSR3 Mode [0-4].  Lower values yield superior images.  Higher values yield improved performance.  Default is 1 - Quality.\n")
	TEXT(" 0 - Native AA			1.0x \n")
	TEXT(" 1 - Quality				1.5x \n")
	TEXT(" 2 - Balanced				1.7x \n")
	TEXT(" 3 - Performance			2.0x \n")
	TEXT(" 4 - Ultra Performance	3.0x \n"),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3QuantizeInternalTextures(
	TEXT("r.FidelityFX.FSR3.QuantizeInternalTextures"),
	0,
	TEXT("Setting this to 1 will round up the size of some internal texture to ensure a specific divisibility. Default is 0."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskPreDOFTranslucencyScale(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskPreDOFTranslucencyScale"),
	0.8f,
	TEXT("Range from 0.0 to 1.0 (Default 0.8), scales how much contribution pre-Depth-of-Field translucency color makes to the reactive mask. Higher values will make translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3ReactiveMaskPreDOFTranslucencyMax(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskPreDOFTranslucencyMax"),
	0,
	TEXT("Toggle to determine whether to use the max(SceneColorPostDepthOfField - SceneColorPreDepthOfField) or length(SceneColorPostDepthOfField - SceneColorPreDepthOfField) to determine the contribution of Pre-Depth-of-Field translucency. Default is 0."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskCustomStencilScale(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskCustomStencilScale"),
	0.0f,
	TEXT("Range from 0.0 to 1.0 (Default 0.0), scales how much customm stencil values contribute to the reactive mask. Higher values will make translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveHistoryCustomStencilScale(
	TEXT("r.FidelityFX.FSR3.ReactiveHistoryCustomStencilScale"),
	0.0f,
	TEXT("Range from 0.0 to 1.0 (Default 0.0), scales how much customm stencil values contribute to supressing hitory. Higher values will make translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskDeferredDecalScale(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskDeferredDecalScale"),
	0.0f,
	TEXT("Range from 0.0 to 1.0 (Default 0.0), scales how much deferred decal values contribute to the reactive mask. Higher values will make translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveHistoryDeferredDecalScale(
	TEXT("r.FidelityFX.FSR3.ReactiveHistoryDeferredDecalScale"),
	0.0f,
	TEXT("Range from 0.0 to 1.0 (Default 0.0), scales how much deferred decal values contribute to supressing hitory. Higher values will make translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3CustomStencilMask(
	TEXT("r.FidelityFX.FSR3.CustomStencilMask"),
	0,
	TEXT("A bitmask 0-255 (0-0xff) used when accessing the custom stencil to read reactive mask values. Setting to 0 will disable use of the custom-depth/stencil buffer. Default is 0."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3CustomStencilShift(
	TEXT("r.FidelityFX.FSR3.CustomStencilShift"),
	0,
	TEXT("Bitshift to apply to the value read from the custom stencil when using it to provide reactive mask values. Default is 0."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveMaskTAAResponsiveValue(
	TEXT("r.FidelityFX.FSR3.ReactiveMaskTAAResponsiveValue"),
	0.0f,
	TEXT("Range from 0.0 to 1.0 (Default 0.0), value to write to reactive mask when pixels are marked in the stencil buffer as TAA Responsive. Higher values will make translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3ReactiveHistoryTAAResponsiveValue(
	TEXT("r.FidelityFX.FSR3.ReactiveHistoryTAAResponsiveValue"),
	0.0f,
	TEXT("Range from 0.0 to 1.0 (Default 0.0), value to write to reactive history when pixels are marked in the stencil buffer as TAA Responsive. Higher values will make translucent materials more reactive which can reduce smearing."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<float> CVarFSR3VelocityFactor(
	TEXT("r.FidelityFX.FSR3.VelocityFactor"),
	1.0f,
	TEXT("Range from 0.0 to 1.0 (Default 1.0), value of 0.0f can improve temporal stability of bright pixels."),
	ECVF_RenderThreadSafe
);

TAutoConsoleVariable<int32> CVarFSR3DeferDelete(
	TEXT("r.FidelityFX.FSR3.DeferDelete"),
	0,
	TEXT("Number of frames to defer deletion - defaults to 0 which relies on the RHI to ensure resources aren't released while in use on the GPU."),
	ECVF_RenderThreadSafe
);

//------------------------------------------------------------------------------------------------------
// Console variables for Frame Interpolation.
//------------------------------------------------------------------------------------------------------
TAutoConsoleVariable<int32> CVarEnableFFXFI(
	TEXT("r.FidelityFX.FI.Enabled"),
	1,
	TEXT("Enable FidelityFX Frame Interpolation"),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarFFXFICaptureDebugUI(
	TEXT("r.FidelityFX.FI.CaptureDebugUI"),
	!UE_BUILD_SHIPPING,
	TEXT("Force FidelityFX Frame Interpolation to detect and copy any debug UI which only renders on the first invocation of Slate's DrawWindow command."),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarFFXFIUpdateGlobalFrameTime(
	TEXT("r.FidelityFX.FI.UpdateGlobalFrameTime"),
	0,
	TEXT("Update the GAverageMS and GAverageFPS engine globals with the frame time & FPS including frame interpolation."),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarFFXFIModifySlateDeltaTime(
	TEXT("r.FidelityFX.FI.ModifySlateDeltaTime"),
	1,
	TEXT("Set the FSlateApplication delta time to 0.0 when redrawing the UI for the 'Slate Redraw' UI mode to prevent widgets' NativeTick implementations updating incorrectly, ignored when using 'UI Extraction'."),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarFFXFIUIMode(
	TEXT("r.FidelityFX.FI.UIMode"),
	0,
	TEXT("The method to render the UI when using Frame Generation.\n")
	TEXT("- Slate Redraw (0): will cause Slate to render the UI on to both the real & generated images each frame, this is higher quality but requires UI elements to be able to render multiple times per game frame.\n")
	TEXT("- UI Extraction (1): will compare the pre- & post- UI frame to extract the UI and copy it on to the generated frame, this might result in lower quality for translucent UI elements but doesn't require re-rendering UI elements."),
	ECVF_ReadOnly);

TAutoConsoleVariable<int32> CVarFFXFIUseDistortionTexture(
	TEXT("r.FidelityFX.FI.UseDistortionTexture"),
	0,
	TEXT("Set to 1 to bind the UE distortion texture to the Frame Interpolation context to better interpolate distortion, set to 0 to ignore distortion (Default: 0).\n"),
	ECVF_RenderThreadSafe);

#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT || UE_BUILD_TEST)
TAutoConsoleVariable<int32> CVarFFXFIShowDebugTearLines(
	TEXT("r.FidelityFX.FI.ShowDebugTearLines"),
	0,
	TEXT("Show the debug tear lines when running Frame Interpolation."),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarFFXFIShowDebugView(
	TEXT("r.FidelityFX.FI.ShowDebugView"),
	0,
	TEXT("Show the debug view when running Frame Interpolation."),
	ECVF_RenderThreadSafe);
#endif

//-------------------------------------------------------------------------------------
// Console variables for the RHI backend.
//-------------------------------------------------------------------------------------
TAutoConsoleVariable<int32> CVarFSR3UseRHI(
	TEXT("r.FidelityFX.FSR3.UseRHI"),
	0,
	TEXT("True to enable FSR3's default RHI backend, false to disable in which case a native backend must be enabled. Default is 0."),
	ECVF_ReadOnly
);

TAutoConsoleVariable<int32> CVarFSR3PaceRHIFrames(
	TEXT("r.FidelityFX.FI.RHIPacingMode"),
	0,
	TEXT("Enable pacing frames when using the RHI backend.\n")
	TEXT("- None (0) : No frame pacing - default.\n")
	TEXT("- Custom Present VSync (1) : enable VSync for the second presented frame, tearing will only affect the interpolated frame which will be held on screen for at least one VBlank but interferes with presentation state which may not always work."),
	ECVF_RenderThreadSafe
);

//-------------------------------------------------------------------------------------
// Console variables for the D3D12 backend.
//-------------------------------------------------------------------------------------
TAutoConsoleVariable<int32> CVarFSR3UseNativeDX12(
	TEXT("r.FidelityFX.FSR3.UseNativeDX12"),
	1,
	TEXT("True to use FSR3's native & optimised D3D12 backend, false to use the fallback implementation based on Unreal's RHI. Default is 1."),
	ECVF_ReadOnly
);

TAutoConsoleVariable<int32> CVarFSR3OverrideSwapChainDX12(
	TEXT("r.FidelityFX.FI.OverrideSwapChainDX12"),
	1,
	TEXT("True to use FSR3's D3D12 swap-chain override that improves frame pacing, false to use the fallback implementation based on Unreal's RHI. Default is 1."),
	ECVF_ReadOnly
);

TAutoConsoleVariable<int32> CVarFSR3AllowAsyncWorkloads(
	TEXT("r.FidelityFX.FI.AllowAsyncWorkloads"),
	0,
	TEXT("True to use async. execution of Frame Interpolation, 0 to run Frame Interpolation synchronously with the game. Default is 0."),
	ECVF_ReadOnly
);

//-------------------------------------------------------------------------------------
// Console variables for older builds
//-------------------------------------------------------------------------------------
#if UE_VERSION_OLDER_THAN(5, 1, 0)
TAutoConsoleVariable<float> CVarHDRMinLuminanceLog10(
	TEXT("r.HDR.Display.MinLuminanceLog10"),
	0,
	TEXT("Min luminance in nits log10."),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarHDRMaxLuminance(
	TEXT("r.HDR.Display.MaxLuminance"),
	0,
	TEXT("Max luminance in nits."),
	ECVF_RenderThreadSafe);
#endif

//-------------------------------------------------------------------------------------
// FFXFSR3SettingsModule
//-------------------------------------------------------------------------------------
void FFXFSR3SettingsModule::StartupModule()
{
#if UE_VERSION_AT_LEAST(5, 1, 0)
	UE::ConfigUtilities::ApplyCVarSettingsFromIni(TEXT("/Script/FFXFSR3Settings.FFXFSR3Settings"), *GEngineIni, ECVF_SetByProjectSetting);
#else
	ApplyCVarSettingsFromIni(TEXT("/Script/FFXFSR3Settings.FFXFSR3Settings"), *GEngineIni, ECVF_SetByProjectSetting);
#endif
}

void FFXFSR3SettingsModule::ShutdownModule()
{
}

//-------------------------------------------------------------------------------------
// UFFXFSR3Settings
//-------------------------------------------------------------------------------------
UFFXFSR3Settings::UFFXFSR3Settings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FName UFFXFSR3Settings::GetContainerName() const
{
	static const FName ContainerName("Project");
	return ContainerName;
}

FName UFFXFSR3Settings::GetCategoryName() const
{
	static const FName EditorCategoryName("Plugins");
	return EditorCategoryName;
}

FName UFFXFSR3Settings::GetSectionName() const
{
	static const FName EditorSectionName("FSR3");
	return EditorSectionName;
}

void UFFXFSR3Settings::PostInitProperties()
{
	Super::PostInitProperties(); 

#if WITH_EDITOR
	if(IsTemplate())
	{
		ImportConsoleVariableValues();
	}
#endif
}

#if WITH_EDITOR


void UFFXFSR3Settings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(PropertyChangedEvent.Property)
	{
		ExportValuesToConsoleVariables(PropertyChangedEvent.Property);
	}
}

#endif

#undef LOCTEXT_NAMESPACE