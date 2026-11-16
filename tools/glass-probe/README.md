# glass-probe

A small macOS 26 program that shows the real Liquid Glass (`NSGlassEffectView`) over a known test background, saves one PNG per scene and a text dump of the glass's Core Animation layers (filters and SDF effects with all their parameter values). It is the reference we compare our implementation against (`docs/ALGORITHM-FOREGROUND.md`).

Needs a Mac with **macOS 26 or newer** and the Xcode Command Line Tools (`xcode-select --install`). No Xcode project, no network, nothing is installed.

    sh build-and-run.sh

Writes `captures/<macOS>-<arch>/` in the repository (`background.png`, one `.png` and one `.txt` per scene); then `git add captures && git commit && git push`. The binary is not committed (.gitignore).
Scenes: regular 340x200 radius 60, clear 340x200 radius 60, regular 340x200 radius 100, regular 200x200 (circle-like), regular 120x120. Edit the table at the end of `main.m` to add more.
