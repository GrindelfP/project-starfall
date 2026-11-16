# 0015 - Flight translation controls and velocity brake

Name: Flight translation controls and velocity brake
Status: in progress (interactive acceptance pending)
Dependencies: Existing mouse orientation and inertial flight implementations in [0013](0013-mouse-ship-orientation.md) and [0014](0014-inertial-ship-flight.md); both accepted by the user.

## Context and expected behavior

The current chase controls use A/D to yaw and offer reverse thrust but no dedicated brake. The user requests translation on WASD, arrow-key rotation alongside the mouse, and a brake that slows absolute velocity to a stop. Preserve inertial flight: releasing thrust does not itself cancel velocity.

## Scope

- In chase view, W/S apply forward/reverse thrust and A/D apply left/right thrust relative to the ship's heading. Space/Ctrl continue to apply up/down thrust; retain the existing world-space vertical convention unless a later decision changes it. Translation changes velocity rather than directly rotating or teleporting the ship.
- Left/Right arrows yaw and Up/Down arrows pitch in the same directions as mouse steering. Keep mouse capture/release and pitch limits; keep free-camera controls separate and unchanged. Arrow rotation remains time-based; relative mouse displacement remains independent of frame time.
- While B is held, braking reduces the magnitude of the ship's world-space velocity regardless of ship orientation or travel direction, reaching zero without overshoot or reversing velocity due to the brake. Releasing B stops applying the brake. Use a documented, tunable deceleration; define and document how simultaneous thrust and braking combine, ensuring braking alone always stops the ship.
- Update the HUD, [flight documentation](../flight.md), and README controls to describe the implemented behavior. Adapt the scripted demo as necessary to remain deterministic and keep simulation independent of the rendering backend.

## Acceptance criteria and verification

- A/D produce left/right translation without yaw; W/S and Space/Ctrl act in their stated directions. Combined inputs have a documented magnitude rule and no unintended faster diagonal acceleration. Test a rotated ship and combined axes.
- Both mouse and arrows change orientation in the expected directions; arrow keys do not translate the ship on their own. Capture, mode switching, focus loss, minimization and reset do not introduce stuck input or jumps.
- Automated tests cover braking from forward, backward, lateral, vertical and mixed velocities, including zero and low speed, stopping without reversal, release, frame-step consistency and the documented interaction with simultaneous thrust. Reset and collision respawn still clear velocity.
- Build and run the matching Debug and Release CTest suites. Record interactive checks of translation, arrows, braking and HUD on both DX11 and DX12, along with any unresolved acceptance gaps.

## Boundaries

No roll control, full rigid-body simulation, physics-engine dependency or automatic change to free-camera bindings. Keep tuning values explicit and adjustable. Existing HUD and collision tickets' independent acceptance remains tracked separately.

## Implementation and verification

Implemented heading-relative lateral thrust, arrow-key yaw and pitch, and a world-space B brake. Combined thrust is normalized; the brake applies 6 units/s² after thrust and clamps at zero. See [flight controls](../flight.md) for tuning and integration details. Debug and Release builds and CTest passed 5/5 each on 2026-11-16. Bounded Release runs of the named DX11 and DX12 executables verified their backend defaults. Interactive checks of translation, arrows, braking, HUD and input lifecycle remain pending on both backends; automated and bounded runs do not establish those behaviors visually.
