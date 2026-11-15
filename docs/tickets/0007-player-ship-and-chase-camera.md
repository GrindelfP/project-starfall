# 0007 - Player ship and chase camera

Name: Player ship and chase camera
Status: implementation complete; manual flight validation pending
Dependencies: 0006-free-debug-camera.md.

## Description

Create the first controllable gameplay slice using a procedural placeholder spaceship and a third-person chase camera. Keep ship state and movement updates independent of DX11 resource objects so the same simulation can later drive DX12. Propose the initial control scheme and flight behavior for user approval before coding; arcade flight versus inertia-heavy flight is a gameplay decision, not an incidental implementation detail.

Implement only the approved movement model, visible orientation feedback and a chase camera with tunable offset. If smoothing is used, make its time behavior explicit. Provide a way to switch between chase and free debug cameras while preserving ship state, and to reset the scene to a known state.

## Acceptance criteria

- The placeholder ship moves and turns according to the approved controls and remains readable from the chase camera.
- Camera switching and reset are deterministic and do not corrupt input capture or simulation state.
- Pause/focus restoration causes no ship teleport or camera lurch.
- Validate movement over fixed elapsed intervals and document the tuning values and controls.
- Debug/Release smoke tests demonstrate the same intended behavior.

## Boundaries

No combat, drone AI, waves, imported ship art, rigid-body physics dependency or camera-obstacle avoidance system.

## Implementation evidence

See `docs/flight.md` for controls, tuning and verification scope. Interactive flight, camera switching, focus transitions and visual readability remain to be manually validated.

