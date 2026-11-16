# 0013 - Mouse-controlled ship orientation

Name: Mouse-controlled ship orientation
Status: ready
Dependencies: 0003-frame-timing-and-input.md, 0007-player-ship-and-chase-camera.md (existing input and ship implementation).

## Description

Use relative mouse movement to control the ship's orientation in chase mode. Establish a clear control scheme for capture/release, yaw and pitch, sensitivity, keyboard alternatives and free-camera interaction before implementation; the user has requested mouse steering, but exact bindings and tuning remain to be decided. Keep ship orientation shared by both renderers and make the chase camera follow it.

## Acceptance criteria

- Captured relative mouse motion turns the ship predictably in chase mode; the visible ship, forward direction and chase view agree. Sensitivity and pitch limits are documented and tunable.
- Free-camera mouse look remains functional; switching modes, capture/release, focus loss, minimization and reset cause no stale-input jump or stuck input.
- The HUD and flight documentation accurately describe the chosen controls. Verify both renderers through interactive flight and record results.
- Add focused checks for orientation math and input transitions where practical; mouse displacement must not be multiplied by frame time a second time.

## Boundaries

No combat, targeting or required roll control. Coordinate the control scheme with 0014; do not silently decide unresolved gameplay bindings.
