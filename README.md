# This Readme is not finished.
## THIS IS STILL IN DEVELOPEMENT AND IS ABSOLUTELY NOT FINISHED.

# Features 
  -  Full WIP Game menu system
  -  Easy prompt system
  -  VERY complete graphics settings, like there is probably too much settings for a normal user, with engine setings as well as Post Process Volume Settings
  -  Main Menu with background map & camera movements
  - Partial French Localization
  - Every text is on a single string table so it's easier to edit it and to make localization (it's also faster)

# Goal
The goal of this project is to make the Ultimate Game Menu system that has everything you need, from a complete settings (only graphics at the moment) that feature practically all settings that can be useful for normal and more advanced users including settings for common plugins/assets, to beautiful main menu. And all of that free for eveyone to use and/or contribute to save hours and hours of making a menu system that doesn't even necessarily feature as much option as this one. Everything is in Blueprint and is easily reusable/customizable whether you want to use it as it is or you want to build you own menu by using this one as a base. To make things even better, everyone can contribute; just contact me on Discord @adriwin78 before so we can talk and I can make you a contributor.

# Requierments
There are a lot of plugin that I use in this project that are just for making my workflow easier that are absolutely not requiered. BUT the plugins that ARE used in the menu need to be downloaded in order for this to properly work. Here is a table of the requiered Plugins : 
| Name | Description |
| ------------- | ------------- |
| [NVIDIA DLSS 3.5](https://developer.nvidia.com/rtx/dlss/get-started#ue-version)   |  NVIDIA DLSS 3.5 is a suite of AI rendering technologies powered by Tensor Cores on GeForce RTX GPUs for faster frame rates, better image quality, and great responsiveness.  |
| [AMD FidelityFX Super Resolution 3](https://gpuopen.com/learn/ue-fsr3/)  |  The AMD FidelityFX Super Resolution 3 (FSR 3) plugin for Unreal Engine provides an open source, high-quality solution for producing high resolution frames from lower resolution inputs and a frame interpolation technique which can increase the frame rate up to twice the input rate to improve smoothness of animations and frame pacing.  |
| [Intel Xe SUper Sampling](https://github.com/GameTechDev/XeSSUnrealPlugin)  |  Intel XeSS enables an innovative framerate boosting technology supported by Intel Arc graphics cards and other GPU vendors. Using AI deep-learning to perform upscaling, XeSS offers higher framerates without degrading the image quality.  |

# Access the Menu
Press ² In-Game to access the absolutely not finished pause menu while in third person.  
Tab bring the old menu before I switch to CommonUI.
Num 0 bring a menu originally by Unreal Bench https://www.unreal-bench.com/unreal-engine but I modified the two menu and mixed them. 

### What follow next in the old Readme that isn't finished

# Conditions : 
  - You can use this freely in you project
  - You have to credit me
  - You can't claim that it's your work

## Known Bugs : 
  - There is a lot of bugs I can't write everything here but:
  - Settings are not saved
  - Settings got reapplied when oppenning the settings menu
  - Prompt doesn't work in the pause menu

## To do :
  - Store the menu variable in a Gamesave Blueprint. When the save button will be clicked, it will copy all usefull variable value in it and when the menu is oppened, it will restore them from that Gamesave BP. I'm not sure that's the best way of doing it but I will try and exeperiment on it and see where it go.
  - Add other settings like controls, audio, color blind, etc
  - Fix all bugs
