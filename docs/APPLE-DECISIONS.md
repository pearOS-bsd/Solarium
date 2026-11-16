# What Apple does (with evidence)

All values read on macOS 26.7.2 (x86_64) with `tools/sdf-defaults.m` (the classes' own `+defaultValues` and `+CA_attributes`) and from the extracted QuartzCore (REA/Ghidra, `-[CASDFGlassDisplacementEffect configureLayer:transaction:]` at 0x7ff80d3b34e0 of the extracted image; the `defaultValues` block at 0x7ff80d3b393a only builds the dictionary from a constant).

## Architecture: Core Animation signed-distance-field layers (QuartzCore)
Liquid Glass is not an AppKit effect first: it is built from **`CASDFLayer`** (a `CALayer` whose shape is a signed distance field) with **`CASDFElementLayer`** children and a list of **effects** (`CASDFEffect` subclasses: `NSSecureCoding`, `NSCopying`, each with `+name`, `+defaultValues`, `+CA_attributes` and `-configureLayer:transaction:`). A `CASDFGenerator` / `CASDFGeneratorRequest` produces the field. The effects:
`Fill`, `Gradient`, `GradientContour`, `Shadow`, `KeyFillHighlight`, **`GlassDisplacement`**, **`GlassHighlight`**, `Output`, `Visualization`.

## Defaults and ranges of the effects
| effect (`+name`) | property = default (range from `CA_attributes`) |
|---|---|
| **GlassDisplacement** ("Glass Displacement") | `height` = 20 (0..50), `curvature` = 1 (0..1), `angle` = 0, `maskOffset` (no default: 0) |
| **GlassHighlight** ("Glass Highlight") | `amount` = 0.5 (0..1), `angle` = 1.5708 (pi/2), `spread` = 3.1416 (pi), `height` = 20, `curvature` = 1, `color` = white (gray 1, alpha 1), `global` (bool) |
| KeyFillHighlight ("Key Fill Highlight") | `keyAmount` 0.5, `keyAngle` pi/2, `keyHeight` 20, `keySpread` pi, `keyColor` white; `fillAmount` 0.5, `fillAngle` -pi/2, `fillHeight` 20, `fillSpread` pi, `fillColor` white; `curvature` 1; `global` 0; the `*Offset` 0 and `*Scale` 1 of height and spread |
| Shadow | `radius` 10, `offset` (0, -10), `color` black, `invert` 0, `punchout` 0 |
| GradientContour | `radius` 10, `angle` 0, `startColor` white, `endColor` black, `gradientOffset` 0, `shadowAlpha` 1, `shadowEffectOffset` 3, `shadowGradientStart` 0, `shadowGradientEnd` 1 |
| Fill | `color` white |
| Gradient | `colors`, `distances` (-100..100), `interpolations` (timing functions): no defaults |
| Output | `minimum` -10000 (slider -200..0), `maximum` 100 (0..200) |
Colours are in "Generic Gray Gamma 2.2". Angles in radians. The sliders tell the intended ranges (the real inspector uses them).

## How an effect becomes a render record (`configureLayer:transaction:`, decompiled)
`-[CASDFGlassDisplacementEffect configureLayer:transaction:]` (0x7ff80d3b34e0) reads four doubles from the effect and writes one record (a struct of the render transaction):
| offset | content |
|---|---|
| +0x88 | effect kind byte = **7** (glass displacement) |
| +0x18 | float `height` (as is) |
| +0x1c | float `curvature`, **clamped to 0..1** |
| +0x20 | float `angle` |
| +0x24 | float `maskOffset` |
| +0x28 .. +0x80 | zero (other kinds use these) |
The order of the properties fetched is height, curvature, angle, maskOffset (the property accessors are looked up by their selectors; names confirmed by the class dump, the mapping of the last two to +0x20 / +0x24 is from the order of the calls: UNVERIFIED by a second source). The kind numbers of the other effects are read the same way (to do: GlassHighlight, Shadow, KeyFillHighlight ...).

## The pearOS values compared with these
The pearOS Arch settings (blur 2, corner radius 16 ...) describe a blur-based approximation; Apple's glass is a displacement (refraction) and highlight driven by a distance field of the shape, with `height` 20 and `curvature` 1 as the neutral state. Where the two disagree, Apple's numbers here win.

## Not yet read
The Metal functions that implement the displacement and highlight (`QuartzCore/Resources/default.metallib`, 160 MB AIR, being tried with metal2vulkan), the order of the passes, and how `NSGlassEffectView` / `GlassMaterialProvider` choose these parameters for a material (variants: regular, clear, tinted, interactive, subdued).
