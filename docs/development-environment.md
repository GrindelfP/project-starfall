# Development environment

Inventory date: 2026-11-15, Europe/Moscow. This is a native Windows setup, not a cross-platform build.

## Machine and tools

- Windows 11 Pro 25H2, x64, build 26200.9974 after restart (initial inventory: 26200.9457).
- Intel Core i5-8250U; 7.88 GiB usable RAM. Initial free RAM was about 1.19 GiB, so keep profiling workloads modest.
- C: initial free space 40.91 GiB (238.36 GiB volume). Installation consumes additional space.
- NVIDIA GeForce GTX 1050, 2,048 MiB dedicated VRAM, driver 582.28 (Windows driver 32.0.15.8228). An Intel UHD Graphics 620 adapter is also present; diagnostics explicitly select the NVIDIA device.
- PCI vendor/device `10DE:1C8D`, subsystem `11E11043`, revision `A1`, PCI bus `00000000:01:00.0`.
- Initial Windows PowerShell 5.1.26100.9444; post-reboot Codex shell reports PowerShell 7.6.9. Verification scripts explicitly use Windows PowerShell via `powershell.exe`. Winget 1.29.380.
- Git for Windows 2.56.0.windows.4. Git identity was supplied by the user; Windows OpenSSH is selected globally for Git transport.

- Visual Studio 2022 Build Tools 17.14.45, complete stable installation; MSVC toolset directory 14.44.35207, compiler 19.44.35230 for x64.
- Windows 11 SDK 10.0.26100.0. Verified `d3d11.h`, `d3d12.h`, `dxgi1_6.h`, `d3dcompiler.h`, `DirectXMath.h` and x64 `d3d11.lib`, `d3d12.lib`, `dxgi.lib`, `d3dcompiler.lib`, `dxguid.lib`.
- Bundled CMake 3.31.6-msvc6 and Ninja 1.12.1, available after the environment helper.
- SDK DXC reports dxcompiler 1.9.2602.17. No legacy DirectX SDK or CUDA Toolkit was installed.
- Microsoft PIX 2610.28 installed; `pixtool` help executed successfully. Full capture/profiling remains untested.
- WinDbg 1.2611.4001.0, debugger engine 10.0.29680.1004. The x64 console debugger loaded the diagnostic PDB, hit `starfall_verify!main`, displayed the symbolic stack and ran the program to completion.
- Windows Graphics Tools `Tools.Graphics.DirectX~~~~0.0.1.0`: independently verified `Installed` after the user restarted Windows. Both DirectX debug layers now load successfully.

## Verified graphics results

The disposable MSVC C++20 executable compiled, linked and ran successfully, including ranges/concepts and DirectXMath. CTest passed 1/1. Both repository presets configured and built successfully; there are intentionally no game targets.

| Probe | Result |
| --- | --- |
| NVIDIA adapter selection | Exact detected PCI device `10DE:1C8D`, no WARP fallback |
| DXGI dedicated memory | 2,059,923,456 bytes (~1,964.5 MiB); NVIDIA physical memory report is 2,048 MiB |
| DX11 device | Created successfully, feature level 12_1 |
| DX12 device | Created successfully; maximum feature level 12_1; 12_2 rejected |
| DX12 shader model | 6.8, querying 6.9 downward |
| DX11 debug device / DX12 debug layer | Both succeeded after reboot, HRESULT `0x0` |
| DX12 GPU-based validation | Interface query succeeded (`0x0`); enabled before device creation; no GPU workload tested |
| DX12 resource binding / heap tiers | Tier 3 / tier 1 |
| DX12 tiled resources | Tier 4 reported by runtime |
| Typed UAV additional formats | Supported |
| DXR / mesh shaders / sampler feedback | Not supported (tiers 0) |
| DX11 SM5 shader path | `D3DCompile` `vs_5_0`/`ps_5_0`, then native shader object creation: passed |
| DX12 SM6 shader path | DXC `vs_6_0`/`ps_6_0`, DXIL container inspection: passed |
| DX11 timestamp query | Query object creation passed; no measurement taken |
| DX12 direct queue timestamps | Frequency query passed, 1,000,000,000 ticks/sec; no measurement taken |

DX12 PSO creation, GPU draw/dispatch execution, synchronization behavior under load, swap chains, frame presentation, actual timestamp measurements, profiler captures and performance targets are not tested. Shader compilation does not prove those behaviors.

The initial sandboxed Debug compile failed with MSVC C1902 during PDB creation. The same compiler check and full diagnostics succeeded outside the sandbox, with matching PDB DLL/server versions; this is an execution-environment restriction, not evidence of a broken toolchain. Codex Debug build commands may need explicit sandbox escalation; do not disable PDBs to hide the issue.

The core diagnostic's successful exit permits absent optional debug layers. The post-reboot run separately verified successful DX11 debug device creation, DX12 debug-layer acquisition and GPU-validation-interface acquisition. GPU-based validation was enabled before device creation, but no GPU workload is submitted by this probe. CTest passed 1/1 again after reboot, as did both shader compiler paths.

## PowerShell build workflow

Open PowerShell in the repository, then launch a child shell with the approved process-only policy. This does not change user/machine execution policy:

```powershell
powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -NoExit
. .\scripts\Enter-DevEnvironment.ps1
cmake --preset debug
cmake --build --preset debug
cmake --preset release
cmake --build --preset release
cmake --build --preset debug --target clean
cmake --build --preset release --target clean
```

The helper discovers the current complete MSVC installation via `vswhere`, runs `VsDevCmd.bat` for x64 host/target, and imports its environment only into the current process. It exposes the bundled CMake/Ninja and Windows SDK tools. Repeat it for every new shell used by Codex. A Codex restart may be needed for the Git install's PATH change; its absolute executable is under `$env:ProgramFiles\Git\cmd\git.exe`.

There are no application targets or in-repository tests yet. Configuring and building this foundation checks toolchain discovery; an empty build is expected and does not prove gameplay or rendering.

## Disposable verification

Local machine diagnostics are outside the source tree at:

```powershell
$diag = Join-Path $env:USERPROFILE '.codex\visualizations\2026\11\15\01a1e063-d9a9-74e0-aff6-6e0200a50229\starfall-bootstrap'
powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -File "$diag\Run-Verification.ps1" -ProjectRoot (Get-Location).Path
```

This local-only diagnostic directory is not a portable repository dependency. It contains the C++20/DXGI/DX11/DX12 probe, trivial HLSL, CMake build, script, inventory and local result logs. Keep it while reproducing bootstrap checks; it is excluded from commits by residing outside the checkout. Generated diagnostic outputs are under its `out/` directory. To clean compiled objects without deleting evidence:

```powershell
cmake --build "$diag\out" --target clean
```

To reproduce the native debugger breakpoint check before cleaning:

```powershell
$dbg = Get-AppxPackage -Name Microsoft.WinDbg
& (Join-Path $dbg.InstallLocation 'amd64\cdb.exe') -y "$diag\out" -c 'bp starfall_verify!main;g;k;g;q' "$diag\out\starfall_verify.exe"
```

The script compiles and executes MSVC C++20 ranges/concepts and DirectXMath, enumerates adapters, creates devices on the exact detected NVIDIA adapter, probes optional features and debug interfaces, compiles/creates DX11 SM5 shaders with `D3DCompile`, and separately compiles/inspects DX12 SM6 DXIL with SDK DXC. No production shader is added to the project. It does not create a window/swap chain, present a frame, submit a draw, validate a DX12 PSO, or capture a profiling workload.

## Git and SSH workflow

Remote: `git@github.com:GrindelfP/project-starfall.git`. Both this repository and `git@github.com:GrindelfP/grindelf-skills.git` were accessible over SSH.

A user-approved dedicated Ed25519 key without a passphrase is selected for `github.com` by the user's SSH config, with `IdentitiesOnly yes` and strict host checking. GitHub's published Ed25519 host key was pinned and its fingerprint checked. The private key stays in the user SSH directory, never in this repository. No SSH agent is needed for this key, so the disabled agent service was preserved.

```powershell
git status --short --branch
git diff
git diff --cached
git fetch origin
git switch -c feature/describe-change
# Once remote main has its first commit, and the working tree is clean:
git pull --ff-only origin main
# Only after separate user approval of the exact commit:
git commit -m '[UPDATE] describe change'
# Only after separate user approval to publish:
git push -u origin main
```

The remote was empty at bootstrap; no pull is possible until it has a commit. Never force-push or rewrite history.

## Private skills

Reusable checkout: `$env:USERPROFILE\Projects\grindelf-skills`, SSH origin, initial inspected commit `ac258575fb6fdd513376e25c560f1c3818e2cbb3`.

With user approval, Windows directory junctions connect its six `grindelf-*` directories to `$env:USERPROFILE\.codex\skills`. This adapts the repository's documented OpenCode link installation to Codex while preserving a single updateable source. Existing skills were preserved. To update, inspect changes and then run `git -C "$env:USERPROFILE\Projects\grindelf-skills" pull --ff-only`; review skill instructions before using changed workflows.

Installed links: `grindelf-begin`, `grindelf-check-hw`, `grindelf-finalize`, `grindelf-project-bootstrap`, `grindelf-tex-compile`, `grindelf-translate-ruen`. The user confirmed `grindelf-project-bootstrap` appears in Codex. Its installed instructions were read and applied. `grindelf-initialize` does not exist in the repository; the user explicitly approved the bootstrap replacement. Do not claim that the absent skill was executed.

## Limitations and references

- The restart and required debug-interface checks are complete. Workload validation and profiler captures are deferred until a graphics application exists.
- 2 GiB VRAM is a firm resource constraint; 1080p/60 FPS remains an unmeasured goal.
- Pascal has no hardware RT or Tensor cores. Optional API capabilities must be queried; DirectX 12 installation alone proves none of them.
- Current Nsight Graphics removed Pascal support. An older release requires separate compatibility and download-access verification; an incompatible current version is not installed. [NVIDIA 2026.1 release notes](https://developer.nvidia.com/nsight-graphics/getting-started/release-note-v2026.1), [older release](https://developer.nvidia.com/nsight-graphics-2025_3).
- PIX supports native x64 DX12 captures; DX11 capture requires a D3D11On12 path. Full capture, counters, shader disassembly and actual GPU timing are not proven by device creation. [PIX requirements](https://devblogs.microsoft.com/pix/requirements/).
- No NVIDIA driver, global execution policy, Windows security feature, or unrelated installed software was changed.

Official setup references: [Microsoft Build Tools components](https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools?view=vs-2022), [GitHub host fingerprints](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/githubs-ssh-key-fingerprints).
