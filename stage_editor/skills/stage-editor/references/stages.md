# Data, backgrounds, and PS1 integration

## Repository sources

- `stages/stages.json`: data that can also be edited in the editor.
- `src/data.h`: StageData capacities and layout.
- `src/game.c`: stage_load, movement, zones_collision.
- `src/psx.c` and `src/utils.c`: mesh_on_plane, camera, and background_init.
- `stage_editor/model.c`: projection, collision checks, validation, and JSON handling.
- `stage_editor/assets.c`: PNG import, TIM encoder, and ZIP export.
- `stage_editor/render.c` and `ui.c`: OpenGL rendering and GTK interface.
- `stage_editor/README.md`: starting the tools.
- `stages/Makefile`, `build.sh`, `cdrom/CONF.CTI`: generation and integration.

Check these files if they have changed since this guide was written. Do not apply
Crystal Fantasy background rules to Neon Ranger/SHOOTER assets.

## Editor and data

Build and launch the native editor from the root with:

```bash
make -C stage_editor build
make -C stage_editor run
```

It reads `stages/stages.json` and backgrounds in `data/stages`. Saving is manual,
using Save JSON or Ctrl+S: it keeps the previous version in `stages.json~` and
checks the file revision. If there is a conflict, reread and reapply only the
relevant changes; avoid overwriting a newer user copy.
Append new areas at the end: stage and spawn indices are the IDs used by zones.
Preserve NPCs, dialogues, and unknown fields; modify the starting area only as
needed to connect the exit and return entry point.

To validate without opening a window, after `make build`:

```bash
stage_editor/build/stage_editor --validate stages/stages.json
```

The standalone `stage_editor/preview_stage.py` script generates an SVG
layout guide, even before a background exists:

```bash
python3 stage_editor/preview_stage.py 8 --output /tmp/area-8.svg
```

Choose the camera and paths before creating the image, and use the projected
guide as a reference. Leave margins around walls, railings, and decorations:
adding rectangles cannot fix a floor with incompatible perspective.

## Geometry and camera

Each area has name, tim_0, tim_1, cam_x/y/z, cam_rx/ry/rz, and the arrays planes,
zones, spawns, and npcs (optional). A plane is `{x,y,z,w,h,d}`: positive W,
negative D, with sides aligned to X/Z. The engine loads Y as elevation; H does
not create stairs or vertical collisions. For normal paths, use Y=0 and H=0.
Do not replace these rectangles with arbitrary polygons or 3D meshes.

Per-area limits: 10 planes, 5 zones, 5 spawns (at least one), and 5 NPCs in total.
Coordinates/dimensions are 32-bit integers; rotations are 16-bit shorts.
TIM names must fit in char[10], including the terminator: for example BG9_0.TIM.
The editor accepts uppercase basenames of 1–5 characters, using letters, digits,
and underscores. Dialogue pages may contain at most 77 UTF-8 bytes, without NUL.

Projection matches `stage_editor/model.c`: R=Rx*Ry*Rz, camera point=R*(p+camera.pos),
center (160,120), focal length 512. Angles: 4096 units per turn. The camera's
world coordinates are -camera.pos; do not interpret cam_x/y/z as its direct position.
The preview uses floating point; the GTE uses fixed-point rounding.
Clip polygons at the near plane instead of discarding entire paths that extend
outside the camera. `preview_stage.py` supports all three rotations.

## Connections and reachability

Each zone also has stage_id and spawn_id. For a bidirectional passage:

1. Add an entry point in the new area, on the ground and away from its exit.
2. In the starting area, add a reachable zone pointing to the new entry point.
3. Add a return entry point in the original area without moving existing ones.
4. In the new area, add a zone pointing to this return entry point.

Check IDs, platform continuity, and trigger reachability from each relevant
entry point. A zone and a platform merely touching in the drawing is not enough.
The walkability check in `model.c` mirrors the strict bounds of mesh_on_plane:
x > plane.x-1, x < plane.x+w, z > plane.z+d, z < plane.z+1.
The footprint used by triggers extends the character point's x and z by mesh.size;
validation in `model.c` defaults to 100, which must be checked against the current model.
A spawn that appears outside the trigger rectangle may still activate it.
Avoid immediate bouncing between areas, and do not move entry points used by other zones.

## Preparing the background

Create or normalize a 320×240 PNG. After the preliminary layout, overlay platforms,
zones, and entry points to verify that the floor depicts the path.
The 320×240 PNG is the editor guide; complete the PAL rows as described below.
The SVG can be opened in a browser; for tools that require raster images, use
a capture of the rendering or convert the SVG to PNG.

The two PS1 pages are:

- left: 256×256, columns 0–255, VRAM (320,0);
- right: 64×256, columns 255–318, VRAM (576,0).

The second page starts at column **255**, matching the engine's sprite:
do not change it to 256; the images share a column. Check `SCREEN_HEIGHT` and
PAL/NTSC in `src/psx.h`: the current game is PAL 320×256, so rows 240–255 are
visible and must contain the background, never black padding.
The editor displays only the first 240 rows: for PAL integration, also complete
the bottom 16 rows without shifting the content or coordinates of the first 240.
For NTSC 320×240, the additional rows can instead be padding. Both pages are
16-bit TIMs and replace the previous background's textures: do not assign them
additional resident slots.

The editor preserves all rows of a 320×256 PNG; for other formats, it scales to
320×240 and reflects the last 16 rows to complete the PAL background.
Still check that the extension is consistent with the floor. The program keeps
imports in `stage_editor/assets`; ZIP export includes four assets,
CONF_ENTRIES.TXT, and README.txt. Copy runtime assets into `data/stages`, and also
keep the full PNG for future previews if it was produced separately.
Use names that are not already assigned; after BG9, B10_0.TIM/B10_1.TIM fit the limit.

Alternatively, from `data`, after preparing the PNG pages:

```bash
wine img2tim.exe -bpp 16 -usealpha -org 320 0 stages/BG9_0.png
wine img2tim.exe -bpp 16 -usealpha -org 576 0 stages/BG9_1.png
```

Preserve opaque black as 0x8000; 0x0000 is transparent on the PS1.
With `-usealpha`, prepare PNGs with explicitly opaque alpha. Also check very dark
pixels: RGB555 quantization may produce zero; if the source pixel is opaque,
correct it to 0x8000 in the TIM.
Do not remove opaque black pixels from shadows or objects. For an opaque background,
normalize alpha first; the native encoder does this by compositing onto black.

## CTI, generation, and build

Register both TIMs **at the root of the existing hierarchy** in `cdrom/CONF.CTI`,
with uppercase File names and Source matching the actual file.
Do not put them in the SHOOTER directory or duplicate existing entries.
Example (repeat for the second page):

```text
File BG9_0.TIM
    XAFileAttributes Form1 Data
    Source [root]\data\stages\BG9_0.tim
EndFile
```

Preserve the CTI's existing line endings. Only registered files are present
in the ISO: having them in data is not enough.

Validate before running the generator: it does not check all array limits.
Run `make build` from the `stages` folder. The Makefile uses **-m32**:
StageData contains NPC pointers; compiling the generator as 64-bit changes the layout.
This updates `data/stages/STAGES.BIN` and `src/stages.h`, even though git ignores them.
The fixed-size part is currently 684 bytes; dialogue pages add variable blocks.
Do not write offsets based on 684 without checking the current layout.
Check the offset table, record count, and destinations.

Run `bash build.sh` from the root, then inspect `src/BUILD.LOG` for Psy-Q, linker,
BUILDCD, and ISO generation errors. A successful shell command alone does not
prove that the entire DOS build succeeded. If necessary, verify the presence of
the new TIMs and STAGES.BIN in the ISO. For graphics-only work without integration,
stop at the requested assets and state what remains to be integrated.

Check character alignment, exits, spawns, and visual continuity on an emulator
or hardware when those tools are available. If they are not, clearly report
that this check remains to be done. Game C code must remain ANSI C89/Psy-Q;
the native editor uses C11. This workflow normally requires no C changes.
