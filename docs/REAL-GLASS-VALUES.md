# Real glass values (macOS 27.0, Apple silicon), read from the layers of `NSGlassEffectView`

Source: `captures/27.0-arm64/*.txt`, produced by `tools/glass-probe` on a real Mac (macOS 27.0, 26A5416b): the Core Animation layer tree of the glass view. These are the **values the system actually sets** on the `glassBackground` CAFilter (not defaults of the filter), for a 340x200 view with corner radius 60, style regular / clear, and for a 120x120 regular view. (Layer of the same filter: `CASDF*` effects around it, see below.)

## What the screenshots show
- `regular` (captures/.../regular-340x200-r60.png): mostly a **frosted blur** of the background (blur radius 5 plus a wide "bleed" blur), slightly brightened and saturated, a soft edge; the refraction of the grid is weak. That is what Apple draws for regular glass, so a blur-like result is not an error of the capture program.
- `clear`: no blur (radius 0), nearly transparent, the background is lightly lifted; bright specular highlights on the rim and at the corners; a stronger refraction band (outer refraction 40, height 25, refraction opacity 0).

## Structure of the glass (from the dump)
`NSGlassEffectView` -> a SwiftUI `SDFLayer` -> children: a layer with `vibrantColorMatrix` filters (the content tint), a `CAPortalLayer` (the backdrop), a **`CASDFLayer`** whose layers carry the **`glassBackground` filter** and the effects `CASDFKeyFillHighlightEffect` and `CASDFOutputEffect`; an `SDFPortalLayer`. So the glass body is the `glassBackground` filter (the shader `glass_background_sdf` we translated) and the rim light is the SDF `KeyFillHighlight` effect. No `glassForeground` filter is on a plain glass view (it belongs to the lens-like controls).

## The `glassBackground` filter, values set by the system (77 keys on 27.0; 55 on 26.7.2)
| key (`input...`) | regular 340x200 | clear 340x200 | regular 120x120 |
|---|---|---|---|
| SourceSublayerName | @0 | @0 | @0 |
| InnerRefractionAmount | -60 | -60 | -60 |
| InnerRefractionHeight | 20 | 20 | 20 |
| OuterRefractionAmount | 50 | 40 | 30 |
| OuterRefractionHeight | 40 | 25 | 24 |
| RefractionDistance0 | -1 | -1 | -1 |
| RefractionDistance1 | 0 | 0 | 0 |
| RefractionOpacity | 0.6 | 0 | 0.6 |
| BlurRadius | 5 | 0 | 5 |
| BlurOpacity0 | 0.8 | 0 | 0.4666666 |
| BlurOpacity1 | 0.4 | 0 | 0.2333333 |
| BlurOpacity2 | 0.5 | 0 | 0.5 |
| BlurOpacity3 | 1 | 0 | 1 |
| BlurOpacity4 | nil | nil | nil |
| BlurDistance0 | -100 | -100 | -60 |
| BlurDistance1 | -1 | -1 | -1 |
| BlurDistance2 | 0 | 0 | 0 |
| BlurDistance3 | 0 | 0 | 0 |
| BlurDistance4 | nil | nil | nil |
| FaceOpacity | 1 | 1 | 1 |
| FaceColorMatrixWhite | 1.125 | 1.08 | 1.125 |
| FaceColorMatrixBlack | 0.125 | 0.125 | 0.125 |
| FaceColorMatrixSaturation | 1.3 | 1.06 | 1.3 |
| FaceColorMatrixFillColor | colour(?) | colour(?) | colour(?) |
| FaceColorMatrixMaxLuma | 0.45 | 1 | 0.45 |
| FaceColorMatrixMaxLumaSDR | 0.45 | 1 | 0.45 |
| BleedAmount | 70 | 0 | 42 |
| BleedHeight | 70 | 0 | 42 |
| BleedBlurRadius | 70 | 0 | 42 |
| BleedDistance0 | 1 | 1 | 1 |
| BleedDistance1 | 0 | 0 | 0 |
| BleedOpacity | 0.8 | 0 | 0.4666667 |
| BleedDarkenBlend | 0 | 1 | 0 |
| BleedColorMatrixWhite | 0.5 | 1 | 0.5 |
| BleedColorMatrixBlack | 0.125 | 0.75 | 0.125 |
| BleedColorMatrixSaturation | 1 | 1.2 | 1 |
| BleedColorMatrixFillColor | nil | nil | nil |
| ShadowOffset | NSSize: {0, 8} | NSSize: {0, 8} | NSSize: {0, 8} |
| ShadowAmount | 0 | 75 | 0 |
| ShadowHeight | 0 | 80 | 0 |
| ShadowOpacity | 0.6 | 0 | 0.3666666 |
| ShadowColorMatrixWhite | 1 | 1 | 1 |
| ShadowColorMatrixBlack | 0 | 0 | 0 |
| ShadowColorMatrixSaturation | 1 | 1.2 | 1 |
| ShadowColorMatrixFillColor | colour(?) | colour(?) | colour(?) |
| ShadowDistanceOffset | 0 | 0 | 0 |
| ShadowBlurRadius | 0 | 0 | 0 |
| ShadowRadius | 24 | 0 | 15.66666666666667 |
| RingShadowOffset | 8 | 0 | 8 |
| RingShadowStrokeWidth | 4 | 0 | 4 |
| RingShadowBlurRadius | 5 | 0 | 5 |
| RingShadowOpacity | 0.06 | 0 | 0.06 |
| RingShadowMask | 1 | 0 | 1 |
| KeyFillHighlightHeight | 1 | 1 | 1 |
| KeyFillHighlightAngle | 1.570796326794897 | 1.570796326794897 | 1.570796326794897 |
| KeyFillHighlightSpread | 1.308996938995747 | 1.308996938995747 | 1.308996938995747 |
| KeyFillHighlightSpreadSDR | 1.308996938995747 | 1.308996938995747 | 1.308996938995747 |
| KeyFillHighlightAmount | 0.4 | 0.4 | 0.4 |
| KeyFillHighlightEffectOffset | -1 | -1 | -1 |
| KeyFillHighlightColorBias | -0.25 | -0.25 | -0.25 |
| BlurFillBlurRadius | 8 | 0 | 8 |
| BlurFillLightenOpacity | 0 | 0 | 0 |
| BlurFillDarkenOpacity | 0.675 | 0 | 0.675 |
| BlurFillNormalOpacity | 0 | 0 | 0 |
| SDRHoldingToneEnabled | 0 | 0 | 0 |
| SDRHoldingToneWhite | 1 | 1 | 1 |
| SDRGradientDistance0 | 0 | 0 | 0 |
| SDRGradientDistance1 | 0 | 0 | 0 |
| SDRShadowOpacity | 0 | 0 | 0 |
| MaxHeadroom | 9999 | 9999 | 9999 |
| ShadowVibrancyContribution | 0 | 0 | 0 |
| Clamp | 1.308311 | 1.191748 | 1.308311 |
| ClampPreserveHue | 0 | 0 | 0 |
| AberrationAmount | nil | nil | nil |
| AberrationHeight | nil | nil | nil |
| AberrationOffset | nil | nil | nil |
| AberrationAngle | nil | nil | nil |

## Observations
- **Size dependence:** the 120x120 view gets smaller values: OuterRefractionAmount/Height 30/24 (vs 50/40), BleedAmount/Height/BlurRadius 42 (vs 70), ShadowRadius 15.67 (vs 24), BlurOpacity0/1 0.467/0.233 (vs 0.8/0.4), ShadowOpacity 0.367 (vs 0.6). 200x200 and 340x200 with radius 60 or 100 give the **same values** as 340x200: from the minimum side of 200 up nothing changes; below it the values scale (OuterRefraction and Bleed exactly by minSide/200 = 0.6; the opacities and ShadowRadius follow another curve, to be fitted with more sizes).
- The **corner radius does not change the filter values** (r60 and r100 identical): the shape goes through the SDF, not the filter.
- macOS 27.0 has more keys than 26.7.2 (e.g. `RingShadow*`, `KeyFillHighlight*`, `BlurFill*`, `FaceColorMatrixMaxLuma`, `Aberration*` on the background filter).
- The values differ from the filter's built-in defaults of `GlassBackgroundFilter::render` (APPLE-DECISIONS.md): the view sets them.

Not done: the same capture on macOS 26.x (the version this project targets), more sizes to fit the size law, the `CASDFKeyFillHighlightEffect` / `CASDFOutputEffect` values (they are in the .txt files), running `glass_background_sdf` with these values to compare with the screenshot.
