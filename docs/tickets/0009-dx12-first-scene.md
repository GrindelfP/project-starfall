# 0009 - Native DX12 first scene

Name: Native DX12 first scene
Status: ready
Dependencies: 0005-indexed-3d-scene-and-depth.md. Can proceed independently of 0006-0008 after prerequisites.

## Description

Introduce a native DirectX 12 path in two verifiable steps: first clear/present, then render the same deterministic indexed scene used by DX11. Before implementation, present the smallest practical backend-selection and shared-scene boundary for approval. Reuse CPU scene/math data where appropriate while leaving native API resource management visible; do not build a general rendering hardware abstraction.

Create device, queue, swap chain, command allocators/lists, render-target/depth descriptors, root signature and graphics PSO. Use DXC-generated DXIL with a supported Shader Model 6 target. Manage resource transitions, upload lifetimes, fences and frame reuse explicitly. Select the GTX 1050 and enable the debug layer/GPU-based validation for correctness runs. Handle resize and shutdown after outstanding GPU work safely completes.

## Acceptance criteria

- A documented backend choice launches either DX11 or DX12 against the same reference scene and camera pose.
- DX12 clear/present and indexed geometry both work in Debug/Release without application-caused validation errors.
- Resize, minimize/restore and shutdown safely handle in-flight work and resource lifetimes.
- Shader build errors are actionable; DX11 SM5 bytecode and DX12 DXIL paths stay explicit.
- Document command submission, barriers, frame ownership and synchronization with a small diagram or explanation.

## Boundaries

No async compute, bindless system, multithreaded command recording, raytracing, mesh shaders or optimization before correctness. This ticket does not require porting the gameplay sandbox.
