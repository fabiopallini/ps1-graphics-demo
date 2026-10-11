# PSX Studio

Minimal PS1 tools with a Windows 95/98 interface.

- **Stage editor:** edit backgrounds, walkable platforms and transitions. From the repository root, run `python3 psxstudio/stage_editor/server.py`, then open http://127.0.0.1:8000. Save manually. Opening [index.html](stage_editor/index.html) directly also works, with manual file import/export.
- **3D model editor:** open [index.html](3dmodel_editor/index.html) directly in your browser; no server required. Edit low-poly meshes and UVs, save `.model.json` projects, and export PS1-compatible OBJ files. One pose and one PNG texture at a time.

Project-local skills: [psx-stages](stage_editor/skills/psx-stages/SKILL.md) and [psx-models](3dmodel_editor/SKILL.md). Ask your assistant to read the relevant file before creating assets.

**Stage editor**

![Stage editor preview](stage_editor/preview.png)

**3D model editor**

![3D model editor preview](3dmodel_editor/preview.png)
