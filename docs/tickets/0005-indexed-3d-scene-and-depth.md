# 0005 - Indexed 3D scene and depth

Name: Indexed 3D scene and depth
Status: process
Dependencies: 0003-frame-timing-and-input.md, 0004-dx11-shader-and-triangle.md.

## Description

Render a small procedural scene of indexed cubes with per-object transforms, a perspective view/projection and a depth buffer. Exercise vertex/index buffers, constant-buffer updates, depth testing, culling and viewport changes. Use DirectXMath already supplied by the SDK; do not introduce a separate mathematics package.

Document coordinate handedness, world units, matrix layout, multiplication order, HLSL transfer/transposition rules and depth range consistently. Keep these conventions explicit for later shared simulation and DX12 work; review substantive convention choices with the user under RULES.md. Recreate the depth buffer and projection when the client size changes.

## Acceptance criteria

- Overlapping objects occlude correctly regardless of submission order.
- Distinct translation, rotation and scale are visible; perspective aspect stays correct after resizing.
- Near/far planes and winding/culling behavior are documented and demonstrated.
- Verify representative transform/projection calculations with meaningful checks and a visual reference scene.
- Debug and Release run without invalid constant-buffer bindings or depth-resource diagnostics.

## Boundaries

No external assets, lighting system, textures, shadows or general scene graph. Prefer a tiny deterministic procedural scene.
