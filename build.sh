#!/usr/bin/env bash
set -e

if command -v dosbox-x >/dev/null 2>&1; then
    exec dosbox-x -conf "$PWD/dos.conf"
elif command -v dosbox >/dev/null 2>&1; then
    exec dosbox -conf "$PWD/dos.conf"
elif command -v flatpak >/dev/null 2>&1 && flatpak info com.dosbox_x.DOSBox-X >/dev/null 2>&1; then
    exec flatpak run com.dosbox_x.DOSBox-X -conf "$PWD/dos.conf"
else
    echo "Install DOSBox-X or DOSBox, or DOSBox-X via Flatpak (com.dosbox_x.DOSBox-X)." >&2
    exit 1
fi
