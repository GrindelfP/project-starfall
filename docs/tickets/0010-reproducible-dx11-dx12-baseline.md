# 0010 - Reproducible DX11 and DX12 baseline

Name: Reproducible DX11 and DX12 baseline
Status: ready
Dependencies: 0003-frame-timing-and-input.md, 0009-dx12-first-scene.md.

## Description

Establish the first measured comparison using the identical deterministic procedural scene, camera, resolution and workload in both backends. Define warm-up, sample duration, repetition count, presentation/VSync settings and build configuration explicitly. Use Release builds for performance runs and keep debug layers/GPU validation out of timing comparisons; retain separate correctness runs with validation enabled.

Measure CPU frame/update/submission intervals and GPU workload time separately. Use native timestamp/disjoint handling for DX11 and timestamp resolve/readback plus fence-safe access for DX12. Avoid a synchronous GPU readback stall solely to obtain every frame's timing. Report invalid/unavailable samples honestly. Keep metric definitions identical across APIs, including exactly which GPU commands lie inside each measured interval.

## Acceptance criteria

- Document commands/settings to reproduce both backends at 1920x1080 and save raw samples plus a concise comparison report.
- Report sample count, median and tail frame time, GPU time, run-to-run variability, scene/object count, adapter, driver and build details.
- Record GPU memory budget/usage separately from total physical VRAM and owned resource-allocation estimates.
- Check a short correctness capture with PIX for DX12 where supported; explicitly report capture/counter limitations. Do not force an incompatible Nsight release or change drivers.
- State whether the measured workload approaches 60 FPS; avoid extrapolating the small scene to finished gameplay or declaring an API universally faster.

## Boundaries

No benchmark framework dependency, broad GPU compatibility matrix, optimization campaign or full-game performance claim. Use findings to propose the next measured task.
