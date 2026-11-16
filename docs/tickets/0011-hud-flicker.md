# 0011 - Stop HUD flicker

Name: Stop HUD flicker
Status: ready
Dependencies: 0008-asteroid-flight-sandbox.md (existing HUD implementation).

## Description

Reproduce the reported periodic disappearance and reappearance of the sandbox HUD, identify its cause, and keep the panel continuously visible while its contents update. The current Win32 child panel is invalidated every 0.25 seconds and both renderers present to its parent; investigate their interaction rather than assuming the timer is the cause. Preserve readable controls, current status, mouse interaction and benchmark/reference-scene behavior.

## Acceptance criteria

- Manually observe the HUD during sustained sandbox flight on both DX11 and DX12; it remains visible without periodic flicker while status values change.
- Verify resize, minimize/restore, focus changes and DPI changes do not leave the panel missing, clipped or stale; capture/release and camera switching remain usable.
- Document the cause, correction, tested configurations and any remaining limitations. Run the appropriate build and existing tests.

## Boundaries

No general UI framework or visual redesign is required. Obtain approval for any substantive architecture change or substantial dependency under RULES.md.
