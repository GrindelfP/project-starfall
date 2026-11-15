# Approved project context

Starfall has two equal goals: a small playable third-person space shooter and a graphics laboratory for comparing native DirectX 11 and DirectX 12 on the same hardware.

Planned areas are shared simulation/gameplay; separate native DX11 and DX12 backends; appropriate shared math, camera, scene and asset abstractions; a chase camera and a free debug camera; rendering controls, GPU profiling and reproducible benchmarks. No source directory hierarchy or rendering interfaces have been implemented or selected by this bootstrap.

Study topics include buffers, HLSL shaders, constants, textures/samplers, depth/stencil, lighting/shadows, instancing, offscreen rendering/post-processing, compute, synchronization, timestamps, resource management and memory pressure.

Target platform: Windows 11 x64, MSVC C++20, CMake/Ninja, native Win32. No cross-platform abstraction layer or general-purpose engine is required. Initial performance goal: approximately 60 FPS at 1080p, subject to measurement.

Target GPU: NVIDIA GTX 1050 (Pascal), 2 GiB VRAM. Budget resources carefully. Hardware RT cores and Tensor cores are absent; do not plan features requiring them or mesh shaders. Runtime optional capability query results are recorded in the environment document.

Game implementation has not started. The first implementation milestone requires a new instruction: a native Win32 application that creates a DX11 device and swap chain, clears the screen and presents a frame.
