/*
* Copyright (c) 2020 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/
#pragma once

#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/DeveloperSettings.h"
#include "SceneView.h"

#include "DLSSSettings.generated.h"

UENUM()
enum class EDLSSSettingOverride : uint8
{
	Enabled UMETA(DisplayName = "True"),
	Disabled UMETA(DisplayName = "False"),
	UseProjectSettings UMETA(DisplayName = "Use project settings"),
};

UENUM(BlueprintType)
enum class EDLSSPreset : uint8
{
	Default=0 UMETA(ToolTip = "default behavior, preset specified per DLSS SDK release"),
	A=1  UMETA(ToolTip = "Force preset A, For Perf/Balanced/Quality modes, An older variant best suited to combat ghosting for elements with missing inputs (such as motion vectors)", DisplayName = "Preset A"),
	B=2  UMETA(ToolTip = "Force preset B, For Ultra Perf mode, Similar to Preset A but for Ultra Performance mode", DisplayName = "Preset B"),
	C=3  UMETA(ToolTip = "Force preset C, For Perf/Balanced/Quality modes, Preset which generally favors current frame information. Generally well-suited for fast-paced game content", DisplayName = "Preset C"),
	D=4  UMETA(ToolTip = "Force preset D, For Perf/Balanced/Quality modes, Similar to Preset E. Preset E is generally recommended over Preset D.", DisplayName = "Preset D"),
	E=5  UMETA(ToolTip = "Force preset E, For Perf/Balanced/Quality modes, The default preset for Perf/Balanced/Quality modes. Generally recommended preset for most performance and image stability.", DisplayName = "Preset E"),
	F=6  UMETA(ToolTip = "Force preset F, For Ultra Perf/DLAA modes, The default preset for Ultra Perf and DLAA modes.", DisplayName = "Preset F"),
	G=7  UMETA(ToolTip = "Force preset G, Do not use – reverts to default behavior", Hidden),
	H=8  UMETA(ToolTip = "Force preset H, Do not use – reverts to default behavior", Hidden),
	I=9  UMETA(ToolTip = "Force preset I, Do not use – reverts to default behavior", Hidden),
	J=10 UMETA(ToolTip = "Force preset J, Similar to preset K. Preset J might exhibit slightly less ghosting at the cost of extra flickering. Preset K is generally recommended over preset J", DisplayName = "Preset J"),
	K=11 UMETA(ToolTip = "Force preset K, Default preset for DLAA/Perf/Balanced/Quality modes that is transformer based. Best image quality preset at a higher performance cost.",DisplayName= "Preset K"),
	L=12 UMETA(ToolTip = "Force preset L, Do not use - Reverts to default behavior", Hidden),
	M=13 UMETA(ToolTip = "Force preset M, Do not use - Reverts to default behavior", Hidden),
	N=14 UMETA(ToolTip = "Force preset N, Do not use - Reverts to default behavior", Hidden),
	O=15 UMETA(ToolTip = "Force preset O, Do not use - Reverts to default behavior", Hidden),

	//Please add new presets above this line.
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EDLSSRRPreset : uint8
{
	Default=0 UMETA(ToolTip="Default behavior, may or may not change after OTA"),
	A =  1 UMETA(ToolTip = "Force preset A",Hidden),
	B =  2 UMETA(ToolTip = "Force preset B",Hidden),
	C =  3 UMETA(ToolTip = "Force preset C",Hidden),
	D =  4 UMETA(ToolTip = "Force preset D, Default model (transformer)",DisplayName = "Preset D"),
	E =  5 UMETA(ToolTip = "Force preset E, Latest transformer model (must use if DoF guide is needed)",DisplayName = "Preset E"),
	F =  6 UMETA(ToolTip = "Force preset F, Do not use – reverts to default behavior", Hidden),
	G =  7 UMETA(ToolTip = "Force preset G, Do not use – reverts to default behavior", Hidden),
	H =  8 UMETA(ToolTip = "Force preset H, Do not use – reverts to default behavior", Hidden),
	I =  9 UMETA(ToolTip = "Force preset I, Do not use – reverts to default behavior", Hidden),
	J = 10 UMETA(ToolTip = "Force preset J, Do not use – reverts to default behavior", Hidden),
	K = 11 UMETA(ToolTip = "Force preset K, Do not use – reverts to default behavior", Hidden),
	L = 12 UMETA(ToolTip = "Force preset L, Do not use – reverts to default behavior", Hidden),
	M = 13 UMETA(ToolTip = "Force preset M, Do not use – reverts to default behavior", Hidden),
	N = 14 UMETA(ToolTip = "Force preset N, Do not use – reverts to default behavior", Hidden),
	O = 15 UMETA(ToolTip = "Force preset O, Do not use – reverts to default behavior", Hidden),

	//Please add new presets above this line.
	MAX UMETA(Hidden)
};

UCLASS(Config = Engine, ProjectUserConfig)
class DLSS_API UDLSSOverrideSettings : public UObject
{
public:

	GENERATED_BODY()
	
	/** This enables DLSS/DLAA in editor viewports. Saved to local user config only.*/
	UPROPERTY(Config, EditAnywhere, Category = "Level Editor - Viewport (Local)", DisplayName = "Enable DLSS/DLAA to be turned on in Editor viewports")
	EDLSSSettingOverride EnableDLSSInEditorViewportsOverride = EDLSSSettingOverride::UseProjectSettings;

	/** This enables DLSS/DLAA in play in editor viewports. Saved to local user config only. */
	UPROPERTY(Config, EditAnywhere, Category = "Level Editor - Viewport (Local)", DisplayName = "Enable DLSS/DLAA in Play In Editor viewports")
	EDLSSSettingOverride EnableDLSSInPlayInEditorViewportsOverride = EDLSSSettingOverride::UseProjectSettings;

	/** This enables warnings about plugins & tools that are incompatible with DLSS/DLAA in the editor. This setting and the project setting both must be set to get warnings */
	UPROPERTY(Config, EditAnywhere, Category = "Editor (Local)", DisplayName = "Warn about incompatible plugins and tools")
	bool bShowDLSSIncompatiblePluginsToolsWarnings = true;

	/** This enables on screen warnings and errors about DLSS/DLAA  */
	UPROPERTY(Config, EditAnywhere, Category = "Editor (Local)", DisplayName = "Show various DLSS/DLAA on screen debug messages")
	EDLSSSettingOverride ShowDLSSSDebugOnScreenMessages = EDLSSSettingOverride::UseProjectSettings;

};

UCLASS(Config = Engine, DefaultConfig, DisplayName="NVIDIA DLSS")
class DLSS_API UDLSSSettings: public UObject
{
	GENERATED_BODY()

private:

public:

	/** Enable DLSS/DLAA for D3D12, if the driver supports it at runtime */
	UPROPERTY(Config, EditAnywhere, Category = "Platforms", DisplayName = "Enable DLSS/DLAA for the D3D12RHI")
		bool bEnableDLSSD3D12 = PLATFORM_WINDOWS;

	/** Enable DLSS/DLAA for D3D11, if the driver supports it at runtime */
	UPROPERTY(Config, EditAnywhere, Category = "Platforms", DisplayName = "Enable DLSS/DLAA for the D3D11RHI")
		bool bEnableDLSSD3D11 = PLATFORM_WINDOWS;

	/** Enable DLSS/DLAA for Vulkan, if the driver supports it at runtime */
	UPROPERTY(Config, EditAnywhere, Category = "Platforms", DisplayName = "Enable DLSS/DLAA for the VulkanRHI")
		bool bEnableDLSSVulkan = PLATFORM_WINDOWS;

	/** This enables DLSS/DLAA in editor viewports. This project wide setting can be locally overridden in the NVIDIA DLSS (Local) settings.*/
	UPROPERTY(Config, EditAnywhere, Category = "Level Editor - Viewport", DisplayName = "Enable DLSS/DLAA to be turned on in Editor viewports")
		bool bEnableDLSSInEditorViewports = false;

	/** This enables DLSS/DLAA in play in editor viewports. This project wide setting can be locally overridden in in the NVIDIA DLSS (Local) settings.*/
	UPROPERTY(Config, EditAnywhere, Category = "Level Editor - Viewport", DisplayName = "Enable DLSS/DLAA in Play In Editor viewports")
		bool bEnableDLSSInPlayInEditorViewports = true;

	/** This enables on screen warnings and errors about DLSS/DLAA. This project wide setting can be locally overridden in the NVIDIA DLSS (Local) settings. */
	UPROPERTY(Config, EditAnywhere, Category = "Level Editor - Viewport", DisplayName = "Show various DLSS/DLAA on screen debug messages")
		bool bShowDLSSSDebugOnScreenMessages = true;

	/** This is part of the DLSS plugin and used by most projects*/
	UPROPERTY(VisibleAnywhere, Config, Category = "General Settings", DisplayName = "Generic DLSS Binary Path")
		FString GenericDLSSBinaryPath;

	UPROPERTY(VisibleAnywhere, Config, Category = "General Settings", DisplayName = "Exists")
		bool bGenericDLSSBinaryExists;

	/** By default the DLSS plugin uses the UE Project ID to initialize DLSS. In some cases NVIDIA might provide a separate NVIDIA Application ID, which should be put here. Please refer to https://developer.nvidia.com/dlss for details*/
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "NVIDIA NGX Application ID", AdvancedDisplay)
		uint32 NVIDIANGXApplicationId;

	/** In some cases NVIDIA might provide a project specific DLSS binary for your project. Please refer to https://developer.nvidia.com/dlss for details*/
	UPROPERTY(VisibleAnywhere, Config, Category = "General Settings", DisplayName = "Custom DLSS Binary Path", AdvancedDisplay)
		FString CustomDLSSBinaryPath;
	UPROPERTY(VisibleAnywhere, Config, Category = "General Settings", DisplayName = "Exists", AdvancedDisplay)
		bool bCustomDLSSBinaryExists;

	/** Allow OTA updates of DLSS models */
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "Allow OTA update")
		bool bAllowOTAUpdate = true;

	/** This enables warnings about plugins & tools that are incompatible with DLSS/DLAA in the editor. This setting and the local setting both must be set to get warnings */
	UPROPERTY(Config, EditAnywhere, Category = "Editor", DisplayName = "Warn about incompatible plugins and tools")
	bool bShowDLSSIncompatiblePluginsToolsWarnings = true;

	/** DLAA preset setting. Allows selecting a different DL model than the default */
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLAA Preset", AdvancedDisplay)
		EDLSSPreset DLAAPreset = EDLSSPreset::Default;

	/** DLSS quality mode preset setting. Allows selecting a different DL model than the default */
	// NOT IMPLEMENTED YET UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLSS Ultra Quality Preset", AdvancedDisplay)
		EDLSSPreset DLSSUltraQualityPreset = EDLSSPreset::Default;

	/** DLSS quality mode preset setting. Allows selecting a different DL model than the default */
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLSS Quality Preset", AdvancedDisplay)
		EDLSSPreset DLSSQualityPreset = EDLSSPreset::Default;

	/** DLSS balanced mode preset setting. Allows selecting a different DL model than the default */
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLSS Balanced Preset", AdvancedDisplay)
		EDLSSPreset DLSSBalancedPreset = EDLSSPreset::Default;

	/** DLSS performance mode preset setting. Allows selecting a different DL model than the default */
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLSS Performance Preset", AdvancedDisplay)
		EDLSSPreset DLSSPerformancePreset = EDLSSPreset::Default;

	/** DLSS ultra performance mode preset setting. Allows selecting a different DL model than the default */
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLSS Ultra Performance Preset", AdvancedDisplay)
		EDLSSPreset DLSSUltraPerformancePreset = EDLSSPreset::Default;

	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLAA-RR Preset", AdvancedDisplay)
		EDLSSRRPreset DLAARRPreset = EDLSSRRPreset::Default;

	/** DLSS-RR quality mode preset setting. Allows selecting a different DL model than the default */
	// NOT IMPLEMENTED YET UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLSS Ultra Quality Preset", AdvancedDisplay)
		EDLSSRRPreset DLSSRRUltraQualityPreset = EDLSSRRPreset::Default;

	/** DLSS-RR quality mode preset setting. Allows selecting a different DL model than the default */
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLSS-RR Quality Preset", AdvancedDisplay)
		EDLSSRRPreset DLSSRRQualityPreset = EDLSSRRPreset::Default;

	/** DLSS-RR balanced mode preset setting. Allows selecting a different DL model than the default */
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLSS-RR Balanced Preset", AdvancedDisplay)
		EDLSSRRPreset DLSSRRBalancedPreset = EDLSSRRPreset::Default;

	/** DLSS-RR performance mode preset setting. Allows selecting a different DL model than the default */
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLSS-RR Performance Preset", AdvancedDisplay)
		EDLSSRRPreset DLSSRRPerformancePreset = EDLSSRRPreset::Default;

	/** DLSS-RR ultra performance mode preset setting. Allows selecting a different DL model than the default */
	UPROPERTY(Config, EditAnywhere, Category = "General Settings", DisplayName = "DLSS-RR Ultra Performance Preset", AdvancedDisplay)
		EDLSSRRPreset DLSSRRUltraPerformancePreset = EDLSSRRPreset::Default;

public:

	virtual void PostInitProperties();

};

