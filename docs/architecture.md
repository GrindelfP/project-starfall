# Project architecture

Starfall combines an early third-person space-flight sandbox with a native DirectX 11/DirectX 12 graphics laboratory. The shared CPU simulation and procedural scene feed both renderers; no general-purpose engine or portability framework is used.

`src/main.cpp` owns the Win32 window, input, timing, camera selection and application loop. `camera.h`, `ship.h` and `asteroid_field.h` hold renderer-independent state. `scene.h` defines shared indexed cube geometry, object transforms and the left-handed view/projection conventions. The sandbox renders a ship, 64 seeded asteroids and simple landmarks; swept cuboid contact resets the ship. The reference scene is retained for comparisons. See [scene](scene.md), [flight](flight.md), [camera](camera.md), and [timing and input](timing-input.md).

`render_backend.h` selects native DX11 or DX12 implementations. Each backend owns its API resources and consumes the same scene objects and camera. DX11 uses its SM5 shader; DX12 compiles SM6 shaders with the Windows SDK `dxc`, uses explicit resource transitions and waits on a fence before reusing its allocator. The current DX12 path favors correctness over throughput; see [DX12](dx12.md). Depth, resize and shutdown are handled per backend. Debug graphics validation and captures are correctness tools, not performance measurements.

Target platform: Windows 11 x64, MSVC C++20, CMake/Ninja, GTX 1050 with 2 GiB physical VRAM. The approximately 60 FPS at 1080p goal requires measured evidence; physical capacity, operating-system budget and owned allocations are distinct. The GPU does not have hardware ray tracing or Tensor cores and does not support mesh shaders. Environment and capability evidence is in [development environment](development-environment.md).

Ticket acceptance and remaining interactive validation are tracked in [tickets](tickets). Historical bootstrap verification is not a substitute for application validation. Future research may explore textures, lighting, instancing, post-processing, compute, synchronization and memory pressure, guided by measurements rather than an assumed API advantage.
