# P5General latency investigation notes

## Report pipeline

1. Core0 `P5GeneralDriver::process()` converts the current `gamepad->state` into a `P5GenerorReport`.
2. Changed reports are copied into `P5GeneralAuthData::hash_pending_buffer` and marked `hash_pending`.
3. Core1 `P5GeneralAuthUSBListener::process()` sends `hash_pending_buffer` to the P5General auth dongle and marks the request `hash_in_flight` until the dongle response arrives.
4. Core1 `report_received()` copies the signed response into `hash_finish_buffer` and marks `hash_ready`.
5. Core0 `P5GeneralDriver::process()` sends `hash_finish_buffer` to the PS5 via `tud_hid_report()`.

## Suspected cause and mitigation

The old Core0 path returned early while a report was pending, so an LK-only report could remain the active signed report while the next LP+LK state was not retained.  The updated path always rebuilds the latest P5General report first.  If signing is busy, the newest observed report is stored as a deferred report and overwrites older deferred intermediate states.  As soon as the previous signed report is sent, the newest deferred report is queued for signing before repeat reports.

To avoid sending another signing request before the dongle response for the previous one returns, `hash_in_flight` now tracks the Core1 dongle transaction between `tuh_hid_send_report()` and `report_received()`.

## OLED/Core1 isolation

P5General mode now has two build-time OLED controls:

- `P5GENERAL_DISABLE_OLED=1`: skip display drawing entirely outside config mode while using P5General.
- `P5GENERAL_OLED_SAFE_INTERVAL_MS`: when OLED is enabled in P5General, draw at a lower default rate (250 ms) so Core1 can prioritize auth USB processing.

## Suggested validation

- Build once with the default P5General OLED safe interval and once with `P5GENERAL_DISABLE_OLED=1`.
- With `P5GENERAL_LATENCY_DEBUG=1`, compare the timing between report queue, dongle send, dongle receive, and PS5 send.
- Test B1 then B1+B3 on the next frame, same-loop B1+B3, and repeated B1+B3 simultaneous presses.  The B1+B3 report should be queued as the next signed report rather than waiting behind stale repeat reports.
