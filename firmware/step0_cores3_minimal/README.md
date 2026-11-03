# CoreS3 Step 0 — minimal base

This isolated sketch is the v1.0 Step 0 starting point. It uses Arduino, M5Unified, M5GFX, and this project's own code only. It does not include the production `cores3_base`, Ghost, sensors, camera, network, or character behavior.

## Open and upload

Open `step0_cores3_minimal.ino` in Arduino IDE, select the M5CoreS3 board and its connected port, then upload. The sketch is intentionally small: it initializes M5Unified, draws a fixed startup label, and keeps the normal update loop running.

## Step 0 evidence to record

Pass only after the CoreS3 repeatedly boots into the label without relying on the previous firmware and the serial monitor shows:

```text
[STEP0] CoreS3 minimal base started
```

This sketch has not yet been compiled or uploaded in this work session. A source file or successful host inspection is not physical Step 0 evidence. Step 1 diagnostics will be added only after this base is confirmed on the device.
