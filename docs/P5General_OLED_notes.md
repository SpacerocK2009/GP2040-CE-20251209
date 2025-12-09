# P5General OLED pacing notes

## Overview
This change reduces the time the PS5 P5General driver spends stalled by OLED I2C writes while keeping the display usable. Updates are now chunked into smaller page writes, and P5General mode can select an explicit pacing level from the WebConfig UI.

To prevent merge conflicts across related branches, the WebConfig guidance lives in one place with a short summary here and the detailed copy in the locale files. If you refresh those strings, mirror the intent in both languages so downstream cherry-picks stay clean.

## Current state check
- Configuration defaults remain stable: when no value is stored, WebConfig still seeds `p5GeneralOledMode` with the Medium (2) pacing.
- The OLED pacing selector and safe-mode toggle do not affect other input modes; non-P5General paths retain the ~8 ms render cadence.
- No additional blocking paths were found in the render loop—the pacing work stays on the display core and respects the busy/defer checks.

## Scheduling
- The display addon still runs on core1, but P5General mode now stretches the render interval and limits how many SSD1306 pages are flushed per cycle. This caps each I2C burst and spreads a full frame across multiple frames.
- Page flushing restarts whenever the UI changes modes to avoid missing sections of a new screen.
- `p5GeneralOledMode` levels:
  - `Safe` (0): legacy-friendly chunked refresh (2–4 pages every ~32 ms).
  - `Low` (1): ~120 ms cadence with single-page writes for minimum bus occupancy.
  - `Medium` (2, default): ~64 ms cadence with 2-page writes.
  - `High` (3): faster 32 ms cadence with 4-page writes for best responsiveness.
- The legacy `p5GeneralOledSafeMode` flag still increases defer timing during authentication bursts.
- WebConfig now shows concise per-level guidance in English and Japanese to clarify cadence and when to pick each mode.

## I2C execution
- SSD1306 writes are now page-scoped (`drawBuffer(..., pages)`) instead of a full 1 kB transfer. Each call programs one or more pages with addressing commands, then advances the rolling page cursor.
- A reset hook is invoked when the display screen changes so partial paging starts from the top of the framebuffer.

## TinyUSB/P5General interplay
- By shortening each I2C transaction and lowering the refresh cadence in P5General, core0 USB scheduling is less likely to contend with long I2C bursts while authentication traffic is active.
- Busy/authentication defer checks remain in place; safe-mode uses a longer busy defer to avoid overlapping with hash traffic.

## Follow-up ideas
- Consider exposing per-mode page counts in firmware for finer tuning per board.
- Add runtime telemetry (max I2C burst time) to validate improvements on hardware.
- If even lighter load is needed, allow an "event-only" OLED mode that triggers redraws solely on state changes.
