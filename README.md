# Project Starfall

A native Windows C++20 space game and graphics research laboratory using DirectX 11 and DirectX 12.

The first milestone opens a native Win32 window and presents a dark blue DirectX 11 clear on an explicitly selected NVIDIA hardware adapter. The window has a fixed client size; resize support is a later milestone.

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

Verified on 2026-11-15: Debug and Release builds succeeded; both selected NVIDIA GeForce GTX 1050 (vendor `10DE`, device `1C8D`), feature level `12_1`. A visible Debug window presented 602 frames and closed using Alt+F4; the debug layer reported no messages before its live-object check and only the device during the check. Bounded Debug and Release runs each presented 120 frames and exited successfully. This is a functional smoke test, not a performance measurement.

See [development environment](docs/development-environment.md), [architecture context](docs/architecture.md), [workflow rules](RULES.md), and [session log](docs/session-log.md).
