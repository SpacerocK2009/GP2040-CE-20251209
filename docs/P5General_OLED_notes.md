# P5General OLED notes

## Current behavior
P5General no longer has dedicated OLED pacing or safe-mode settings in WebConfig or the saved display options. OLED refresh uses the common `DISPLAY_FRAME_INTERVAL_MS` cadence for every input mode, including P5General.

Boards that need a different display cadence should override `DISPLAY_FRAME_INTERVAL_MS` at build time or in the board definition before `headers/addons/display.h` provides its default.

## Removed options
The following P5General-only display options were removed from the active configuration and WebConfig payloads:

- `disableWhenP5General`
- `p5GeneralOledSafeMode`
- `p5GeneralOledMode`

These options are no longer read from or written to `/api/getDisplayOptions` and `/api/setDisplayOptions`. Existing stored values are ignored by current firmware.

## Validation
The display add-on gates redraws only through the shared frame interval check, so P5General adapters receive the same OLED update cadence as other modes. Hardware validation with a P5General adapter is still required before release because this environment cannot attach USB adapters or observe PS5 authentication state.
