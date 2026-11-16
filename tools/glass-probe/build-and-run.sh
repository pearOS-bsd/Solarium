#!/bin/sh
# glass-probe: build (needs only the Xcode Command Line Tools) and run.
# Results go to <repo>/captures/<macOS version>-<machine>/ : background.png, one .png and one .txt per scene.
set -e
cd "$(dirname "$0")"
REPO="$(cd ../.. && pwd)"
VER="$(sw_vers -productVersion)-$(uname -m)"
OUT="${1:-$REPO/captures/$VER}"
clang -fobjc-arc -Wno-deprecated-declarations -o glass-probe main.m -framework Cocoa -framework QuartzCore
echo "Running; the first time macOS asks to allow Screen Recording for Terminal: allow it, then run this script again."
./glass-probe "$OUT"
echo "Results in: $OUT"
echo "To send them: cd $REPO && git add captures && git commit -m 'captures from a real Mac' && git push"
