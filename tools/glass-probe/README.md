# glass-probe

A small macOS 26 program that shows the real Liquid Glass (`NSGlassEffectView`) over a known test background, saves one PNG per scene and a text dump of the glass's Core Animation layers (filters and SDF effects with all their parameter values). It is the reference we compare our implementation against (`docs/ALGORITHM-FOREGROUND.md`).

Needs a Mac with **macOS 26 or newer** and the Xcode Command Line Tools (`xcode-select --install`). No Xcode project, no network, nothing is installed.

    sh build-and-run.sh

Writes `~/Desktop/glass-probe-out/` (`background.png`, one `.png` and one `.txt` per scene) and a zip next to it. The screenshots contain only the test window, nothing else of the screen is captured (`screencapture -l <window>`). The first run asks for the Screen Recording permission for Terminal; allow it and run again.
Scenes: regular 340x200 radius 60, clear 340x200 radius 60, regular 340x200 radius 100, regular 200x200 (circle-like), regular 120x120. Edit the table at the end of `main.m` to add more.
