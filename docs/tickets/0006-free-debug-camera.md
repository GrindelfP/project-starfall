# 0006 - Free-flying debug camera

Name: Free-flying debug camera
Status: implementation complete; manual navigation validation pending
Dependencies: 0005-indexed-3d-scene-and-depth.md.

## Description

Add keyboard translation and mouse-look navigation through the procedural scene. Provide documented controls for movement, a speed modifier, reset pose and mouse capture/release. Keep camera pose and projection data suitable for later reuse by both renderers without defining a broad rendering interface prematurely.

Use the timing/input behavior from 0003 and the coordinate conventions from 0005. Avoid unintended roll, singular vertical views and speed changes caused by diagonal input. Reset must return to a known view of the scene. UI/OS interaction must remain possible after releasing capture or changing focus.

## Acceptance criteria

- Navigate in all intended directions and inspect nearby geometry without unstable orientation.
- Movement over a fixed duration is approximately consistent across presentation rates; normalized diagonal input does not move faster.
- Resize updates the projection, and focus/capture changes cause no orientation jump or stuck motion.
- Reset reliably restores the reference pose.
- Record control bindings and manual validation results; verify camera basis and movement calculations where practical.

## Boundaries

No camera collision, third-party UI dependency, editor, orbit camera or third-person ship controls; those begin in 0007.

## Implementation evidence

Controls and automated coverage are documented in `docs/camera.md`. Interactive navigation, capture and focus behavior remain to be manually validated.
