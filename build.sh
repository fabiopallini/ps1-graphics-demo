#!/usr/bin/env bash
set -e

flatpak run com.dosbox_x.DOSBox-X -conf "$PWD/dos.conf"
