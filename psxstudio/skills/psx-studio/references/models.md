# Models for PSX 3D Studio

Read the repository's `AGENTS.md` and [README.md](../../../README.md) for tool startup and an overview; consult `psxstudio/3d_editor/ui.c` for modeler controls.

The modeler is `psxstudio/3d_editor`, built with GTK 4, C11 and OpenGL.
Build and launch it with `make -C psxstudio/3d_editor build` and
`make -C psxstudio/3d_editor run` from the repository root. It uses the
`psx3dstudio-1` project schema and PS1 OBJ format. Its mesh operations are in
`model.c`, import/export in `files.c`, rendering in `render.c`, and controls in
`ui.c`. It imports triangle/quad OBJ files; triangulate larger polygons before
importing. Use `make -C psxstudio/3d_editor test` to check its core operations.

## Creation and editing

Start from the request: character, object, scale, style, and intended destination. Before refining an existing model, inspect its OBJ, PNG, and the code that loads it. For Crystal Fantasy, compare the assets in `data/chars/char1`; for other models, follow the requested style.

Prioritize a readable silhouette, few vertices, and details painted into the texture. Loader limits are safety thresholds, not budgets to fill. Keep hair, shoulders, and other recognizable parts visible even from the side. Check normals, missing faces, and collapsed surfaces.

You can use the native editor or generate data and verify it through its C code: `mesh_validate` in `model.c` and `obj_parse`, `obj_export`, `project_encode`, and `project_decode` in `files.c`. Consult their signatures in `editor.h` instead of recreating an incompatible exporter. The program handles a single pose, with no rig or timeline.

## Game compatibility

The loader in `src/psx.c` holds 2048 vertices, 2048 UVs, and 2048 face indices: each quad uses 8 indices, allowing at most 256 quads. Export in the PS1 format: only `v/vt` faces with four pairs; for triangles, repeat the last pair. Do not mix triangles and quads, or add normals or materials to the PS1 OBJ.

Coordinates must be decimal numbers without exponents, within the parser's 9-character limit, with lines shorter than 100 characters. Use the program's exporter. Check SVECTOR conversion at the actual scale read by the code: RUN and ATT may use different scales.

In Crystal Fantasy, Y points down, feet are approximately at Y=0, and the head is at negative Y. The modeler's Y control changes the view, not the exported coordinates.

For existing animations, preserve topology, vertex order, and UV indices across all frames. Poses are separate OBJ files: editing one file does not change the entire animation. The `nan` UVs in some assets are normalized to zero, as in the game's parser. The current character has about 240 quads, leaving little room for extrusions.

## Textures and delivery

Use a small PNG atlas consistent with the mesh, and preserve UVs when refining existing assets. The interface's Map UV command assigns the entire texture to each face: do not use it to indiscriminately replace an existing atlas. For new textures, plan which regions each model part needs. Preserve alpha and check transparent edges.

Deliver a PS1 OBJ, a reopenable `.model.json` project, and a PNG when requested, with a preview and geometry counts. The JSON uses `format: psx3dstudio-1`, `model`, `texture`, `camera`, `view`, and `yDown`; consult `files.c` to construct valid values. Use the folder specified by the user; if none is specified, keep new assets in a subfolder of the modeler. Do not replace existing assets unless the request includes replacement.

Verify that the project and OBJ open in the native editor, check the texture, front and side views, and index round-tripping. The OpenGL viewport is a working view, not a PS1 emulation. For changes to the program, also run `make -C psxstudio/3d_editor test` and, when rendering or controls change, its `--smoke-test output.png` GUI check.

When game integration is requested, follow `AGENTS.md` for Wine/img2tim, free VRAM slots, and registration in `cdrom/CONF.CTI`. Use ISO9660-compatible names, verify runtime paths and scale, build with `bash build.sh`, and read `src/BUILD.LOG`. Creating a model alone does not require engine changes or a game build.
