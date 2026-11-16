# vk-run

Runs the glass foreground shader pair (vertex `sdf_filter_vert_lpf` + fragment `glass_foreground_sdf_lpf`, from the user's own Mac's `QuartzCore/default.metallib`, translated to SPIR-V by metal2vulkan; **the shaders are not in this repository**) offscreen on a Vulkan device, with the Apple default parameters of `docs/APPLE-DECISIONS.md`, over a synthetic background.

    gcc -O1 -o vkrun run.c -lvulkan -lm
    metal2vulkan QuartzCore-default.metallib fg.spv --profile laptop-profile.json \
        --function sdf_filter_vert_lpf --fragment glass_foreground_sdf_lpf \
        --color-format 0:RGBA8Unorm --vertex-descriptor vd3.json
    ./vkrun fg.vert.spv fg.frag.spv 0 4      # device 0, kind 4 = squircle

`laptop-profile.json` is the repository profile `intel-irisxe-anv.json` without `VK_KHR_shader_fma` (Mesa 25.2 does not have it yet); `vd3.json` is the vertex descriptor: attribute 0 Float4 at 0 (clip position), 1 Float2 at 16 (shape-local coordinates in pixels), 2 Float2 at 24 (uv), stride 32. Specialization constants: vertex id 1 = true; fragment id 0 = 0, ids 2 and 3 = true (fixed buffer offsets).
Result of 2026-11-16: `docs/img/glass-foreground-intel-xe.png` (Intel Xe; llvmpipe gives the same within 1 byte per channel).
