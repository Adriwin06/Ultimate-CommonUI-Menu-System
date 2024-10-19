# UCMS - Ultimate CommonUI Menu System

<img src="https://i.imgur.com/uzxaYqd.png" width=49%> <img src="https://i.imgur.com/afaZQmT.png" width=49%>

## Features 
  - Full game menu system
  - Easy prompt system
  - VERY complete "Options" menu, like there is probably too much settings for a normal user, with engine settings, Post Process Volume Settings, display settings, TSR/DLSS/FSR/XeSS settings that follows their UI Guidelines, Audio/Volume settings (Music, Ambient, SFX, Voice, Controller Speaker, etc), input mapping, etc
  - Details panel that displays information about the hovered setting. It's not finished because there is a lot of information to add for each setting, so it takes quite a lot of time
  - Save system that saves all settings when you quit/reopen the game. They now got reapplied automatically at launch and when openning a level
  - Main menu with background map & camera movements
  - Beautiful animated character selection menu
  - Pause menu (press ² or escape if in standalone mode)
  - Full controller support (navigate between widget, change settings, etc)
  - Keyboard arrows navigation (validate with space)
  - Simple notification system
  - Partial French localization
  - ~~Every text is on a single string table so it's easier to edit it and to make localization (it's also faster)~~ [for the last commits I didn't take the time to actually add each text in the string table]
  - Practically no canvas panel is used, so the performances are pretty good since canvas panels have high performance demands
  - Easy integration in your own project. For more informations, check out the [Wiki](https://github.com/Adriwin06/Ultimate-UE5-CommonUI-Full-Game-Menu-System/wiki/Integrate-to-your-own-project)

## Try it yourself
I highly recommend trying it yourself. I regularly publish releases with packaged versions so you don't need to have Unreal Engine installed to try it. Just download, extract, and launch the .exe. I'm also working on integrating it into the City Sample to showcase its capabilities in a real game. Additionally, I'm adding more gameplay features so it's not boring, and you can truly experience the Matrix. 

You can already experience this City Sample build in it current stage by clicking on the [Google Drive link](https://drive.google.com/file/d/1oqVVWh9xc0bn1s7C-WmTVzkSu3oSzeKz) in the [latest Release](https://github.com/Adriwin06/Ultimate-UE5-CommonUI-Full-Game-Menu-System/releases).

## Goal
The goal of this project is to make the ultimate game menu system that has everything you need, from a complete options menu (only graphics settings at the moment) that feature practically all settings that can be useful for normal and more advanced users including settings for common plugins/assets, to a beautiful main menu. And all of that free for everyone to use and/or contribute to save hours and hours of making a menu system that doesn't even necessarily feature as many options as this one. Everything is in Blueprint and is easily reusable/customizable whether you want to use it as it is or you want to build your own menu by using this one as a base. You can check out the WIP [Wiki](https://github.com/Adriwin06/Ultimate-UE5-CommonUI-Full-Game-Menu-System/wiki) for more informations. To make things even better, everyone can contribute, because the more people contribute, the better it will represent what the community wants. I will then create a branch with all community changes.

## Branches
There are currently 4 different branches:

- [NvRTX](https://github.com/Adriwin06/Ultimate-CommonUI-Menu-System/tree/NvRTX): This branch has the most settings since it includes many settings from the [NvRTX branch](https://developer.nvidia.com/game-engines/unreal-engine/rtx-branch) of Unreal Engine.
- [main](https://github.com/Adriwin06/Ultimate-CommonUI-Menu-System/tree/main): This is the default branch and the one you should use if your game is not using the [NvRTX Branch](https://github.com/Adriwin06/Ultimate-CommonUI-Menu-System/tree/NvRTX).
- [Lyra](https://github.com/Adriwin06/Ultimate-CommonUI-Menu-System/tree/Lyra): This branch make the menu look like Lyra, and even better. Big thanks to [ArbyKay](https://github.com/ArbyKay) for this.
- [Enhanced Inputs](https://github.com/Adriwin06/Ultimate-CommonUI-Menu-System/tree/Enhanced-Inputs): This branch is experimental, and updates are very rare, so it will almost always be outdated compared to the other branches. Here, I experiment and try to make Enhanced Inputs work with CommonUI.

## Plugins Used
If you have a missing plugins error, it's probably because I forgot to disable the plugins that I use to developp this project and makes my worflow easier, but they are absolutely not requiered.
The plugins that are used in the menu need to be installed in order for this to properly work. Fortunately, they are now included in the project files.  Here is a table of these plugins: 
| Name | Description |
| ------------- | ------------- |
| [NVIDIA DLSS 3.7](https://developer.nvidia.com/rtx/dlss/get-started#ue-version)   |  NVIDIA DLSS 3.7 is a suite of AI rendering technologies powered by Tensor Cores on GeForce RTX GPUs for faster frame rates, better image quality, and great responsiveness.  |
| [AMD FidelityFX Super Resolution 3](https://gpuopen.com/learn/ue-fsr3/)  |  The AMD FidelityFX Super Resolution 3 (FSR 3) plugin for Unreal Engine provides an open source, high-quality solution for producing high resolution frames from lower resolution inputs and a frame interpolation technique which can increase the frame rate up to twice the input rate to improve smoothness of animations and frame pacing.  |
| [Intel Xe Super Sampling](https://github.com/GameTechDev/XeSSUnrealPlugin)  |  Intel XeSS enables an innovative framerate boosting technology supported by Intel Arc graphics cards and other GPU vendors. Using AI deep-learning to perform upscaling, XeSS offers higher framerates without degrading the image quality.  |
| [Async Loading Screen](https://github.com/truong-bui/AsyncLoadingScreen)  |  Async Loading Screen allows you to easily configure a Loading Screen System in the project settings, and automatically add a Loading Screen whenever you open a new level. Async Loading Screen also comes with pre-design UI layouts and default icons that make it easy to custom your loading screen in a few minutes.  |

This was external plugins, but I use other plugins integrated in the Engine:
  - Common UI
  - Cine Camera Rigs
  - Steam Controller

## Access the Menu
  - Main Menu: The main menu is launched automatically.
  - Pause Menu: Press "²", "Escape" or "Start"/"Options" on a controller in-game to access the pause menu.

## Conditions of Use
  - You can use this freely in your projects
  - You have to credit me:
    - It can be "UCMS Base UI" or anything you want but you need to add at least the name of this project.

## Recommendations
I recommend launching the game in "Standalone Game" mode instead of "Selected Viewport" or "New Editor Window (PIE)" mode. This way, if it crashes, it won't crash the whole engine, and you can change resolution settings, fullscreen, etc, and actually see the changes. And it's better because it's closer to what players will experience.

## Known Bugs
  - Bloom Size Scale, Lumen GI Scene Lighting Update Speed, Lumen GI Final Gather Lighting Update Speed, Exposure EV100 Min, Exposure EV100 Max, AO Power and AO Quality settings are not working. The issue stems from these variables not being assigned to a pin in a "Make PostProcessSettings" node, because the pin doesn't show up, despite checking the "Show pin" box. It seems like it is an Engine bug.

## To do
  - Enhance Enhanced Input system implementation (Available in experimental version in [this branch](https://github.com/Adriwin06/Ultimate-UE5-CommonUI-Full-Game-Menu-System/tree/Enhanced-Inputs))
  - Add comparaison images for each settings, and custom description widgets when it's needed
  - Fix all bugs.

### To maybe do in C++
  - CommonBoundActionBar actions, instead of hidden action button in the widget that got displayed in this bar.
  - Update the style of buttons in the CommonBoundActionBar of the "Options" menu when switching between controller and mouse & keyboard inputs, avoiding the use of two separate bars with different styles that are hidden based on the input type.
  - Prompt system.
  - Tab List View automation to eliminate the need for separate button blueprints for each tab button.
