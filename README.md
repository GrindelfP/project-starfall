# Project Starfall

A native Windows C++20 space game and graphics research laboratory using DirectX 11 and DirectX 12.

The first milestone opens a resizable native Win32 window and presents a dark blue DirectX 11 clear on an explicitly selected NVIDIA hardware adapter.

From the repository root, open Windows PowerShell and build both configurations:

```powershell
powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -NoExit
. .\scripts\Enter-DevEnvironment.ps1
cmake --preset debug
cmake --build --preset debug
cmake --preset release
cmake --build --preset release
```

Run from any working directory using the executable path. From the repository root:

```powershell
.\out\build\debug\starfall.exe
.\out\build\release\starfall.exe
# Optional bounded smoke run:
.\out\build\debug\starfall.exe --frames 120
```

For the manual smoke test, run Debug, confirm the visible dark blue window, and close it with the window close button or Alt+F4. Each run overwrites `dx11-diagnostics.log` beside its executable under `out/build/<configuration>/`. Check the adapter, feature level, frame count, clean shutdown and Debug messages. In the live-object report, the device itself is expected while the diagnostic interfaces are still querying it; render targets, swap chain and context should have been released.

For a lifecycle check, run each configuration and repeatedly drag the window edges, maximize and restore it, minimize it for several seconds, then restore it and confirm the clear is visible. Close normally, and repeat with the window minimized (for example, from its taskbar menu). Inspect the diagnostics for each client/viewport size, clean shutdown, and, in Debug, errors and unexpected live graphics objects. The process waits for messages while minimized instead of rendering. The process uses per-monitor-v2 DPI awareness, accepts Windows' suggested rectangle when the monitor DPI changes, and obtains actual client pixels for the swap chain and viewport; the initial 1280 x 720 client request is adjusted at the system DPI and may differ on another monitor.

The optional `starfall.exe --lifecycle-test` changes the window size four times, minimizes for 500 ms while measuring process CPU time, restores, and requests close while minimized. It records the result in the same diagnostics file.

Lifecycle verification on 2026-11-15: Debug and Release built and were manually observed maximizing (client 1920 x 991), restoring (1280 x 720), minimizing, restoring the clear, and closing normally. The internal test exercised four further client sizes (782 x 553 through 1082 x 703), restore, and close while minimized in both configurations; process CPU time was 15.625 ms over the 500 ms minimized interval in each run. Debug reported no messages before its live-object report, which listed only the expected device. Interactive edge dragging and movement between monitors with different DPI have not been manually verified.

Verified on 2026-11-15: Debug and Release builds succeeded; both selected NVIDIA GeForce GTX 1050 (vendor `10DE`, device `1C8D`), feature level `12_1`. A visible Debug window presented 602 frames and closed using Alt+F4; the debug layer reported no messages before its live-object check and only the device during the check. Bounded Debug and Release runs each presented 120 frames and exited successfully. This is a functional smoke test, not a performance measurement.

See [development environment](docs/development-environment.md), [architecture context](docs/architecture.md), [workflow rules](RULES.md), and [session log](docs/session-log.md).
