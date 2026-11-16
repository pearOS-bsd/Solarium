# Solarium

"Solarium" is the codename of **Liquid Glass**, the material of macOS 26. This repository is pearOS's version of it: the same effects, the same names, the same behaviour, written from what the real frameworks do and not from anyone's source.

**Status: started.** This commit holds the map of where Liquid Glass lives in macOS 26.7.2 and the plan; no effect is implemented yet.

## Where Liquid Glass lives in macOS 26 (measured on the Mac VM, `tools/list-glass-classes.m`)
There is no `Solarium.framework` on disk; the code is spread over existing frameworks:

| framework | what | classes found |
|---|---|---|
| **QuartzCore** | the renderer: glass as a signed-distance-field effect on a layer | `CASDFGlassDisplacementEffect`, `CASDFGlassHighlightEffect` |
| **AppKit** | the views and the container that melts neighbouring glass | `NSGlassEffectView`, `NSGlassEffectContainerView`, `NSGlassContainerView`, `NSGlassView`, `NSContainerConcentricGlassEffectView`, `_NSGlassEffectViewMaterialContext`, `NSGlassMaterialProviderPocketWrapper`, `_NSSolarium`, `_NSLiquidMorph`, `_NSLiquidMorphingContainerView`, `NSLessExpensiveSubduedGlassEffectView`, `NSProgressIndicatorSolariumVisualProvider` |
| **DesignLibrary** | the parameters of the material | `GlassMaterialProvider` (with `Defaults`, `CustomFillBase`, `Pocket`) |
| **SwiftUICore** | the SwiftUI side (`.glassEffect`) | `GlassMaterialContext`, `GlassEffectBackdropObserver`, `GlassContainerCache`, `PlatformGlassDefinition`, `PlatformGlassInteractionDefinition` |
| **CoreImage / CoreUI** | filters | `CIGlassLozenge`, `CIGlassDistortion`, `CUIGlassHighlight`, `CUIGlassHighlightFromAlpha` |

## Plan
1. Take the thin slices of QuartzCore, AppKit and DesignLibrary from the Mac (`macdeps`, never committed) and read them with REA: the classes above, their properties and the Metal shaders they load (function names, constants, the order of the passes).
2. Write down what Apple does in `docs/APPLE-DECISIONS.md` with the evidence (addresses, decompiled constants), as in the other repositories.
3. Implement `CASDFGlass*Effect` for our `QuartzCore` and the window server's compositor (`WindowServer/src/compositor_gl.c`, GLES2, which already has a first glass shader and a CPU blur), then the AppKit views on top.
4. Defaults of the material: pearOS settings (`docs/EVIDENCE.md`).

Layer: 3 (UI), above `QuartzCore`, below `AppKit` (AppKit may use Solarium; Solarium must not use AppKit). Licence: BSD-2-Clause, written from scratch; GPL code (for example the KWin "better blur" fork used in the Arch edition of pearOS) is **not** copied: only numbers and public techniques.
