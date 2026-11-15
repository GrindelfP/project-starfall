# Initial DX11 / DX12 baseline

Measured 2026-11-15 on NVIDIA GeForce GTX 1050 (device 1c8d), driver 582.28 / Windows 32.0.15.8228, Windows 11 Pro 25H2 build 26200.9974. Release x64, MSVC 19.44.35230, CMake/Ninja; debug layers and GPU-based validation are absent from Release. The window is borderless at a verified 1920x1080 client/viewport, with `Present(0, 0)` (VSync interval zero). Composition or driver behavior can still pace presentation. The fixed reference scene has three indexed cubes (36 indices each), identity view, no HUD or simulation animation. Each run warms up for 2 seconds, then samples 5 seconds; three independent processes per backend. A changed viewport aborts the run. No captures or depth checks run during timing.

From the repository root in PowerShell (dot-source the environment in the fresh build shell):

```powershell
. .\scripts\Enter-DevEnvironment.ps1
cmake --preset release
cmake --build --preset release
.\scripts\Run-Benchmark.ps1 -Backend dx11
.\scripts\Run-Benchmark.ps1 -Backend dx12
python scripts\summarize-benchmark.py out\benchmark\dx11-1.csv out\benchmark\dx11-2.csv out\benchmark\dx11-3.csv out\benchmark\dx12-1.csv out\benchmark\dx12-2.csv out\benchmark\dx12-3.csv > out\benchmark\summary.json
```

The helper runs `starfall.exe --backend dx11|dx12 --scene reference --benchmark out/benchmark/<backend>-<run>.csv`. `--benchmark` fixes the scene, view, resolution, VSync setting and durations. Use a Python 3 interpreter for the summarizer. CSVs and per-run diagnostic logs remain in ignored `out/benchmark/`; a repeated run overwrites that backend's files. Close competing work while measuring. The local results below correspond to the six raw CSVs and logs there.

Frame time is the wall interval between successive frame-loop starts; update covers message processing and simulation through the update timestamp; submission covers the renderer call including Present and any existing fence wait. Consequently these are related intervals, not additive independent CPU costs. GPU timestamps enclose color/depth clears, pipeline setup and three draws, including DX11 constant updates; they exclude DX12 barriers, Present, capture and timestamp readback. DX11 uses a 64-slot timestamp/disjoint ring polled without flushing; disjoint, unavailable and ring-overflow results are blank, not zero. DX12 resolves queue timestamps and reads them after its existing per-frame fence wait, using queue frequency. The current DX12 renderer serializes every frame; this is a correctness path, not an optimized frames-in-flight implementation. Missing final DX11 results are not synchronously drained.

Times are milliseconds. Median uses the conventional midpoint for an even count; tails use nearest rank. Counts are measured frames; GPU missing counts are in parentheses.

| API/run | Frames (GPU missing) | Frame median | Frame p95 | Frame p99 | Update median | Submission median | GPU median |
|---|---:|---:|---:|---:|---:|---:|---:|
| DX11 1 | 785 (1) | 5.1169 | 12.4198 | 13.6926 | 0.0037 | 5.0691 | 0.046080 |
| DX11 2 | 817 (1) | 5.0938 | 12.4797 | 16.3188 | 0.0040 | 5.0467 | 0.046080 |
| DX11 3 | 842 (1) | 5.0944 | 12.6403 | 14.1147 | 0.0042 | 5.0576 | 0.046080 |
| DX12 1 | 299 (0) | 16.6718 | 17.1949 | 17.9544 | 0.0025 | 16.6659 | 0.068608 |
| DX12 2 | 299 (0) | 16.6716 | 18.3790 | 22.8812 | 0.0028 | 16.6674 | 0.068608 |
| DX12 3 | 300 (0) | 16.6654 | 17.6928 | 23.2148 | 0.0028 | 16.6509 | 0.068608 |

Across runs, frame medians range 5.0938–5.1169 ms for DX11 and 16.6654–16.6718 ms for DX12; p99 is more variable. The 16.67 ms DX12 median is consistent with presentation pacing near 60 Hz, but the cause has not been isolated. The reference workload reaches roughly 60 FPS in these runs, though DX12 tails exceed the 16.67 ms frame budget. These numbers do not represent finished gameplay or establish a general API ranking.

At initialization, DXGI reported 2,059,923,456 bytes dedicated physical VRAM, and local budget 1,750,934,939 bytes. Process local usage was 7,565,312 bytes for DX11 and 56,336,384 bytes for DX12. These are dynamic OS accounting snapshots before the first measured frame, not dedicated VRAM or exact owned allocations. As a separate payload estimate, two 1920x1080 RGBA8 swap buffers plus one D24S8 depth image contain 24,883,200 bytes; cube vertex/index data add 168 bytes, and DX11 has an 80-byte constant buffer. Actual allocations, alignment, compression, residency, driver overhead and other resources are excluded, so this is not a VRAM usage measurement.

Separate Debug correctness runs use the DX11 debug layer or DX12 debug layer with GPU-based validation; they are not performance data. The reference images were previously verified pixel-identical between backends. PIX 2610.28 CLI was attempted for a short DX12 capture: `pixtool launch ... --command-line=... take-capture --open save-capture ...` rejected the documented command-line option; attaching to a normally launched process returned PIXTOOL17 (not launched for GPU capture). No PIX capture or hardware counter results were obtained. No driver or Nsight installation was changed.

Next measured task: investigate DX12 presentation pacing and replace the single-frame allocator/fence serialization with measured frames in flight, then repeat this same workload and a separately defined representative sandbox workload.
