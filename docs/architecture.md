# Approved project context

Starfall has two equal goals: a small playable third-person space shooter and a graphics laboratory for comparing native DirectX 11 and DirectX 12 on the same hardware.

The user authorized implementation of the starter tickets on 2026-11-15, including structure and gameplay choices, parallel Terra agents, and at least two local commits per ticket. This session authorization does not authorize a push or change the standing approval rules for future work.

The implementation uses a small native application in `src/main.cpp`, native API resource ownership in backend files, and renderer-independent CPU data in `src/scene.h`. Objects contain transforms and colors; indexed geometry, view matrices and projection conventions can be consumed by either backend. There is no scene graph or general-purpose engine framework. See [scene conventions](scene.md) and [timing/input](timing-input.md).

The next areas in this implementation run are a free camera, arcade ship flight with a chase camera, a seeded asteroid field with ship reset on contact, a separate native DX12 resource path, and reproducible timing measurements. Shared simulation contains no graphics-resource objects. Native API synchronization remains explicit in each backend.

Study topics include buffers, HLSL shaders, constants, textures/samplers, depth/stencil, lighting/shadows, instancing, offscreen rendering/post-processing, compute, synchronization, timestamps, resource management and memory pressure.

Target platform: Windows 11 x64, MSVC C++20, CMake/Ninja, native Win32. No cross-platform abstraction layer or general-purpose engine is required. Initial performance goal: approximately 60 FPS at 1080p, subject to measurement.

Target GPU: NVIDIA GTX 1050 (Pascal), 2 GiB VRAM. Budget resources carefully. Hardware RT cores and Tensor cores are absent; do not plan features requiring them or mesh shaders. Runtime optional capability query results are recorded in the environment document.

The first Win32/DX11 frame, window lifecycle, timing/input, shader pipeline and indexed 3D scene are implemented. Ticket files record their acceptance evidence and any remaining manual-check limitations. Environment-only verification described in the original bootstrap remains historical evidence, not a substitute for application validation.
