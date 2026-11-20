#!/bin/sh
# Builds realcpc.dsk : build.sh <path to rasm>
set -e
RASM="${1:-rasm}"
cd "$(dirname "$0")"
rm -f realcpc.dsk
for f in menu crtc ga moniteur fdc plus; do "$RASM" $f.asm -eo; done
