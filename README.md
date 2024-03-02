## THIS IS STILL IN DEVELOPEMENT AND IS ABSOLUTELY NOT FINISHED.

# Features 
  -  Full WIP Game menu system
  -  Easy prompt system
  -  VERY complete graphics settings, like there is probably too much settings for a normal user, with engine setings as well as Post Process Volume Settings
  -  Save System that save all settings accross levels and when you quit/reopen the game
  -  Main Menu with background map & camera movements
  -  Partial French Localization
  -  Every text is on a single string table so it's easier to edit it and to make localization (it's also faster)

# Goal
The goal of this project is to make the Ultimate Game Menu system that has everything you need, from a complete options menu (only graphics settings at the moment) that feature practically all settings that can be useful for normal and more advanced users including settings for common plugins/assets, to a beautiful main menu. And all of that free for eveyone to use and/or contribute to save hours and hours of making a menu system that doesn't even necessarily feature as much option as this one. Everything is in Blueprint and is easily reusable/customizable whether you want to use it as it is or you want to build you own menu by using this one as a base. To make things even better, everyone can contribute, because the more people contribute, the better it will represent what the community wants.

# Requirements
There are a lot of plugin that I use in this project that are just for making my workflow easier that are absolutely not requiered. BUT the plugins that ARE used in the menu need to be downloaded in order for this to properly work. Here is a table of the requiered Plugins : 
| Name | Description |
| ------------- | ------------- |
| [NVIDIA DLSS 3.5](https://developer.nvidia.com/rtx/dlss/get-started#ue-version)   |  NVIDIA DLSS 3.5 is a suite of AI rendering technologies powered by Tensor Cores on GeForce RTX GPUs for faster frame rates, better image quality, and great responsiveness.  |
| [AMD FidelityFX Super Resolution 3](https://gpuopen.com/learn/ue-fsr3/)  |  The AMD FidelityFX Super Resolution 3 (FSR 3) plugin for Unreal Engine provides an open source, high-quality solution for producing high resolution frames from lower resolution inputs and a frame interpolation technique which can increase the frame rate up to twice the input rate to improve smoothness of animations and frame pacing.  |
| [Intel Xe SUper Sampling](https://github.com/GameTechDev/XeSSUnrealPlugin)  |  Intel XeSS enables an innovative framerate boosting technology supported by Intel Arc graphics cards and other GPU vendors. Using AI deep-learning to perform upscaling, XeSS offers higher framerates without degrading the image quality.  |

# Access the Menu
The Main Menu is launched automatically.
Press ² In-Game to access the pause menu while in third person.  
Tab bring the old menu before I switch to CommonUI.
Num 0 bring a menu originally by Unreal Bench https://www.unreal-bench.com/unreal-engine but I modified the two menu and mixed them. 

# Conditions : 
  - You can use this freely in you project
  - You have to credit me

## Known Bugs : 
There is a lot of bugs I can't write everything here but:
  - To reapply your settings, you need to open the "Options" menu in the Main Menu
  - Prompt doesn't work in the pause menu

## To do :
  - Make a better Common Bound Action Bar with Apply, Reset and Cancel settings
  - Add other settings like controls, audio, color blind mode, etc
  - Fix all bugs
