# PlayStation 1 3D graphics demo

### Features

- Render 3D
- Gamepad input
- XA File music player
- Sounds Effect
- OBJ Parser
- Texture mapping
- Camera 3D
- Sprites
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

## Compile on Windows (9x/2000/XP) and make game.iso file with buildcd.exe
  (this method is suggested for the release build to play on real hardware)
  1. edit cdrom/CONF.CTI file by changing the root variable to the correct path where your project is located
  ```console
  Define root C:\path\to\your\game
  ```
  2. move inside src folder with the command line (cmd on windows) and launch the following command
  ```console
  psymake
  ```
  3. (only if you want to burn a cd-rom to play on real ps1) Patch the iso with DiscPatcher.exe, the program can be found inside cdrom folder, select PSX console and Europe region

## Compile on DosBox and make game.iso file with buildcd.exe
  (good enough for developing)
  1. Install dosbox in your system
  2. launch the following command from the project's root 
  ```console
  dosbox -conf dos.conf
  ```
  3. (only if you want to burn a cd-rom to play on real ps1) Patch the iso with DiscPatcher.exe, the program can be found inside cdrom folder, select PSX console and Europe region

## Play 
  - Move the character with d-pad buttons
  - Press R1 to start a fight example
  - Press circle button to exit the fight (when ATB bar is ready)

## Follow me on [YouTube](https://www.youtube.com/@FabioPallini88) 

![screen1](https://github.com/fabiopallini/ps1-graphics-demo/assets/8449266/e231e45f-0049-4d8a-85d2-453c18d52370)

![screen2](https://github.com/fabiopallini/ps1-graphics-demo/assets/8449266/e7675d90-583d-441d-838b-f6d4a58c1f77)

![screen3](https://user-images.githubusercontent.com/8449266/84420744-c4da7600-ac1a-11ea-90af-86e16c00ec95.gif)
