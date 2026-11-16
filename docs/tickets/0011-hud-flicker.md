# 0011 - Stop HUD flicker

Name: Stop HUD flicker
Status: implementation in progress; interactive acceptance pending
Dependencies: 0008-asteroid-flight-sandbox.md (existing HUD implementation).

## Description

Reproduce the reported periodic disappearance and reappearance of the sandbox HUD, identify its cause, and keep the panel continuously visible while its contents update. The current Win32 child panel is invalidated every 0.25 seconds and both renderers present to its parent; investigate their interaction rather than assuming the timer is the cause. Preserve readable controls, current status, mouse interaction and benchmark/reference-scene behavior.

## Acceptance criteria

- Manually observe the HUD during sustained sandbox flight on both DX11 and DX12; it remains visible without periodic flicker while status values change.
- Verify resize, minimize/restore, focus changes and DPI changes do not leave the panel missing, clipped or stale; capture/release and camera switching remain usable.
- Document the cause, correction, tested configurations and any remaining limitations. Run the appropriate build and existing tests.

## Boundaries

No general UI framework or visual redesign is required. Obtain approval for any substantive architecture change or substantial dependency under RULES.md.

## Implementation and verification

The GDI child panel shared the parent client area with DXGI presentation. Its periodic invalidation could make the panel visible again after presentation overwrote its pixels. The correction uses a nonactivating owned popup, composed separately from the swap chain, with its position, dimensions and DPI synchronized to the parent client area. The existing text update interval and click-through behavior are retained, and reference and benchmark scenes still create no HUD. This is the suspected cause; visual reproduction and confirmation on both backends remain outstanding.

An initial Debug CTest run passed 5/5; the concurrent Debug build could not complete while the asteroid-contact interface was being changed by another ticket. Rebuild and rerun the suite after integration. Sustained flight, changing status, resize, minimize/restore, focus, DPI, mouse capture/release and camera switching require interactive verification on DX11 and DX12 before closing this ticket. In particular, check positioning during an active window move or resize as well as after it ends.
