# Agent guidance

Read [RULES.md](RULES.md) first. It contains the user's approval requirements.

- The repository now contains the native `starfall` application, a DX11 renderer, versioned HLSL and a shared procedural scene. Read the current handoff and ticket status before extending it.
- Use Windows PowerShell, MSVC x64, C++20, CMake and Ninja. Dot-source `scripts/Enter-DevEnvironment.ps1` in every fresh shell before CMake commands.
- Configure/build with `cmake --preset debug` / `cmake --build --preset debug`, or the corresponding `release` presets. Run the CTest suite from the matching build directory.
- Keep outputs in ignored `out/`. See `docs/development-environment.md` for verification evidence and diagnostic locations.
- Architecture context is in `docs/architecture.md`. Append session handoffs to `docs/session-log.md` in Europe/Moscow time.
- Private skills live in a separate user checkout. `grindelf-project-bootstrap` was explicitly approved as the replacement for the absent `grindelf-initialize`. User approval rules override any skill's automatic commit/push cadence.
