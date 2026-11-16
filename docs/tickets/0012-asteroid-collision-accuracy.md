# 0012 - Correct premature asteroid contacts

Name: Correct premature asteroid contacts
Status: done
Dependencies: 0008-asteroid-flight-sandbox.md (existing collision implementation).

## Description

Reproduce the reported ship respawn when flying close to an asteroid without visible contact. The current test uses a ship sphere of radius 1.9 and enclosing asteroid spheres, which can report overlap outside the visible placeholder geometry. Replace or refine the contact test so it better matches the visible ship and rock shapes while keeping collision data independent of the renderer. Retain the current reset-on-contact response.

## Acceptance criteria

- Reproducible near-miss cases pass without reset, including approaches near the sides and corners at multiple ship and rock orientations; visible impacts still reset the ship.
- Add meaningful automated cases for separated, touching and overlapping shapes, rotations, spawn safety and deterministic fields; document any remaining approximation and tolerances.
- Evaluate movement between frames so the revised test does not introduce avoidable high-speed tunneling; cover the chosen behavior with tests or explicitly document its limitation and follow-up.
- Manually validate near misses and impacts in the sandbox on both backends and record the seed and results.

## Boundaries

Do not add asteroid destruction or change the contact response. No third-party physics engine is required; seek approval for a substantial dependency or architecture change.

## Implementation and verification

Collision uses the visible ship and rock cuboids as oriented boxes, with a 0.00001 world-unit contact tolerance. Each frame sweeps translation continuously using the ship's final orientation. This catches translational tunneling, but does not sweep rotation: a rapidly rotating extremity can pass through a rock between frames. Debug build and CTest passed 5/5 on 2026-11-16. Manual near-miss and impact checks on DX11 and DX12 remain pending; no interactive result is claimed.

User accepted this ticket on 2026-11-16. Earlier verification notes describe the state before that acceptance.
