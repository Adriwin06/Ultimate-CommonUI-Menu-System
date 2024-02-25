# This Readme is not finished.
## THIS IS STILL IN DEVELOPEMENT AND IS ABSOLUTELY NOT FINISHED.

# Features 
  -  Full WIP Game menu system
  -  Easy prompt system
  -  VERY complete graphics settings, like there is probably too much settings for a normal user, with engine setings as well as Post Process Volume Settings
  -  Main Menu with background map & camera movements

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
Press Num ² In-Game to access my menu.  
Tab bring the old menu before I switch to CommonUI
Num 0 bring a menu originally by Unreal Bench https://www.unreal-bench.com/unreal-engine but I modified the two menu and mixed them. 

### What follow next in the old Readme that isn't finished

# Conditions : 
  - You can use this freely in you project
  - You don't have to credit me (even if I would be glad if you do)
  - You can't claim that it's you work

## Known Bugs : 
  - If you enable the DLSS Super Resolution and then disable it, other upsaclers won't work until the game is restarted with DLSS Super Resolution disabled. (I can't figure out what causes this problem)
  - Settings will not be saved when exiting/re-enterring the menu
  - Some settings do nothing at the moment

## To do :
  - Store the menu variable in a Gamesave Blueprint. When the save button will be clicked, it will copy all usefull variable value in it and when the menu is oppened, it will restore them from that Gamesave BP. I'm not sure that's the best way of doing it but I will try and exeperiment on it and see where it go.
  - Add a lot more settings (full list not available atm because it's too long and not completed)
