# Project Starfall

A native Windows C++20 space-flight sandbox and graphics research project with DirectX 11 and DirectX 12 renderers. Fly among a reproducible field of placeholder asteroids; contact resets the ship. This is an early prototype.

Double-click **Play-Starfall.cmd** for the DX11 Release sandbox, or **Play-Starfall-DX12.cmd** for DX12. If the Release build is missing, the launchers print the build commands. They do not build or update files. Close the game with the window close button or Alt+F4.

In chase view, use W/S for thrust, A/D for yaw, Up/Down for pitch, Space/Ctrl for vertical movement and Shift for boost. C toggles the free camera, and R restores the ship, camera and seeded field. In free view, W/A/S/D and Q/E move; right-click captures the mouse for looking and Escape releases it. See [flight controls](docs/flight.md) and [camera controls](docs/camera.md).

From the repository root, build with Windows PowerShell, MSVC x64, CMake and Ninja:

```powershell
powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -NoExit
. .\scripts\Enter-DevEnvironment.ps1
cmake --preset release
cmake --build --preset release
ctest --test-dir out/build/release --output-on-failure
```

Use the `debug` preset and build directory for Debug. The app and compiled shaders live in ignored `out/build/<configuration>/`. Examples:

```powershell
.\out\build\release\starfall.exe --backend dx11 --scene sandbox
.\out\build\release\starfall.exe --backend dx12 --scene sandbox
.\out\build\release\starfall.exe --backend dx12 --scene sandbox --demo --frames 120
.\out\build\debug\starfall.exe --backend dx11 --depth-check
```

DX11 and the sandbox are defaults. `--seed <uint32>` selects a deterministic asteroid field; `--scene reference` displays the shared reference scene. The bounded demo uses fixed simulation steps; it is a smoke run, not by itself a performance measurement. Diagnostics are overwritten beside the executable on each run. [DX12 details](docs/dx12.md) describe capture and synchronization. See the [benchmark report](docs/benchmark-0010.md) for reproducible performance measurements and their limitations.

[Architecture](docs/architecture.md) · [Development environment](docs/development-environment.md) · [Tickets](docs/tickets) · [Session log](docs/session-log.md) · [Historical first-frame validation](docs/first-frame-validation.md)
