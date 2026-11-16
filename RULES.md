# Project rules

- Read this file before other project work.
- Project Starfall is a Windows x64 C++20 game and graphics research project using native Win32, MSVC, CMake/Ninja, DirectX 11 and DirectX 12.
- Shared simulation and appropriate scene/math/camera abstractions will serve two native rendering backends. Avoid portability frameworks and general-purpose engine scope.
- Target hardware: GTX 1050 with 2 GiB dedicated VRAM. The initial 1080p/60 FPS goal requires measurement.
- Bootstrap authorizes foundation and disposable environment checks only. Game, renderer and production shader implementation requires a new instruction.
- Ask before substantive architectural/structure changes, substantial dependencies, overwriting user work, deleting files, security changes, GPU driver changes, or SSH key changes.
- Preserve files. Move retired material to a suitable `legacy/` location with approval where required.
- Never commit credentials, SSH material, or generated binaries. Keep build products in ignored `out/`.
- Agents and subagents may commit and push project work autonomously without separate approval. Preserve unrelated work, inspect remote changes, and use non-destructive Git operations; no force pushes.
- Use commit prefixes `[INIT]`, `[FIX]`, `[BUGFIX]`, `[FEATURE]`, `[STABLE]`, or `[UPDATE]` as appropriate. Fetch and inspect remote changes before finishing; do not overwrite local work.
- Append a concise local-time entry to `docs/session-log.md` at session end. Report installed, compiled, executed, functionally validated, untested and blocked states accurately.
