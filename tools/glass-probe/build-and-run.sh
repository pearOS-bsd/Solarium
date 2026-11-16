#!/bin/sh
# glass-probe: build (needs only the Xcode Command Line Tools) and run. Output: ~/Desktop/glass-probe-out
set -e
cd "$(dirname "$0")"
clang -fobjc-arc -Wno-deprecated-declarations -o glass-probe main.m -framework Cocoa -framework QuartzCore
OUT="${1:-$HOME/Desktop/glass-probe-out}"
echo "Running; the first time macOS asks to allow Screen Recording for Terminal: allow it, then run this script again."
./glass-probe "$OUT"
cd "$OUT/.." && zip -qr glass-probe-out.zip "$(basename "$OUT")" && echo "Send me: $OUT/../glass-probe-out.zip"
