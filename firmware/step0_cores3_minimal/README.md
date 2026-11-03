# CoreS3 Step 0 — minimal base

This isolated sketch is the v1.0 Step 0 starting point. It uses Arduino, M5Unified, M5GFX, and this project's own code only. It does not include the production `cores3_base`, Ghost, sensors, camera, network, or character behavior.

## Build and upload state (2026-11-03)

- CoreS3 FQBN: `m5stack:esp32:m5stack_cores3`
- Build: **PASS** — 518,287 bytes flash (16% of 3,145,728); 25,768 bytes global RAM (7% of 327,680).
- Upload: **NOT COMPLETED** — the session could not open `/dev/cu.usbmodem5101`; the operating system returned `Operation not permitted`. No other process held the port according to the available port inspection.
- Device display and serial output: **NOT VERIFIED**.

## Open and upload

Open `step0_cores3_minimal.ino` in Arduino IDE, select M5CoreS3 and its connected port, then upload. The sketch initializes M5Unified, draws a fixed startup label, and keeps the normal update loop running.

## Step 0 acceptance

Pass only after the CoreS3 repeatedly boots into the label without relying on the previous firmware and the serial monitor shows:

```text
[STEP0] CoreS3 minimal base started
```

Build success alone is not physical Step 0 evidence. Step 1 diagnostics will be added only after this base is confirmed on the device.
