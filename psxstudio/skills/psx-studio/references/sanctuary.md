# Sanctuary courtyard — area 8

The route continues from area 7_city: reaching the end of the street
leads to the courtyard. Returning along the path at the bottom takes the player
back to the city, at its second spawn, away from the exit trigger.

The configuration is in `stages/stages.json`. The engine and its existing editor
remain unchanged: planes are ground rectangles on X/Z, with positive w and
negative d. Two overlapping rectangles define the central path and courtyard.
A small inset from the floor edges avoids flower beds, railings, and lanterns.
The area uses two of the ten available planes, one return zone, and one spawn.

Camera: position (0,1055,2410), rotation (305,0,0), in game units.
The background was generated from a guide projected with this camera;
SetGeomOffset(160,120) and SetGeomScreen(512) match the engine.

Assets in data/stages:

- BG8.png: 320x240 background preview.
- BG8_0.png/TIM: left part, 256x256, VRAM (320,0).
- BG8_1.png/TIM: right part, 64x256, VRAM (576,0).
- BG8_WALK.svg: technical preview with green planes, a red trigger, and a light-blue spawn.

The initial version had padding in rows 240–255, but those rows are visible
in the PAL 320×256 game. For new exports, extend the background into those rows,
following [stages.md](stages.md). The right part starts at column 255,
matching s1.pos.vx in background_init: the shared column maintains continuity
with the existing engine's rendering.
The textures replace those of the previous screen without increasing the number
of resident textures. Both are registered at the ISO root in CONF.CTI with
DOS 8.3 names, within the 10-byte limit of the tims fields in StageData.

After changes to stages.json, run from the stages folder:

    make build

The 32-bit generator produces STAGES.BIN and stages.h for the Psy-Q layout.
Do not compile it as 64-bit: that would change pointer sizes and alignment.

To update the polygon preview, run from the project root:

    python3 psxstudio/stage_editor/preview_stage.py 8 --output data/stages/BG8_WALK.svg

To reconvert the textures, run from the data folder:

    wine img2tim.exe -bpp 16 -usealpha -org 320 0 stages/BG8_0.png
    wine img2tim.exe -bpp 16 -usealpha -org 576 0 stages/BG8_1.png

Finally, run bash build.sh from the root and read src/BUILD.LOG.
In the game, SELECT opens the existing editor and SQUARE adds a plane;
the directional buttons move it, and R1 + directional buttons change its width/depth.
Leaving the editor with SELECT prints the planes for stages.json.

The layout and binary values, spawn/area references, exit reachability using
mesh_on_plane and trigger logic, separation of entry points from triggers,
and TIM dimensions and VRAM positions have been verified.
The SVG image is a projected check, not a game screenshot.
Visual placement still needs to be checked in the game on an emulator or PS1.
