#!/bin/sh
# Builds realcpc.dsk : build.sh <path to rasm>
set -e
RASM="${1:-rasm}"
cd "$(dirname "$0")"
rm -f realcpc.dsk
for t in t1in t2vsync t3mode t4border; do "$RASM" "$t.asm" -eo; done
