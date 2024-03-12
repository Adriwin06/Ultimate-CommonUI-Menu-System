# Ultimate UE5 CommonUI Full Game Menu System

### THIS IS STILL IN DEVELOPMENT AND IS ABSOLUTELY NOT FINISHED.

## Features 
  - Full WIP game menu system
  - Easy prompt system
  - VERY complete graphics settings, like there is probably too much settings for a normal user, with engine settings as well as Post Process Volume Settings
  - Save system that saves all settings when you quit/reopen the game. They now got reapplied automatically at launch and when openning a level.
  - Main menu with background map & camera movements
  - Pause menu (press ²)
  - Controller support (except in the "Options" menu but I'm working on it)
  - Partial French localization
  - ~~Every text is on a single string table so it's easier to edit it and to make localization (it's also faster)~~ [for the last commits I didn't take the time to actually add each text in the string table]

## Goal
The goal of this project is to make the ultimate game menu system that has everything you need, from a complete options menu (only graphics settings at the moment) that feature practically all settings that can be useful for normal and more advanced users including settings for common plugins/assets, to a beautiful main menu. And all of that free for everyone to use and/or contribute to save hours and hours of making a menu system that doesn't even necessarily feature as many options as this one. Everything is in Blueprint and is easily reusable/customizable whether you want to use it as it is or you want to build your own menu by using this one as a base. To make things even better, everyone can contribute, because the more people contribute, the better it will represent what the community wants.

## Requirements
There are a lot of plugins that I use in this project that are just for making my workflow easier that are absolutely not required. BUT the plugins that ARE used in the menu need to be downloaded in order for this to properly work. Here is a table of the required plugins: 
| Name | Description |
| ------------- | ------------- |
| [NVIDIA DLSS 3.5](https://developer.nvidia.com/rtx/dlss/get-started#ue-version)   |  NVIDIA DLSS 3.5 is a suite of AI rendering technologies powered by Tensor Cores on GeForce RTX GPUs for faster frame rates, better image quality, and great responsiveness.  |
| [AMD FidelityFX Super Resolution 3](https://gpuopen.com/learn/ue-fsr3/)  |  The AMD FidelityFX Super Resolution 3 (FSR 3) plugin for Unreal Engine provides an open source, high-quality solution for producing high resolution frames from lower resolution inputs and a frame interpolation technique which can increase the frame rate up to twice the input rate to improve smoothness of animations and frame pacing.  |
| [Intel Xe SUper Sampling](https://github.com/GameTechDev/XeSSUnrealPlugin)  |  Intel XeSS enables an innovative framerate boosting technology supported by Intel Arc graphics cards and other GPU vendors. Using AI deep-learning to perform upscaling, XeSS offers higher framerates without degrading the image quality.  |

## Access the Menu
The main menu is launched automatically.
Press "²" in-game to access the pause menu while in third person.  
"Tab" bring the old menu before I switch to CommonUI.
"Num 0" bring a menu originally by Unreal Bench https://www.unreal-bench.com/unreal-engine but I modified the two menu and mixed them. 

## Conditions:
  - You can use this freely in you project
  - You have to credit me

## Known Bugs
Here are the main ones:
  - Bloom Size Scale, Lumen GI Scene Lighting Update Speed, Lumen GI Final Gather Lighting Update Speed, Exposure EV100 Min, Exposure EV100 Max, AO Power, AO Quality, and all Clouds settings are not functioning. For all settings except the clouds, the issue stems from these variables not being assigned to a pin in a "Make PostProcessSettings" node, because the pin doesn't show up, despite checking the "Show pin" box. As for the clouds settings, I have not yet investigated why they are not working.

## To do
  - Make the Tab List View working
  - Add other settings like controls, display, audio, color blind mode, etc
  - Fix all bugs

### To maybe do in C++
  - The CommonBoundActionBar actions, instead of hidden action button in the widget that got displayed in this bar.
  - Update the style of buttons in the CommonBoundActionBar of the "Options" menu when switching between controller and mouse & keyboard inputs, avoiding the use of two separate bars with different styles that are hidden based on the input type.
  - The prompt system.
