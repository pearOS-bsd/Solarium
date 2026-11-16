# The Metal shaders of the glass (QuartzCore default.metallib, macOS 26.7.2)

The Apple file is NOT in this repository (it is read from the user's Mac: `/System/Library/Frameworks/QuartzCore.framework/Versions/A/Resources/default.metallib`, AIR, 160 MB, 180 functions). Only names and results are recorded here.

## Functions that make the glass (suffix `_lpf` = float precision, `_lph` = half; same set)
```
displacement_map_lpf downsample_blur_2_frag_lpf downsample_blur_4_frag_lpf downsample_blur_vert_lpf glass_background_lpf glass_background_no_bleed_lpf glass_background_sdf_lpf glass_background_sdf_no_bleed_lpf glass_foreground_lpf glass_foreground_sdf_lpf narrow_blur_11_frag_lpf narrow_blur_15_frag_lpf narrow_blur_19_frag_lpf narrow_blur_23_frag_lpf narrow_blur_27_frag_lpf narrow_blur_7_frag_lpf resolve_blur_weight_frag_lpf sdf_filter_vert_lpf sdf_gen_field_lpf sdf_gen_gradients_lpf single_pass_blur_3_lpf tile_simd_blur_11 tile_simd_blur_15 tile_simd_blur_19 tile_simd_blur_23 tile_simd_blur_27 tile_simd_blur_5 tile_simd_blur_7 variable_blur_clamp_downsample_frag_lpf variable_blur_copy_base_mip_compute variable_blur_downsample_compute variable_blur_downsample_compute_agx2 variable_blur_downsample_frag_lpf variable_blur_frag_lpf variable_blur_vert_lpf 
```
Main ones: `sdf_gen_field`, `sdf_gen_gradients` (build the distance field and its gradient), `displacement_map`, `glass_background[_no_bleed][_sdf]`, `glass_foreground[_sdf]`, `sdf_filter_vert` (the vertex stage), `downsample_blur_vert` / `variable_blur_vert` (blur).

## Experiment: metal2vulkan on Apple's shaders (2026-11-16)
Tool: github.com/steelbrain/metal2vulkan 0.2.0 (GPL-2.0-or-later OR LGPL-3.0-or-later; used only as a tool here, nothing copied), built with nightly-2026-11-03, profile `intel-irisxe-anv`.
| shader | result |
|---|---|
| SkyLight `BlurComposite`, `UberCompositeFragment`, `ShadowHorizontalBlurFragment` | SPIR-V written, `spirv-val` vulkan1.2 and 1.4 PASS |
| QuartzCore `sdf_gen_field`, `sdf_gen_gradients` | written, `spirv-val` PASS |
| QuartzCore `glass_background_sdf`, `glass_foreground_sdf` (with `sdf_filter_vert`) | written; `spirv-val` of this machine (v2025.1) rejects capability 6030 (`FMAKHR`, newer than the validator), so run with `--no-spirv-val`: NOT validated |
| `glass_background`, `glass_foreground`, `displacement_map` | "UnmatchedVarying": the vertex function I paired them with is not theirs (6 candidates tried); the right pair is not found yet |
So Apple's own glass shaders can be turned into Vulkan SPIR-V. Not done: running them on a GPU, checking the pixels against the Mac, the exact vertex descriptors (mine are placeholders: 8 Float4 attributes).
