# 0014 - Inertial heavy-ship flight

Name: Inertial heavy-ship flight
Status: ready
Dependencies: 0007-player-ship-and-chase-camera.md (existing ship implementation); coordinate controls and orientation with 0013-mouse-ship-orientation.md.

## Description

Replace immediate, inertialess translation with a controllable heavy ship that accelerates under thrust and retains velocity after thrust is released. Agree the intended flight model and controls before coding: thrust directions, braking or flight assist, boost, speed limits, rotational response and tuning remain design choices. Define any damping or artificial speed limit explicitly rather than presenting them as inevitable space physics. Keep the simulation independent of rendering and avoid a physics-engine dependency unless approved.

## Acceptance criteria

- Ship velocity changes gradually under thrust and persists on release according to the documented model; turning does not implicitly rotate existing world-space velocity unless the chosen assist applies a documented force.
- Braking, reverse/vertical thrust and boost behave according to the agreed controls. Reset and asteroid respawn restore a known pose and velocity; focus loss and camera switching do not cause unexpected impulses.
- Motion over equal elapsed time remains reasonably consistent across frame rates and bounded time steps. Cover acceleration, coasting, turning, braking and reset with meaningful deterministic tests.
- Assess collision behavior at attainable speeds, including tunneling, and coordinate any necessary follow-up with 0012. Manually validate handling and chase-camera readability in both backends.
- Document units, equations, tuning values, controls and any assists or limitations; update the HUD to match.

## Boundaries

No claim of full spacecraft or rigid-body simulation; no gravitational model, aerodynamic flight, damage system or third-party physics engine is implied by this ticket. Obtain approval for substantive architecture changes or dependencies.
