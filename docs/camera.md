# Free debug camera

The initial and reset pose is the origin, looking along +Z with +Y up, matching the reference scene view. Projection remains the scene's left-handed perspective and is updated by the renderer on resize.

- W/S: forward/backward along the viewing direction; A/D: strafe.
- E/Q: rise/descend along world Y. Shift triples movement speed.
- Right-click to capture the mouse, move it to look, and press Escape to release capture. Focus loss or minimization also releases capture and clears input.
- R resets the position and orientation. Pitch is limited short of vertical, with no roll.

Movement uses elapsed frame time and normalizes combined directions. Mouse movement is relative and is cleared on capture transitions to avoid applying stale deltas.

Automated camera basis, clamp, movement, frame-rate and reset checks are in `tests/camera_tests.cpp`. Debug and Release builds succeeded, and CTest passed all three tests in each configuration, including the camera tests. Interactive navigation, focus and capture validation remain pending; no manual result is claimed.
