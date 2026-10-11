# Project instructions

## Background tasks and token usage

Avoid frequent status polling for background tasks such as builds. Prefer
completion notifications, longer waits, or a single automated wait that returns
only the final result. Inspect logs after completion rather than repeatedly
reading them while the task runs. Keep tool output and progress updates concise;
perform intermediate checks only when needed to investigate an error or respond
to the user. This reduces unnecessary tool calls and token usage.

## C language compatibility

All game source code must be compatible with ANSI C89 and the legacy Psy-Q
compiler. Do not use C99 or newer language features. In particular:

- declare variables at the beginning of a block, before statements;
- do not declare variables inside `for` initializers;
- use `/* ... */` comments instead of `//` comments;
- do not use `bool`, variable-length arrays, designated initializers,
  compound literals, `inline`, or C99-only headers and functions;
- keep declarations and function definitions compatible with the existing
  Psy-Q SDK headers and compiler.

## Build

Build the project from the repository root with:

```bash
bash build.sh
```

This launches the Psy-Q build environment through DOSBox-X via Flatpak.

After the build, inspect `src/BUILD.LOG` to see the compiler, linker, CD image,
and ISO generation output, including any errors.

## PNG to TIM textures

Run `img2tim.exe` through Wine from the `data` directory. For example:

```bash
cd data
wine img2tim.exe -bpp 16 -usealpha -org 320 0 shooter/TEX1.png
```

The `-org X Y` arguments specify where the TIM texture will be loaded in PS1
VRAM. Texture placement starts at X=320. Each 1024-pixel-wide VRAM row has 704
pixels available from X=320 through X=1023. This allows two 256x256 textures
plus one texture up to 192x256 on each row. Using Y=0 and Y=256 gives six
texture slots:

- `(320, 0)`
- `(576, 0)`
- `(832, 0)`, maximum size `192x256`
- `(320, 256)`
- `(576, 256)`
- `(832, 256)`, maximum size `192x256`

Always choose a free slot and make sure texture rectangles do not overlap in
VRAM. With this layout, use at most four 256x256 textures and two additional
textures up to 192x256, for a maximum of six textures total.

## Adding TIM files to the CD image

Every TIM file used by the game code must first be added to
`cdrom/CONF.CTI`. A file that exists under `data/` but is not listed in the CTI
will not be present in the generated ISO and cannot be loaded with
`cd_read_file()`.

Keep shooter assets inside the single ISO9660 `SHOOTER` directory. For
example, a source file at `data/shooter/BG1.tim` should be registered as:

```text
Directory SHOOTER
    File BG1.TIM
        XAFileAttributes Form1 Data
        Source [root]\data\shooter\BG1.tim
    EndFile
EndDirectory
```

The corresponding runtime path is:

```c
cd_read_file("SHOOTER\\BG1.TIM", &buffer);
```

Use ISO9660-compatible uppercase names for `Directory` and `File` entries,
keep each directory definition unique, and make the `Source` path match the
actual file under `data/shooter`. Rebuild the ISO with `bash build.sh` after
adding or changing a CTI entry, then inspect `src/BUILD.LOG` for BUILDCD
errors.
