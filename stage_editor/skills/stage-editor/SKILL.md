---
name: stage-editor
description: Create or edit playable PS1 stages in this Psy-Q project, using the native stage editor, stages.json, backgrounds, and TIM textures.
---

# Stage Editor: PS1 stages

This skill is local to the project, in `stage_editor/skills/stage-editor`.
Work in the repository with `stages/stages.json`, `src/game.c`, `cdrom/CONF.CTI`,
and `stage_editor`. Read `AGENTS.md` and the [editor README](../../README.md).

For backgrounds, platforms, zones, and entry points, read
[references/stages.md](references/stages.md). The native editor is
`stage_editor`, built with GTK 4, C11, and OpenGL.
For an integrated example, see [references/sanctuary.md](references/sanctuary.md).
Use the editor or edit the same data directly, whichever method is most
effective; validate the formats and visually inspect the result.

Respect the style, scale, and actual limits of the PS1 loader. When refining
existing assets, first inspect them and the code that loads them. Preserve
unaffected data, dialogues, and unknown fields.
Do not adapt the engine to bypass asset limits.

To generate or edit raster images, use the `imagegen` skill when available;
otherwise use supplied assets or a suitable local method, stating their origin.
A request for only a preview or background does not require game integration
or a game build. A request for a new connected playable area includes data,
assets, and integration unless explicitly limited.

Integrate into the game only within the requested scope: check names, VRAM,
CTI entries, and runtime paths as described in the guide. Game C code
must remain ANSI C89/Psy-Q; the native editor uses C11. Report files, connections,
checks performed, and actual limitations. Distinguish previews from emulator
or PS1 testing, and never claim tests that were not performed.
