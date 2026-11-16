# Evidence

## Class map (2026-11-16, macOS 26.7.2 x86_64, `tools/list-glass-classes.m`)
Built on the Mac VM with `clang -fobjc-arc -o glass list-glass-classes.m -framework Foundation`; it loads AppKit, QuartzCore, SwiftUICore, SwiftUI, SkyLight, UIFoundation, CoreUI, WindowManagement and lists every Objective-C class whose name contains solarium, glass or liquid, with the image that defines it. `MaterialKit`, `Solarium`, `LiquidGlass` and `SolariumKit` did not load as frameworks (no such framework on disk). The list is in README.md.

## Parameters from pearOS (to be checked against the real frameworks)
From `pearOS-archlinux/pearos-settings` (`etc/skel/.config/kwinrc`): blur strength 2, noise 0, corner radius of docks, menus and the bottom 16. From the defaults of the Better Blur fork in `pearOS-archlinux/pkgbuilds/pearos-liquidgel` (GPL-3.0, not copied; numbers only): brightness, contrast, saturation 1.0; refraction: normal power 2.0, strength 0.0, edge size 20, RGB fringing 1.0. These were chosen to look like macOS; they are the defaults here until the decompilation of `GlassMaterialProvider.Defaults` gives Apple's own.
