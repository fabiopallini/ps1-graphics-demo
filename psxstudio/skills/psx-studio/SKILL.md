---
name: psx-studio
description: Create or edit playable stages, characters, and low-poly models for real PS1 hardware in this Psy-Q project, using PSX Studio tools and their stages.json, OBJ, and TIM texture formats.
---

# PSX Studio: PS1 stages and models

This skill is local to the project, in `psxstudio/skills/psx-studio`.
Work in the repository with `stages/stages.json`, `src/game.c`, `cdrom/CONF.CTI`,
and `psxstudio`. Read `AGENTS.md` and the [tools README](../../README.md).

Choose the guide according to the requested task:

- **Backgrounds, platforms, zones, and entry points:** read [references/stages.md](references/stages.md).
  The tool is the native `psxstudio/stage_editor`, built with GTK 4, C11, and OpenGL.
  For an integrated example, see [references/sanctuary.md](references/sanctuary.md).
- **Characters, OBJ meshes, UVs, and model textures:** read
  [references/models.md](references/models.md). The native modeler is
  `psxstudio/3d_editor` (GTK 4, C11, OpenGL).

Load both guides only when the task involves both asset types.
Use the editor or edit the same data directly, whichever method is most
effective; validate the formats and visually inspect the result.

Respect the style, scale, and actual limits of the PS1 loader. When refining
existing assets, first inspect them and the code that loads them. Preserve
unaffected data, dialogues, unknown fields, and existing animation topology.
Do not adapt the engine to bypass asset limits.

To generate or edit raster images, use the `imagegen` skill when available;
otherwise use supplied assets or a suitable local method, stating their origin.
A request for only a preview, background, or model does not require game
integration or a game build. A request for a new connected playable area
includes data, assets, and integration unless explicitly limited.

Integrate into the game only within the requested scope: check names, VRAM,
CTI entries, and runtime paths as described in the relevant guide. Game C code
must remain ANSI C89/Psy-Q; the native editor uses C11. Report files, connections
or geometry counts, checks performed, and actual limitations. Distinguish previews
from emulator or PS1 testing, and never claim tests that were not performed.
