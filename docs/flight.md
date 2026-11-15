# Flight controls and tuning

The default `sandbox` scene starts in chase view. W/S move forward/back; A/D yaw; Up/Down pitch; Space/Control move vertically in world space; Shift doubles movement speed. Movement stops immediately on release. C switches to the free camera without changing the ship; in free view W/A/S/D, Q/E, Shift and captured mouse retain the bindings in camera.md. R resets both poses. Right-click captures mouse; Escape releases it. Focus loss and minimization clear input and reset the frame timer.

The ship moves at 6 world units per second (12 with boost), with a yaw/pitch rate of 1.4 radians per second and pitch clamped just short of vertical. Combined thrust and vertical movement are normalized when needed. The chase eye follows instantly, 7 units behind the ship's forward direction and 2.5 units above it, looking at the ship with world up. There is no smoothing, inertia, collision, or obstacle avoidance. Ship cube parts share its orientation.

`--scene reference` retains the original reference cubes and free camera for renderer comparisons. `--scene sandbox --demo --frames 120` runs a repeatable scripted turn using fixed 1/60-second simulation steps for bounded smoke runs; normal flight uses the clamped frame timer. `--backend dx11|dx12` selects the renderer.

Automated ship tests cover elapsed movement, normalized diagonal movement, turn rates, part orientation, chase target, and reset. Interactive controls and visual readability require manual validation on a desktop.
