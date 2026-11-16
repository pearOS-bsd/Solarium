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

## The glass filter: parameters and defaults (`CA::OGL::GlassBackgroundFilter::render`, decompiled with REA)
Read from `CA::OGL::GlassBackgroundFilter::render` (0x7ff80d236774 in the extracted QuartzCore 26.7.2 x86_64; `GlassForegroundFilter::render` is at 0x7ff80d2392d0, not read yet). The filter takes its parameters from the layer filter's key/value array (`Render::KeyValueArray`) by **numeric key**, with the default Apple uses when the key is absent. The values are written into the shader's constant block (the 336 bytes at binding 2 of `glass_background_*`): the half-float matrices first, the floats after (struct offsets in the Context: floats from +0xB0 on).

| key (hex) | default | read as |
|---|---|---|
| 0x1ab | vec2 (0, 10) | offset |
| 0x177 / 0x179 / 0x176 / 0x178 | colour; 1.0 / 0.0 / 1.0 | tint colour and its three YCC composite factors (`ColorMatrix::set_ycc_composite`) |
| 0x150 / 0x152 / 0x14f / 0x151 | colour; 1.0 / 0.0 / 1.0 | second colour (same composite) |
| 0x1a6 / 0x1a8 / 0x1a5 / 0x1a7 | colour; 1.0 / 0.0 / 1.0 | third colour (same composite) |
| 0x187 | 1.2 | a ratio compared with the destination scale (clamped; `(min(scale,v)-1)/(v-1)` when v > 1.0000001) |
| 0x19f | 0.0 | extra alpha added to the third colour when > 0 |
| 0x182 / 0x183 | -150 / 60 | |
| 0x18b / 0x18c | 100 / 50 | |
| 0x195 / 0x196 | -11 / -3 | |
| 0x164 | 30 | blur radius (scaled by `1.6 * scale`) |
| (function) `get_glass_filter_bleed_blur_radius` | | bleed blur radius |
| 0x14d / 0x156 | 400 / 500 | (500 is stored as 1/500) |
| 0x1a1 / 0x1aa | 200 / 250 | (250 is stored as 1/250) |
| (function) `get_glass_filter_shadow_blur_radius` | | shadow blur radius |
| 0x1ad | 25 | stored as 1/25 |
| 0x15f / 0x160 / 0x161 / 0x162 | 1.0 / 0.1 / 0.1 / 0.4 | a falling ramp (stored as differences: 0.9, 0.0, -0.3) |
| 0x15a / 0x15b / 0x15c / 0x15d | -450 / -3 / -0 / 0 | |
| 0x154 / 0x155 | -400 / -42 | |
| 0x157 | 0.2 | also selects the pipeline flag (`<= 0` adds 2) |
| 0x17a | 1.0 | |
| 0x153 (int) | 0 | picks (1,0) or (-1,1) as a sign pair |
| 0x1a9 | -50 | |
| 0x1ac | 1.0 | clamped to 0..1 |
| 0x199 | 0.75 | |
| 0x1ae | 1.0 | |
| 0x19d (int) / 0x19b / 0x19c / 0x19e | 0 / -2.5 / -1.5 / 0.97 | (when 0x19d != 0 an extra term `1 - ratio` is set) |
| 0x166 / 0x167 (int) | 0.0 / 0 | passed through `powf` and a flag |
Two code paths: with an SDF (`emit_sdf_bounds`, pipeline flag 0x45) or without (0x44): the same values go to different offsets (+0xB0.. vs +0x100..).

**Unverified:** the *names* of the keys (the numbers are the CAFilter input-key ids; the table of id -> name has to be read from the key-name table of QuartzCore), what each number means visually, and the exact byte offsets of each value in the block (the decompiler shows Context field indices, not the final packed layout of the Metal struct).

## Names of the glass filter keys (runtime, `tools/filter-keys.m`; 2026-11-16)
`+[CAFilter filterWithType:@"glassBackground"]` has **55 input keys**, `glassForeground` has **13** (`-inputKeys`; the filter-type constants are `kCAFilterGlassBackground` / `kCAFilterGlassForeground`, instantiated by `CA::OGL::initialize_filters`).
- glassBackground: SourceSublayerName; InnerRefractionAmount/Height; OuterRefractionAmount/Height; RefractionDistance0/1; RefractionOpacity; BlurRadius; BlurOpacity0..4; BlurDistance0..4; FaceOpacity; FaceColorMatrix{White,Black,Saturation,FillColor}; BleedAmount/Height/BlurRadius/Distance0/Distance1/Opacity/DarkenBlend; BleedColorMatrix{...}; ShadowOffset/Amount/Height/Opacity/DistanceOffset/BlurRadius/Radius; ShadowColorMatrix{...}; SDRHoldingToneEnabled/White; SDRGradientDistance0/1; SDRShadowOpacity; MaxHeadroom; ShadowVibrancyContribution; Clamp; ClampPreserveHue.
- glassForeground: SourceSublayerName; RefractionAmount/Height/Offset/Angle; AberrationAmount/Height/Offset/Angle; EdgeStart/End; EdgeOpacityStart/End.

### Numeric id -> name (SOLVED, read from the real `CAInternAtom`, `tools/filter-atoms.m`)
The ids are the **atoms** QuartzCore interns for the key names (`CAInternAtom("inputShadowOffset", 0)` = 0x1ab ...). This replaces the earlier guess by alphabetical order, which was only right near the end. glassBackground, sorted by id, with the default `GlassBackgroundFilter::render` uses when the key is absent:
| id | key (`input...`) | default |
|---|---|---|
| 0x14d | BleedAmount | 400 |
| 0x14e | BleedBlurRadius | (function get_glass_filter_bleed_blur_radius) |
| 0x14f | BleedColorMatrixBlack | 0.0 |
| 0x150 | BleedColorMatrixFillColor | colour |
| 0x151 | BleedColorMatrixSaturation | 1.0 |
| 0x152 | BleedColorMatrixWhite | 1.0 |
| 0x153 | BleedDarkenBlend | 0 (int) |
| 0x154 | BleedDistance0 | -400 |
| 0x155 | BleedDistance1 | -42 |
| 0x156 | BleedHeight | 500 |
| 0x157 | BleedOpacity | 0.2 |
| 0x15a | BlurDistance0 | -450 |
| 0x15b | BlurDistance1 | -3 |
| 0x15c | BlurDistance2 | -0.0 |
| 0x15d | BlurDistance3 | 0.0 |
| 0x15e | BlurDistance4 | - |
| 0x15f | BlurOpacity0 | 1.0 |
| 0x160 | BlurOpacity1 | 0.1 |
| 0x161 | BlurOpacity2 | 0.1 |
| 0x162 | BlurOpacity3 | 0.4 |
| 0x163 | BlurOpacity4 | - |
| 0x164 | BlurRadius | 30 |
| 0x166 | Clamp | 0.0 |
| 0x167 | ClampPreserveHue | 0 (int) |
| 0x176 | FaceColorMatrixBlack | 0.0 |
| 0x177 | FaceColorMatrixFillColor | colour |
| 0x178 | FaceColorMatrixSaturation | 1.0 |
| 0x179 | FaceColorMatrixWhite | 1.0 |
| 0x17a | FaceOpacity | 1.0 |
| 0x182 | InnerRefractionAmount | -150 |
| 0x183 | InnerRefractionHeight | 60 |
| 0x187 | MaxHeadroom | 1.2 |
| 0x18b | OuterRefractionAmount | 100 |
| 0x18c | OuterRefractionHeight | 50 |
| 0x195 | RefractionDistance0 | -11 |
| 0x196 | RefractionDistance1 | -3 |
| 0x199 | RefractionOpacity | 0.75 |
| 0x19b | SDRGradientDistance0 | -2.5 |
| 0x19c | SDRGradientDistance1 | -1.5 |
| 0x19d | SDRHoldingToneEnabled | 0 (int) |
| 0x19e | SDRHoldingToneWhite | 0.97 |
| 0x19f | SDRShadowOpacity | 0.0 |
| 0x1a1 | ShadowAmount | 200 |
| 0x1a2 | ShadowBlurRadius | (function get_glass_filter_shadow_blur_radius) |
| 0x1a5 | ShadowColorMatrixBlack | 0.0 |
| 0x1a6 | ShadowColorMatrixFillColor | colour |
| 0x1a7 | ShadowColorMatrixSaturation | 1.0 |
| 0x1a8 | ShadowColorMatrixWhite | 1.0 |
| 0x1a9 | ShadowDistanceOffset | -50 |
| 0x1aa | ShadowHeight | 250 |
| 0x1ab | ShadowOffset | vec2 (0, 10) |
| 0x1ac | ShadowOpacity | 1.0 |
| 0x1ad | ShadowRadius | 25 |
| 0x1ae | ShadowVibrancyContribution | 1.0 |
| 0x1af | SourceSublayerName | - |
Notes: the value of an *int* key is 0 when absent; `inputBlurRadius` 30 is scaled by `1.6 * scale`; the *ColorMatrix* keys come in groups (Black / FillColor / Saturation / White) that feed `ColorMatrix::set_ycc_composite`; ids 0x13c..0x172, 0x193, 0x194, 0x197, 0x198 and 0x1af belong to glassForeground / shared (`SourceSublayerName` = 0x1af; foreground: Aberration* 0x13c..0x13f, Edge* 0x16f..0x172, Refraction* 0x193 0x194 0x197 0x198).
Read of the table: InnerRefraction 0x182/0x183 = -150 / 60, OuterRefraction 0x18b/0x18c = 100 / 50, RefractionDistance 0x195/0x196 = -11 / -3, RefractionOpacity 0x199 = 0.75, MaxHeadroom 0x187 = 1.2. **Correction:** my earlier "probable" mapping (RefractionDistance0/1 = 0x19b/0x19c, SDRHoldingToneWhite = 0x1a1) was wrong: 0x19b/0x19c are SDRGradientDistance0/1 (-2.5 / -1.5), 0x1a1 is ShadowAmount (200).

## The glass foreground filter (`CA::OGL::GlassForegroundFilter::render`, 0x7ff80d2392d0; decompiled with REA)
Small and complete: 13 keys (ids from the real `CAInternAtom`), pipeline flag **0x48** (no SDF) or **0x49** (with SDF: shader `glass_foreground_sdf`, then `emit_sdf_bounds`).
| id | key (`input...`) | default | stored as |
|---|---|---|---|
| 0x193 | RefractionAmount | -150 | as is |
| 0x197 | RefractionHeight | 100 | **1 / height** |
| 0x198 | RefractionOffset | -10 | as is |
| 0x194 | RefractionAngle | 0 | sin, cos (`sincosf`) |
| 0x13c | AberrationAmount | -15 | as is |
| 0x13e | AberrationHeight | 20 | **1 / height** |
| 0x13f | AberrationOffset | 0 | as is |
| 0x13d | AberrationAngle | 0 | sin, cos |
| 0x172 | EdgeStart | -4.5 | as is |
| 0x16f | EdgeEnd | -3.0 | as is |
| 0x171 | EdgeOpacityStart | 0 | as is |
| 0x170 | EdgeOpacityEnd (the 4th Edge key; read from the id order) | 1.0 | as is |
Constant block, in the order the code writes it (each line = 4 floats in the Context, which is the Metal constant buffer of `glass_foreground*`):
1. `(x, y, z, w)` = the layer transform scaled by the layer scale and divided by the source size (the 2x2 matrix part),
2. `(RefractionAmount, 1/RefractionHeight, RefractionOffset, AberrationAmount)`,
3. `(AberrationOffset, 1/AberrationHeight, sin(AberrationAngle), cos(AberrationAngle))`,
4. `(EdgeStart, EdgeEnd, EdgeOpacityStart, EdgeOpacityEnd)`,
5. `(sin(RefractionAngle), cos(RefractionAngle), 0, 0)`.
(The exact order inside lines 2 and 3 follows the stores at Context +0xB0.. as decompiled; two adjacent pairs are written as 8-byte halves, so the grouping above is by pairs of floats and is **not yet checked against the shader's loads**.)
Meaning, from the names and the maths: a refraction displacement with amount/height/offset/angle (angle given as a direction), a chromatic aberration with the same four parameters, and an edge band (`EdgeStart..EdgeEnd`, with opacity ramping from `EdgeOpacityStart` to `EdgeOpacityEnd`).
