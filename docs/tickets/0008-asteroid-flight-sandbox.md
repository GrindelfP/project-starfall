# 0008 - Asteroid flight sandbox

Name: Asteroid flight sandbox
Status: done
Dependencies: 0007-player-ship-and-chase-camera.md.

## Description

Turn the ship-control slice into a small obstacle-navigation sandbox. Generate a bounded, seeded field of procedural asteroid placeholders, leaving a safe initial spawn area. Reuse a small amount of geometry rather than allocating a unique mesh for every asteroid; establish a modest object count appropriate to 2 GiB VRAM.

Add simple, documented collision volumes and an observable ship/asteroid contact response. Obtain approval for the response rule (for example, reset or stop) before implementing it. Keep collision/simulation data independent of GPU resources and record the seed so failures can be reproduced. Include a reset action that restores the same layout and ship pose.

## Acceptance criteria

- The same seed produces the same field and a safe spawn; a different seed changes the layout.
- The ship can navigate the field and contact obstacles with the approved response.
- Check contact, separation and spawn-safety edge cases; document known high-speed collision limitations.
- Reset reproduces the scenario, and switching to the debug camera allows inspection.
- Record object count, approximate owned GPU-buffer allocation and observed frame behavior; do not claim a 60 FPS target without measurement.

## Boundaries

No full physics engine, asteroid destruction, weapons, enemies, scoring, wave system or required instancing optimization yet.

User accepted this ticket on 2026-11-16. Earlier verification notes describe the state before that acceptance.
