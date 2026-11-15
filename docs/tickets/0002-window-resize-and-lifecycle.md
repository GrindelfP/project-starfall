# 0002 - Window resize and graphics lifecycle

Name: Window resize and graphics lifecycle
Status: done
Dependencies: 0001-win32-dx11-first-frame.md.

## Description

Make the first-frame application reliable during ordinary desktop use. Handle client-area changes by releasing back-buffer references, resizing swap-chain buffers and rebuilding size-dependent views and viewport state. Treat zero-sized/minimized windows explicitly; avoid attempting invalid buffer sizes or busy-spinning while minimized.

Handle focus changes and orderly shutdown. Report device-removal failures with the removal reason and terminate safely; automatic recovery is not required for this starter milestone. Keep rendering resolution tied to the actual client size and document the initial DPI behavior rather than assuming window dimensions equal client pixels.

## Acceptance criteria

- Repeatedly resize, maximize, restore and minimize the window without crashes, invalid views or debug-layer errors.
- Rendering resumes after restoration; minimized CPU activity is bounded rather than a busy loop.
- Close works from normal, resized and minimized states.
- Initial and resized client dimensions are observable in diagnostics and match the viewport.
- Record a reproducible manual lifecycle checklist and its Debug/Release results.

## Boundaries

No exclusive fullscreen, multi-window support, automatic device recovery or generic window framework. Extend depth-buffer resizing later when 0005 introduces depth.
