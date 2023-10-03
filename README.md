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
- [MKPSXISO](https://github.com/Lameguy64/mkpsxiso)

### Config
  1. Download Psy-Q from psxdev.net, make a "temp" and "psyq" folder under C:\ 
  2. Copy the folders "include", "lib" and "bin" from the sdk to C:\psyq\
  3. Execute PSPATH.BAT if you are on Windows 9x, instead if you are using Windows NT (Windows 2000, XP, Vista and so on), you need to add the paths inside PSPATH.BAT to the environment variables.

### Compile
  1. psymake must be in your environment variables, it's part of Psy-Q SDK.
  2. copy data and mkpsxiso folders from one of my releases, and put them alongside src folder, create a new folder named cdrom also, that's where your game iso will be created.
  3. move inside src folder with the command line (cmd from windows) and launch the following command
  ```console
  psymake
  ```

 Follow me on [YouTube](https://www.youtube.com/@fabiopallini01) 

![preview2](https://user-images.githubusercontent.com/8449266/84420744-c4da7600-ac1a-11ea-90af-86e16c00ec95.gif)

![screenshot](https://github.com/fabiopallini/ps1-graphics-demo/assets/8449266/017cd9f4-c939-4971-be51-2cecd6f155aa)

