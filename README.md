# PlayStation 1 3D graphics demo

### Features

- Render 3D
- Gamepad input
- XA File music player
- Sound effects
- OBJ parser / 3d models
- Texture mapping
- Camera 3D
- Sprites
- Pre-rendered backgrounds
- Read data from CD-ROM

### Dependencies
- Psy-Q SDK

## Config
  1. Download Psy-Q from psxdev.net, make a "temp" and "psyq" folder under C:\ 
  2. Copy the folders "include", "lib" and "bin" from the sdk to C:\psyq\
  3. Execute PSPATH.BAT (this file is in the psyq folder) if you are on Windows 9x,  
     instead if you are on Windows NT (Windows 2000, XP),  
     you need to add the paths and variables inside PSPATH.BAT to the environment variables settings.
  4. copy the data folder from the last release, place it in the project directory (alongside src and cdrom)

## Build stages.bin
  stages.bin is a binary file that contains all the pre-rendered background data.  
  You need to generate it using a program I created.  
  The purpose of this is to define all the data you need, and it will create the stages.bin file.  
  This file will be included in the ISO and will be read to load all the necessary background data.  
  You must have GCC and Make installed to be able to run the compilation command:
  ```console
  cd path/of/project/data/stages/bin
  make run
  ```

## Compile on Windows (9x/2000/XP) and make game.iso file with buildcd.exe
  (this method is suggested for the release build to play on real hardware)
  1. edit cdrom/CONF.CTI file by changing the root variable to the correct path where your project is located
  ```console
  Define root C:\path\of\project
  ```
  2. move inside src folder with the command line (cmd on windows) and launch the following command
  ```console
  psymake
  ```

## Compile on DosBox and make game.iso file with buildcd.exe
  (good enough for developing)
  1. Install dosbox in your system
  2. launch the following command from the project's root 
  ```console
  dosbox -conf dos.conf
  ```

## Add License
  if you want to burn a cd-rom to play on real ps1, patch the iso with DiscPatcher.exe, the program can be found  
  inside cdrom folder, select PSX console and Europe/America/Japan region

## Play 
  - Move the character with d-pad buttons
  - Press R1 to start a fight example
  - Press circle button to exit the fight (when ATB bar is ready)

## Follow me on [YouTube](https://www.youtube.com/@FabioPallini88) 

![Screenshot from 2024-09-06 14-23-43](https://github.com/user-attachments/assets/e36a3d23-df61-4aab-bec0-6a5f0675e954)  

![Screenshot from 2024-09-06 14-23-59](https://github.com/user-attachments/assets/a7098b3c-293a-4b67-9a71-7cd460ceaa79)  

![screen2](https://github.com/fabiopallini/ps1-graphics-demo/assets/8449266/77b52bce-f75b-4441-b541-32a624763b7d)  

![screen3](https://user-images.githubusercontent.com/8449266/84420744-c4da7600-ac1a-11ea-90af-86e16c00ec95.gif)
