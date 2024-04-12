# Ultimate UE5 CommonUI Full Game Menu System

### THIS IS STILL IN DEVELOPMENT AND IS ABSOLUTELY NOT FINISHED.

## Features 
  - Full WIP game menu system
  - Easy prompt system
  - VERY complete "Options" menu, like there is probably too much settings for a normal user, with engine settings, Post Process Volume Settings, display settings, TSR/DLSS/FSR/XeSS settings, Audio/Volume settings (Music, Ambient, SFX, Voice, Controller Speaker, etc), input mapping, etc
  - WIP details panel that displays information about the hovered setting. It's not finished because there is a lot of information to add for each setting, so it takes quite a lot of time
  - Save system that saves all settings when you quit/reopen the game. They now got reapplied automatically at launch and when openning a level
  - Main menu with background map & camera movements
  - Beautiful animated character selection menu
  - Pause menu (press ² or escape if in standalone mode)
  - Full controller support (navigate between widget, change settings, etc)
  - Partial French localization
  - ~~Every text is on a single string table so it's easier to edit it and to make localization (it's also faster)~~ [for the last commits I didn't take the time to actually add each text in the string table]
  - Practically no canvas panel is used, so the performances are pretty good since canvas panels have high performance demands
  - Easy integration in your own project. For more informations, check out the [Wiki](https://github.com/Adriwin06/Ultimate-UE5-CommonUI-Full-Game-Menu-System/wiki/Integrate-to-your-own-project)

## Goal
The goal of this project is to make the ultimate game menu system that has everything you need, from a complete options menu (only graphics settings at the moment) that feature practically all settings that can be useful for normal and more advanced users including settings for common plugins/assets, to a beautiful main menu. And all of that free for everyone to use and/or contribute to save hours and hours of making a menu system that doesn't even necessarily feature as many options as this one. Everything is in Blueprint and is easily reusable/customizable whether you want to use it as it is or you want to build your own menu by using this one as a base. You can check out the WIP [Wiki](https://github.com/Adriwin06/Ultimate-UE5-CommonUI-Full-Game-Menu-System/wiki) for more informations. To make things even better, everyone can contribute, because the more people contribute, the better it will represent what the community wants. I will then create a branch with all community changes.

## Requirements
The plugins that are used in the menu need to be installed in order for this to properly work. Fortunately, they are now included in the project files. Here is a table of these plugins: 
| Name | Description |
| ------------- | ------------- |
| [NVIDIA DLSS 3.5](https://developer.nvidia.com/rtx/dlss/get-started#ue-version)   |  NVIDIA DLSS 3.5 is a suite of AI rendering technologies powered by Tensor Cores on GeForce RTX GPUs for faster frame rates, better image quality, and great responsiveness.  |
| [AMD FidelityFX Super Resolution 3](https://gpuopen.com/learn/ue-fsr3/)  |  The AMD FidelityFX Super Resolution 3 (FSR 3) plugin for Unreal Engine provides an open source, high-quality solution for producing high resolution frames from lower resolution inputs and a frame interpolation technique which can increase the frame rate up to twice the input rate to improve smoothness of animations and frame pacing.  |
| [Intel Xe Super Sampling](https://github.com/GameTechDev/XeSSUnrealPlugin)  |  Intel XeSS enables an innovative framerate boosting technology supported by Intel Arc graphics cards and other GPU vendors. Using AI deep-learning to perform upscaling, XeSS offers higher framerates without degrading the image quality.  |
| [Async Loading Screen](https://github.com/truong-bui/AsyncLoadingScreen)  |  Async Loading Screen allows you to easily configure a Loading Screen System in the project settings, and automatically add a Loading Screen whenever you open a new level. Async Loading Screen also comes with pre-design UI layouts and default icons that make it easy to custom your loading screen in a few minutes.  |

## Access the Menu
  - Main Menu: The main menu is launched automatically.
  - Pause Menu: Press "²", "Escape" or "Start"/"Options" on a controller in-game to access the pause menu.

## Conditions
  - You can use this freely in you project
  - You have to credit me

## Recommandations
I recommend launching the game in "Standalone Game" mode instead of "Selected Viewport" or "New Editor Window (PIE)" mode. This way, if it crashes, it won't crash the whole engine, and you can change resolution settings, fullscreen, etc, and actually see the changes. And it's better because it's closer to what players will experience.

## Known Bugs
  - Bloom Size Scale, Lumen GI Scene Lighting Update Speed, Lumen GI Final Gather Lighting Update Speed, Exposure EV100 Min, Exposure EV100 Max, AO Power, AO Quality, and all Clouds settings are not functioning. For all settings except the clouds, the issue stems from these variables not being assigned to a pin in a "Make PostProcessSettings" node, because the pin doesn't show up, despite checking the "Show pin" box. As for the clouds settings, I have not yet investigated why they are not working.
  - Controller navigation is still far from perfect inside the option menu. Sometimes you can't navigate all the way down or up, and occasionally the focus exits the settings panel and goes to the top settings tab list.

## To do
  - Add other settings like audio device, audio mode, color blind mode, etc.
  - Add Enhanced Input system (for scrolling in the menu with left thumbstick on a controller & more)
  - Fix all bugs.

### To maybe do in C++
  - CommonBoundActionBar actions, instead of hidden action button in the widget that got displayed in this bar.
  - Update the style of buttons in the CommonBoundActionBar of the "Options" menu when switching between controller and mouse & keyboard inputs, avoiding the use of two separate bars with different styles that are hidden based on the input type.
  - Prompt system.
  - Tab List View automation to eliminate the need for separate button blueprints for each tab button.
