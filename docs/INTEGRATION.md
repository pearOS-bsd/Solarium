# Integration: Liquid Glass in pearOS exactly as on macOS

Goal (owner, 2026-11-16): applications that need Liquid Glass use it **directly, through the same API path as on macOS**; our window server renders it. No special pearOS API.

## The path on macOS (read from the real system, docs/REAL-GLASS-VALUES.md)
`NSGlassEffectView` (AppKit) -> a SwiftUI `SDFLayer` -> children: a content layer with `vibrantColorMatrix` filters, a `CAPortalLayer` (the backdrop), a **`CASDFLayer`** whose layers carry the **`glassBackground` CAFilter** (55 keys on 26.7, 77 on 27.0) and the SDF effects (`CASDFKeyFillHighlightEffect`, `CASDFOutputEffect`), an `SDFPortalLayer`. The layers are committed to the render server (WindowServer), which decodes them and runs the shader `glass_background_sdf` (QuartzCore `default.metallib`).

## Our layers and the work in each (lowest layer first)
| step | repository | what | state |
|---|---|---|---|
| 1 | QuartzCore (client API) | `CAFilter` types `glassBackground` / `glassForeground` with the real keys, order and atoms; differential test vs the real QuartzCore | **done** (2026-11-16, identical on FreeBSD) |
| 2 | QuartzCore (client API) | `CASDFLayer`, `CASDFElementLayer`, the `CASDF*Effect` classes with the real names and defaults; differential test vs the real QuartzCore | **done** (2026-11-16; `CAPortalLayer` still to do) |
| 3 | WindowServer (render stream) | decode filters with atom keys and the SDF layers in the command stream (`ca_scene.c`); formats read in docs/RENDER-STREAM.md | formats **read**, decoder not extended yet (needs a real stream with a glass filter to test) |
| 4 | WindowServer | render the glass filter in `compositor_gl.c` (GLES 2) and the CPU fallback `blur.c`, replacing the fitted `ws_glass` approximation: the shader is our own GLSL ES file, written from docs/ALGORITHM-*.md, same constant block layout as Apple's; mip chain of the backdrop | to do |
| 5 | AppKit | `NSGlassEffectView` / `NSGlassEffectContainerView` building the same layer tree | to do |
| 6 | tests | the captures of `tools/glass-probe` as the oracle: render the same scene through the whole path and compare with `captures/<ver>/*.png` within a tolerance (CI) | to do |
The shaders live in **one file in this repository** (`shaders/`), with the same function names and the same constant-block layout as Apple's `default.metallib`, so that on a Mac with Apple's file the two can be compared A/B and the Apple one can be dropped in. Apple's own file is never in a repository.
