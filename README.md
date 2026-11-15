# Project Starfall

A native Windows C++20 space game and graphics research laboratory using DirectX 11 and DirectX 12.

The intended game is a small third-person space shooter: fly through an asteroid field, fight drones, avoid obstacles, and survive waves. Shared simulation will support two native rendering backends so graphics behavior and performance can be compared on the same machine.

**Status:** environment foundation only. Game implementation has not started. The root CMake configuration intentionally has no application targets.

```powershell
powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -NoExit
. .\scripts\Enter-DevEnvironment.ps1
cmake --preset debug
cmake --build --preset debug
cmake --preset release
cmake --build --preset release
```

See [development environment](docs/development-environment.md), [architecture context](docs/architecture.md), [workflow rules](RULES.md), and [session log](docs/session-log.md).

The next milestone, requiring a new instruction, is a native Win32 application that creates a DirectX 11 device and swap chain, clears the screen, and presents a frame.
