# The glass foreground shader (`glass_foreground_sdf`), what it computes

Source of this reading: the AIR function `glass_foreground_sdf_lpf` of `QuartzCore/default.metallib` (macOS 26.7.2), translated to SPIR-V by metal2vulkan and read as GLSL (SPIRV-Cross). **Described here in our own words; the translated shader is not committed (Apple's code).** The parameters are the ones `GlassForegroundFilter::render` writes (see APPLE-DECISIONS.md); the offsets below are into the constant buffer that holds them (the shader reads the same 16-float groups the CPU wrote).

## Inputs
- the shape of the glass as a **signed distance** `d` (negative inside, positive outside) and the **outward direction** `n` of the shape at the pixel, plus a coverage value `w`; they come from the first half of the shader, which is the distance function of the shape (kinds < 4: rounded rectangles with a given corner radius; kind 4: the same with **continuous ("squircle") corners**, the 1.5287 ratio that Apple uses for smooth corners; other kinds: a more general form);
- the background texture, sampled at the pixel position;
- the parameters: refraction (amount A, 1/height, offset O, angle), aberration (amount, 1/height, offset, angle), edge band (start, end, opacity start, opacity end).

## Refraction (the part people ask about)
Let `depth = -d` (distance inside the shape, from the edge).
1. `t = clamp((depth - offset) * (1/height), 0, 1)`: 0 at the edge, 1 at `height` inside.
2. **Profile = a quarter circle:** `displacement = amount * (1 - sqrt(t * (2 - t)))` (the `sqrt(t(2-t))` is a circle arc; clamped to 0..1). It is `amount` right at the edge and falls to 0 at depth `offset + height`, with a vertical tangent at the edge (like a lens edge) and a smooth end.
3. **Direction:** the outward normal `n`, **rotated by the refraction angle** (sin/cos passed in), then mapped by the 2x2 matrix of the layer (so it follows the layer's transform).
4. The sample position is `uv + displacement * direction`. So the background is pulled/pushed along the normal near the edge: the lens effect.

## Chromatic aberration
Computed the same way with its own amount, height, offset and angle: a second displacement vector `a` (profile again the quarter circle). The background is then sampled **several times along `a`**: 3 taps going one way (weights ramping for the red and green channels) and 4 taps going the other way (green and blue), 7 taps in all; red is taken from the `+a` side, blue from the `-a` side, green in between, each channel normalised by the alpha it collected (premultiplied handling: divide by `max(alpha, 1e-6)`). The result is a colour fringe at the edge that vanishes inside, because `a` goes to 0 with depth.

## Edge and output
- `coverage = clamp(0.5 - d / max(fwidth(d), 1e-4), 0, 1)`: anti-aliasing of the shape boundary (derivative based).
- the channel sums are scaled by a fixed `1/7` (the number of taps) and fixed per-channel weights (0.5, 1/3, 0.5), by the foreground intensity from the second constant buffer, and by `w * coverage`;
- the **edge band**: a factor `1 - mix(opacityStart, opacityEnd, s)` with `s` a clamped ramp over `[EdgeStart, EdgeEnd]` measured from the edge: the light band at the rim.

## What is verified, what is not
- Verified: that the shader translates and validates; the structure above is read from the translated code; the buffer offsets match the CPU layout we read (amount 64, 1/height 68, offset 72, aberration 76/80/84, sin/cos 88/92, edge 96/100, edge opacities 104/108, refraction sin/cos 112/116).
- **Run (2026-11-16):** the pair runs on Intel Xe (Vulkan, Mesa 25.2) and on llvmpipe, results equal within 1 byte per channel (`tools/vk-run`, image `docs/img/glass-foreground-intel-xe.png`): the grid of the test background bends towards the rim like a lens, with red/blue fringes on the outline, which is what the algorithm above describes. The shape inputs of this run are my guesses (half extents, kind 4, radius 60 in block 2, curvature flags 0).
- **Not verified:** a comparison with a real Mac capture of the same parameters (so the exact look, the shape-input conventions, the direction signs); one operation (the ramp `s`) is not decoded by SPIRV-Cross ("unimplemented op 4427", an FMA-style op): `s` is the normalised distance between EdgeStart and EdgeEnd by its inputs (`end - start` is computed right before), but the exact expression must be confirmed; the first-half shape kinds >= 5 are not read.

---
# Glass background: first run with the real values (2026-11-16)
`tools/vk-run/gen_bg_block.py` builds the 384-byte block of `glass_background_sdf` from a real capture (`captures/27.0-arm64/regular-340x200-r60.txt`) following the stores of `GlassBackgroundFilter::render` (block starts at Context +0xD0; every offset the shader reads matches a store: checked for 48..304), `run.c` runs it with a mip-mapped background. Image: `docs/img/glass-background-ours-vs-real.png` (left ours on Intel Xe, right the real macOS 27.0 capture, same background and shape).

**Result: it is glass and the shape matches, it is NOT pixel perfect.** Mean absolute difference inside the glass box: 59/255 (outside the box 0.3, i.e. the background itself is identical). What differs:
- ours is far **too bright and too transparent** (looks like the `clear` variant); the real `regular` is darker and clearly frosted (the grid is blurred, not just lifted);
- our **rim light** is a bright white frame with corner marks; the real one is a faint soft edge. The rim light is not part of this filter (it is the SDF `KeyFillHighlight` effect, not run here), so this white frame comes from the filter's own terms (e.g. the bleed/refraction bands) being driven wrongly;
- the blur: our blur uses a box-filtered mip chain at the LOD the shader asks for; Apple's mips/blur surfaces (downsample + blur passes before the filter, `downsample_blur_vert`...) are not reproduced.
Most likely causes, to be tested one by one: (1) the block is inferred (the 36 half-float colour matrices and several scale factors, e.g. the `1.6` and the layer scale, are my reading of the CPU code, with the layer scale and alpha assumed 1); (2) the real glass first renders the backdrop through blur passes; (3) the `Face`/`Bleed`/`Shadow` matrix order and the YCC maths are assumed; (4) macOS 27.0 values on a 26.7 shader (the 27.0 filter has 22 more keys, e.g. `FaceColorMatrixMaxLuma`).
