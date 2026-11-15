# 0003 - Frame timing and native input

Name: Frame timing and native input
Status: done
Dependencies: 0002-window-resize-and-lifecycle.md.

## Description

Introduce explicit update timing and native keyboard/mouse input suitable for cameras and ship movement. Use a monotonic high-resolution clock; distinguish held input from press/release transitions. Handle loss of focus so keys do not remain logically held, and provide an explicit mouse-capture release action.

Separate update work from presentation enough to inspect their CPU durations without introducing an engine framework. Bound or reset elapsed time after pauses/minimization so restoration does not produce a large movement jump. Document units, pause behavior and the chosen update policy; obtain approval if it introduces a substantive simulation architecture decision.

## Acceptance criteria

- Diagnostics expose frame delta and basic CPU update/render-submission durations with units; do not label CPU timing as GPU timing.
- A small input diagnostic shows held keys, one-shot transitions and relative mouse movement.
- Alt-tab, focus loss, capture release and minimize/restore produce no stuck input or giant delta step.
- Timing and input code works in both build configurations without additional libraries.
- Tests cover input transitions and pause/delta rules where separable from Win32, supplemented by a manual focus/capture check.

## Boundaries

No controller support, input-remapping UI, simulation framework, FPS claims or GPU performance measurements.

## Implementation and verification

Variable-step timing and reusable input state are implemented; see [timing and input](../timing-input.md). Debug and Release builds and input/timing CTest passed on 2026-11-15. The existing lifecycle test completed in both configurations, including minimize/restore and close while minimized. A visible Debug check confirmed right-click capture, Escape release, W press/release without a stuck held state, and focus loss/recovery with resumed delta below 100 ms. The renderer remained visible and diagnostics recorded clean shutdown after 3392 frames. Holding W continuously through Alt-Tab was not manually exercised; focus clearing is covered by the input-state test.


