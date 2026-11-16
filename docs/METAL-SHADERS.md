# The Metal shaders of the glass (QuartzCore default.metallib, macOS 26.7.2)

The Apple file is NOT in this repository (it is read from the user's Mac: `/System/Library/Frameworks/QuartzCore.framework/Versions/A/Resources/default.metallib`, AIR, 160 MB, 180 functions). Only names and results are recorded here.

## Functions that make the glass (suffix `_lpf` = float precision, `_lph` = half; same set)
```
displacement_map_lpf downsample_blur_2_frag_lpf downsample_blur_4_frag_lpf downsample_blur_vert_lpf glass_background_lpf glass_background_no_bleed_lpf glass_background_sdf_lpf glass_background_sdf_no_bleed_lpf glass_foreground_lpf glass_foreground_sdf_lpf narrow_blur_11_frag_lpf narrow_blur_15_frag_lpf narrow_blur_19_frag_lpf narrow_blur_23_frag_lpf narrow_blur_27_frag_lpf narrow_blur_7_frag_lpf resolve_blur_weight_frag_lpf sdf_filter_vert_lpf sdf_gen_field_lpf sdf_gen_gradients_lpf single_pass_blur_3_lpf tile_simd_blur_11 tile_simd_blur_15 tile_simd_blur_19 tile_simd_blur_23 tile_simd_blur_27 tile_simd_blur_5 tile_simd_blur_7 variable_blur_clamp_downsample_frag_lpf variable_blur_copy_base_mip_compute variable_blur_downsample_compute variable_blur_downsample_compute_agx2 variable_blur_downsample_frag_lpf variable_blur_frag_lpf variable_blur_vert_lpf 
```
Main ones: `sdf_gen_field`, `sdf_gen_gradients` (build the distance field and its gradient), `displacement_map`, `glass_background[_no_bleed][_sdf]`, `glass_foreground[_sdf]`, `sdf_filter_vert` (the vertex stage), `downsample_blur_vert` / `variable_blur_vert` (blur).

## Experiment: metal2vulkan on Apple's shaders (2026-11-16)
Tool: github.com/steelbrain/metal2vulkan 0.2.0 (GPL-2.0-or-later OR LGPL-3.0-or-later; used only as a tool here, nothing copied), built with nightly-2026-11-03, profile `intel-irisxe-anv`.
| shader (both `_lpf` and `_lph`) | vertex function that pairs with it | result |
|---|---|---|
| `glass_background`, `glass_background_no_bleed`, `glass_foreground`, `displacement_map` | `variable_blur_vert` | SPIR-V written, `spirv-val` vulkan1.2 and 1.4 PASS |
| `glass_background_sdf`, `glass_background_sdf_no_bleed`, `glass_foreground_sdf`, `sdf_gen_field`, `sdf_gen_gradients` | `sdf_filter_vert` | SPIR-V written, `spirv-val` PASS |
| SkyLight `BlurComposite`, `UberCompositeFragment`, `ShadowHorizontalBlurFragment` | their own vertex functions | PASS |
**18 of 18 glass shaders translate and validate** with SPIRV-Tools v2026.4 (v2025.1 rejects the `FMA` capability 6030 the translator emits). The pairing was found by trying every `_lpf` vertex function: only `variable_blur_vert` matched the first group's varyings (the others give `UnmatchedVarying`).
Not done: running them on a GPU, checking pixels against the Mac, real vertex descriptors (mine are placeholders: 8 Float4 attributes), the resource bindings (textures, constants) the effects set up.

## Resource layout of `glass_background_lpf` (read from the translated SPIR-V, 2026-11-16)
| where | what |
|---|---|
| set 0, binding 2 | buffer, array of 84 `uint` (336 bytes) in the vertex stage; array of 21 `uvec4` (336 bytes) in the fragment stage: the same Metal buffer(2), the per-draw constants of the effect |
| set 0, binding 3 | buffer, array of 20 `uint` (80 bytes) |
| set 0, binding 4 / 5 | the sampled 2D float image and its sampler (the background texture) |
| push constants, offsets 64 and 68 | two `uint` (added by the translator for the Metal draw parameters) |
| fragment outputs | location 0 (and 1 in the vertex-to-fragment interface) |
What is in the 336-byte block (the field distance, curvature, height, angle values that `configureLayer:transaction:` records: kind 7, height, curvature, angle, maskOffset) is the next thing to read: the Metal function reads it by byte offset, so the fields come from the shader's loads, not from names.
