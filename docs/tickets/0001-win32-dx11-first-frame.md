# 0001 - Win32 and DirectX 11 first frame

Name: Win32 and DirectX 11 first frame
Status: ready
Dependencies: None.

## Description

Deliver the first implementation milestone: a native x64 Win32 application that opens a window, creates a DirectX 11 device and swap chain on the GTX 1050, clears the back buffer and presents frames until closed.

Add an application target to the existing MSVC C++20 CMake foundation. Use the existing Debug/Release Ninja presets. Enumerate adapters and select the NVIDIA hardware explicitly; report the selected adapter and feature level. Use native Win32 and DirectX APIs with deterministic ownership and cleanup. Enable the DX11 debug layer in Debug builds and report initialization failures with useful HRESULT context.

The ticket establishes a minimal vertical slice. Propose any substantive source layout before implementation, following RULES.md. Do not create a renderer abstraction or add external dependencies just to display the first frame.

## Acceptance criteria

- Debug and Release build through the existing presets and produce a runnable Windows application.
- A visible window repeatedly presents a recognizable clear color using the NVIDIA hardware adapter.
- Closing the window exits cleanly, including during partial initialization failure.
- Debug-layer diagnostics show no application-caused errors or live application-owned objects after cleanup.
- README documents exact build/run commands and the manual smoke test, with actual adapter and feature-level evidence.

## Boundaries

No triangle, camera, gameplay, DX12 backend, fullscreen mode or benchmarking. Resizing robustness is handled in 0002. Creating this ticket does not start implementation; select it explicitly before coding.

Status vocabulary for this backlog: `ready` means queued, `process` means actively being implemented, and `done` means acceptance criteria have been verified. A queued ticket's dependencies must be completed before it starts.
