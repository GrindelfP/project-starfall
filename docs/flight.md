# Flight controls and tuning

The default `sandbox` scene starts in chase view. Right-click captures the mouse; in chase view relative mouse movement controls yaw and pitch (right turns right, up pitches up). Escape releases capture. W/S move forward/back; A/D yaw; Up/Down pitch; Space/Control move vertically in world space; Shift doubles movement speed. Movement stops immediately on release. C switches to the free camera without changing the ship; in free view W/A/S/D, Q/E, Shift and captured mouse retain the bindings in camera.md. R resets both poses. Switching camera modes or resetting discards queued mouse movement. Focus loss and minimization clear input and reset the frame timer.

The ship moves at 6 world units per second (12 with boost), with a keyboard yaw/pitch rate of 1.4 radians per second. Mouse sensitivity is `Ship::mouseSensitivity` (0.0025 radians per raw count); pitch is clamped to ±(π/2 − 0.05) radians. Mouse displacement is applied directly, without a frame-time multiplier. Combined thrust and vertical movement are normalized when needed. The chase eye follows instantly, 7 units behind the ship's forward direction and 2.5 units above it, looking at the ship with world up. There is no smoothing, inertia, collision, or obstacle avoidance. Ship cube parts share its orientation.

`--scene reference` retains the original reference cubes and free camera for renderer comparisons. `--scene sandbox --demo --frames 120` runs a repeatable scripted turn using fixed 1/60-second simulation steps for bounded smoke runs; normal flight uses the clamped frame timer. `--backend dx11|dx12` selects the renderer.

Automated ship tests cover elapsed movement, normalized diagonal movement, turn rates, mouse direction, sensitivity, frame-count independence and pitch limits, part orientation, chase target, and reset. Interactive controls and visual readability require manual validation on a desktop.

Debug and Release configured and built; CTest passed 4/4 in each. Bounded sandbox demo smoke runs presented 30 frames and exited successfully in both configurations on both DX11 and DX12. These runs do not establish visual quality or interactive input behavior.
