# The glass background shader (`glass_background_sdf`), what it computes

Read from the AIR function `glass_background_sdf_lpf` of the user's own Mac (macOS 26.7.2), translated to SPIR-V (metal2vulkan) and read as GLSL (SPIRV-Cross). **Described in our own words; the translated shader is Apple's code and is not committed.** Offsets are into the 384-byte constant block (see APPLE-DECISIONS.md and `tools/vk-run/gen_bg_block.py`). All distances are in the shape's units (pixels).

## Inputs per pixel
- from the first half of the shader (same as the foreground one, ALGORITHM-FOREGROUND.md): the signed distance `d` of the pixel to the shape (negative inside), the outward normal `n`, and a coverage `c` in 0..1 (anti-aliased inside factor);
- `dir = A * n` : the normal mapped to uv units by the 2x2 matrix A (block offset 48);
- the backdrop texture with a **mip chain**; every sample is taken with an explicit LOD, which is the blur: `lod = max(0, log2(r < 2 ? r/2 + 1 : r))` for a blur radius `r` (a radius of 8 -> LOD 3);
- "unpremultiply": `rgb / max(a, 1e-6)`, values below 1e-6 snapped to 0.

## Helper: the quarter-circle profile
`qc(x, amount, invHeight) = amount * (1 - sqrt(t (2 - t)))` with `t = clamp(x * invHeight, 0, 1)`: `amount` at x = 0, falling to 0 at x = height (a quarter circle, vertical at the start). It drives every displacement below.

## Pass 1 - the shadow (where `c < 1`, i.e. outside and on the rim)
1. a displacement `qc(-(d + ShadowDistanceOffset), ShadowAmount, 1/ShadowHeight)` along `dir`, the backdrop is sampled there at the LOD of the shadow blur radius;
2. the shadow's strength: the signed distance to the shape **moved by the shadow offset** goes through a smooth bell/erf-like polynomial (input `clamp(0.25 * dist/ShadowRadius + 0.5, 0, 1) * 4 - 2`, a degree-5 polynomial evaluated in half floats) times `ShadowOpacity`: an analytic Gaussian-blurred shadow of the rounded rectangle with radius `ShadowRadius`;
3. its colour: the Shadow colour matrix (rows 3 in `block+176`) applied to the sampled backdrop, mixed with the fill by the "vibrancy contribution" (offset 200/204); if the strength is below 1e-6 and the pixel is fully outside the pixel is discarded.

## Pass 2 - the body (where `c > 0`)
1. **Inner refraction + depth-dependent blur.** `s_in = qc(-d, InnerRefractionAmount, 1/InnerRefractionHeight)`; the sample position is `uv + s_in * dir`. The blur radius is not constant: with the bands `BlurDistance0..3` (negative = inside) three clamped ramps of the depth `s_in + d` give weights `o0-o1`, `o1-o2`, `o2-o3` (the opacities `BlurOpacity0..3`); the blur used is `BlurRadius * (o0 - sum of the three weighted ramps)`: radius `BlurRadius * o0` at the rim, going to `BlurRadius * o3` deep inside. Sample at that LOD.
2. **Outer refraction** (only if `RefractionOpacity > 0`): same with `OuterRefractionAmount/Height`; the result is `mix(inner, outer, clamp((d - RefractionDistance0) / (RefractionDistance1 - RefractionDistance0), 0, 1) * RefractionOpacity)`.
3. **Face colour:** `rgb' = mix(rgb, M_face * rgb + offset_face, FaceOpacity)` (matrix rows at block+128).
4. **Bleed:** a second sample, at the bleed blur radius and displaced by `qc(-d, BleedAmount, 1/BleedHeight)` (bleed's own profile), colour-matrixed with the Bleed matrix (block+152). Its weight is `((a * luma(face rgb) + b)^2 * ramp(d, BleedDistance0..1))^2 * BleedOpacity` where `luma = 0.2125 R + 0.7154 G + 0.0721 B` and `(a, b)` is (-1, 1) for the normal blend or (+1, 0) for `BleedDarkenBlend`: the bleed is the bright/dark glow that the glass takes from the brightest/darkest part of what is under it. Result `mix(face, bleed, weight)`.

## Output
`out = mix(shadowPass, bodyPass, c)`; then the SDR holding-tone and the highlight limit: if `Clamp > 0` the colour is limited to a maximum of `Clamp` (scaled to keep the hue when `ClampPreserveHue > 0`, else clamped per channel to [-0.75, Clamp]); finally multiplied by the intensity of the second constant buffer.

## Observed with a real capture (see APPLE-DECISIONS.md / ALGORITHM-FOREGROUND.md)
`regular` = one wide blur and the face term off; `clear` = narrow blur, the rim light and corner marks come out of the refraction bands. Not covered here: the SDF rim-light effect (`CASDFKeyFillHighlightEffect`), the 27.0 additions (`FaceColorMatrixMaxLuma`...), the half-float precision differences.
Unverified: the exact meaning of the polynomial coefficients of the shadow falloff (the shape is a bell curve), the blend (a, b) pair, the sign conventions of the direction; each is read from the translated code and not yet checked against pixels one by one.
