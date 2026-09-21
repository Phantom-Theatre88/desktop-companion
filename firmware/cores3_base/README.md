# CoreS3 Base — 現行実装基準点

M5Stack Desktop Companion の新しいゼロベース実装基準点。

## 基盤

- Hardware: M5Stack CoreS3
- Framework: Arduino
- Library: M5Unified
- Graphics: M5GFX
- Application: M5Stack Desktop Companion 独自コード

既存Stack-chan系OSSは親Repo・初期基盤として使用しない。

## 現在地

Step 0のClean Base確認後、現在は神経RuntimeとGhost本番骨格の実装へ進んでいる。

現在の主な構成：

- `SemanticNeuron`
- `SynapseRouter`
- `DesktopCompanionRuntime`
- `ReflexLayer`
- `GhostCore`
- `HeartEngine`
- `MemoryEngine`
- `TimeEngine`
- `RelationshipEngine`
- `BehaviorEngine`

Ghostは後付けせず、`Heart / Memory / Time / Relationship / Behavior` を本番境界として最初から保持する。

## Heart Engine 現在実装

LOCK済みの初回起動値を使用する。

- mood = 0.60
- affection = 0.55
- curiosity = 0.65
- boredom = 0.15
- sleepiness = 0.15
- attention = 0.50

内部値は0.0〜1.0で扱い、`HeartContext` はread-only snapshotとして取得する。

1イベント処理では開始時点のHeart Contextを固定し、同一イベント中の各Ghost要素へ同じsnapshotを渡す。

イベントごとの具体的なHeart変化量、時間減衰率、回復係数等は、未LOCK値を勝手に実装しない。

## 起動確認

画面：

- `CoreS3 BASE`
- `NERVE / GHOST READY`

Serial 115200：

- Runtime READY / ERROR
- Synapse binding数
- Heart初期6値

を確認できる。

## 旧資産

Stack-chan本家、AI_StackChan_Ex、stack-chan-ko、RoboEyes、旧Yuki実装等はLEGACY参考資産として扱い、現行基盤へ自動的に混在させない。

## Face expression parts (2026-09-18)

The neutral silhouette and resting Behavior dimensions are preserved. `EyeShape`
provides independent left/right width (0.65–1.20), height (0.60–1.25), radius
(0–2), upper/lower coverage (0–1), and mirrored upper-lid slope (-1–1).
`ExpressionParams::spacing_scale` is center separation (0.85–1.12), not the
edge-to-edge gap. Values are relative to the existing neutral geometry.

Behavior emits shape targets, gaze, blink openness, and instantaneous jitter.
FaceRenderer only bounds, interpolates, and draws them. Geometry follows an
85 ms exponential time constant; openness and jitter bypass that filter to
preserve blink closure and event timing. All tuning values remain implementation
defaults, not personality LOCKs. Masks combine by occlusion; if upper and lower
coverage meet, the visible eye disappears. Fully closed eyes retain the eyelid line.

RoboEyes was consulted for configurable geometry, interpolation, masks and
transient displacement only. No library, source code, mood selection, random
idle, or automatic blink scheduler was imported. Reference:
https://github.com/Harrison-Xu/M5Stack_RoboEyes/blob/main/M5Stack_RoboEyes.h
Our existing silhouette is extended using elapsed-time geometry tracking and
bounded per-eye masks; event timing belongs to Behavior.

### Hardware checks (pending)

1. Boot with the usual camera loop: neutral remains broad, large and cyan;
   blinks fully close and reopen; no full-screen flashing.
2. Touch: lower lids briefly rise, then return; lifting: eyes visibly enlarge
   and return within about one second. These are provisional parameter mappings.
3. SHAKE: asymmetric eyes and small decaying horizontal/vertical displacement;
   no persistent shake after the event (650 ms plus geometry settling).
4. Repeat interactions during camera capture: camera READY and I2C restored YES,
   Touch/IMU logs, and the life loop must continue. Capture blocking may still
   hide brief face frames; this change does not alter camera scheduling.
5. Geometry-only checks: independently vary left/right `EyeShape` in a diagnostic
   build; exercise upper/lower lids, tilt, width, height, radius, and spacing,
   then restore defaults. No automatic showcase mode is enabled in production.

### Host regression check

From the repository root (a C++11 compiler is required):

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Itests/face/stubs tests/face/face_test.cpp firmware/cores3_base/src/face/FaceRenderer.cpp firmware/cores3_base/src/ghost/BehaviorEngine.cpp -o /tmp/deskrobo-face-test
/tmp/deskrobo-face-test
```

The graphics stub checks geometry and call boundaries, not actual M5GFX pixels.
It covers neutral dimensions, lids, blink closure, geometry limits, frame-rate
independence, timestamp wrap, and transient expiration. Actual display quality
and sensor coexistence still require the CoreS3 checks above.

## Vision → Semantic Neuron (2026-09-18)

The physical body is **M5Stack Stack-chan**, with its CoreS3 controller and
built-in two-axis neck. `cores3_base` names the current software foundation;
it does not mean the robot has no neck. Existing Stack-chan firmware is a
reference, not the runtime parent.

CameraVisionInput now emits `MOTION_DETECTED`, `BRIGHTER`, or `DARKER` through
Runtime/Synapse into Ghost (Heart entry, Memory record, Behavior). The neuron
contains normalized change strength; image data stays in Vision and is never
retained after the camera callback. Existing enum values are preserved.

- 16×12 luminance grid, four samples per cell, RGB565 high-byte-first as specified
  by esp32-camera's `fmt2rgb888` conversion. This corrects the old luma byte order;
  old/new luma numbers should not be compared directly.
- First frame, invalid frame, resolution change, or gap over 6.5 s: baseline only.
- Absolute mean change ≥24/255: BRIGHTER/DARKER, preferred over motion.
- Otherwise, at least 12% of cells changing ≥20/255 after global mean correction:
  MOTION_DETECTED. At most one event per frame, at least 3 s between emissions.
- PICKED_UP/SHAKE invalidates Vision history; observations within 3 s are discarded
  and the next accepted frame re-arms the baseline. This is not full ego-motion
  compensation: unreported body movement can still look like scene movement.
- Dispatch happens after camera teardown/I2C restoration; failures clear pending
  events. The body loop uses a fresh timestamp after blocking capture.
- Behavior uses existing eye openness for a brief response. Touch/IMU take
  precedence; visual events do not cancel them. No new expression animations.
- Heart receives the event, but numeric mood/attention update rules remain
  unimplemented. Do not interpret successful routing as completed emotions.

These are scene-change heuristics, not person/face detection, identity recognition,
object tracking, or a reliable motion direction. Two-second sampling can miss
short movement. Camera reinitialization/auto-exposure, shadows and lighting can
produce changes; thresholds require real-device observation.

### Arduino IDE / hardware check

Update `heart-engine`, open `cores3_base.ino`, select M5CoreS3 and its USB port,
verify and upload. Serial Monitor: 115200 baud.

1. Runtime READY, 23 Synapse bindings, Camera READY, I2C restored YES.
2. First frame and a stationary scene should not continuously emit events.
3. Move a high-contrast object across the view, taking at least 2 seconds:
   `[NERVE][VISION] MOTION_DETECTED ... -> Ghost/Heart/Memory/Behavior`.
   The same event must also print
   `[TRACE][VISION] Ghost=YES Heart=YES Memory=YES Behavior=YES`.
   Any `NO` means the end-to-end hardware check has not passed.
4. Change lighting or cover/uncover the camera: BRIGHTER/DARKER. Wait at least
   4 seconds between trials because captures and cooldown are discrete.
5. Keep Touch, IMU, blink and face rendering running; after moving the body,
   let the baseline settle before testing scene motion again.
6. Report false positives with camera luma and the semantic event log. A
   MOTION_DETECTED log does not mean a person was recognized.

Host checks: `bash tests/run_host_tests.sh` (C++17 compiler; optional `CXX`).
Tests use synthetic frames and real Runtime/Ghost/Memory/Behavior code with a
host-only persistence stub; physical camera/sensor coexistence is unverified.

### Built-in neck connection

Stack-chan's two feedback servos are part of the intended body output. The
independent runtime currently has no neck driver. The documented hardware uses
UART TX GPIO6 / RX GPIO7; the reference firmware uses 1 Mbps and IDs 1/2.
Next connect Behavior → neck command → independent Device Driver, verifying
feedback, zero calibration, angle/speed limits and self-motion suppression first.
Do not drive the neck from this directionless MOTION_DETECTED event or pretend
it tracks a person. A validated spatial target is needed for visual following;
expressive head shaking is a separate Behavior action.

Hardware reference: https://docs.m5stack.com/en/StackChan
Pixel format reference: https://github.com/espressif/esp32-camera/blob/master/conversions/to_bmp.c

## ESP-SR / wake word flash layout (2026-09-20)

Wake-word work uses Arduino IDE `Partition Scheme: ESP SR 16M` so the board
package enables its ESP-SR model upload hook. The sketch directory also carries
its own `partitions.csv`; Arduino gives that local table precedence.

Reason: M5Stack Arduino core 3.3.9's bundled `esp_sr_16` table places the model
partition at `0xD10000` with size `0x2E0000` (~2.9 MiB), while the bundled
`srmodels.bin` observed on the CoreS3 development environment is 3,340,296
bytes. That combination compiles but cannot be flashed.

The project-local table follows Espressif's 16MB ESP-SR layout instead:

- app0: 3 MiB
- app1: 3 MiB
- SPIFFS: 6 MiB
- model: `0xC10000` / `0x3E0000` (~3.875 MiB)
- coredump: final 64 KiB

Do not change back to the M5Stack default ESP-SR table unless its model region
is large enough for the actually bundled model binary. Keep the IDE scheme on
`ESP SR 16M`; the local CSV fixes geometry while the selected scheme keeps the
ESP-SR packaging/upload hook active.

### M5Stack Arduino core 3.3.9 upload-offset workaround

The project-local `partitions.csv` moves the ESP-SR model partition to
`0xC10000`, but M5Stack Arduino core 3.3.9 still passes `srmodels.bin` to
esptool at `0xD10000` through the board's upload metadata. This is independent
from the partition CSV, so the binary still fails the 16MB flash fit check.

Run the repository helper once after installing/updating the M5Stack Arduino
core:

```sh
cd ~/desktop-companion
bash firmware/cores3_base/tools/patch_m5stack_esp_sr_upload_offset.sh
```

The helper:

- locates the newest installed M5Stack ESP32 core,
- backs up `boards.txt`,
- changes only the `srmodels.bin` upload offset `0xD10000 -> 0xC10000`,
- verifies the resulting line,
- is safe to run again if already patched.

Afterward, fully restart Arduino IDE, keep `Partition Scheme: ESP SR 16M`,
and upload normally. Re-run the helper after an M5Stack board-package update if
that update restores the old upload offset.

## WakeNet hardware test word: Computer (2026-09-22)

Before the final Japanese wake phrase is decided, hardware validation uses the
official ESP-SR WakeNet9 model:

- spoken wake word: `Computer`
- ESP-SR model: `wn9_computer_tts`
- semantic target during this test: `computer`

This is deliberately not mapped to `kibi`. A successful Computer detection
proves the real WakeNet path without pretending that another acoustic model
means the companion's final name.

Install the official test model with:

```sh
cd ~/desktop-companion
bash firmware/cores3_base/tools/install_computer_wakenet_model.sh
```

The installer generates `srmodels.bin` from Espressif's official
`wn9_computer_tts` model, backs up the installed M5Stack Arduino SDK model
binary, installs the generated binary, and writes a gitignored local marker that
allows semantic wake events only after installation.

Then fully restart Arduino IDE, keep `Partition Scheme: ESP SR 16M`, and
upload `cores3_base.ino`.

Expected boot state:

```text
Wake word target: computer model=wn9_computer_tts ... semantic=READY
```

Expected successful detection:

```text
[WAKENET][EVENT] ... wakes=1 ... consumed=1 ... semantic=READY
[NERVE][MIC] WAKE_WORD_DETECTED word=computer ...
```

## WakeNet production path for kibi (2026-09-22)

The production wake-word path is now **ESP-SR WakeNet**, not MultiNet command
recognition.

Why:
- MultiNet successfully received PCM and produced command callbacks, but natural
  Japanese `キビ` did not match its English G2P command pronunciation reliably.
- The wake word is part of the companion's identity, so pronunciation hacks must
  not leak into SemanticNeuron / Ghost / Heart.
- Espressif's WakeNet is the dedicated always-listening wake-word engine for
  ESP32-S3.

Current runtime state:
- `EspSrWakeNetDetector` runs `SR_MODE_WAKEWORD`.
- PCM feed and WakeNet events have their own diagnostics:
  - `[WAKENET][FEED] ...`
  - `[WAKENET][EVENT] ...`
- Semantic output is intentionally **disabled** until the model partition is
  explicitly confirmed to contain a custom WakeNet model trained for `kibi`.
- A bundled/default WakeNet phrase is never allowed to masquerade as
  `WAKE_WORD_DETECTED word=kibi`.

The switch controlling semantic delivery is:

```cpp
constexpr bool kKibiWakeNetModelInstalled = false;
```

Do **not** change it to `true` merely to make the pipeline appear complete.
Change it only after installing and verifying the actual custom kibi WakeNet
model.

### External model dependency

A true custom `kibi` WakeNet model is an external model artifact; it cannot be
created by changing this repository alone. Espressif currently documents
WakeNet customization and also accepts community TTS-trained wake-word requests.
Japanese is listed among the languages supported by the community TTS pipeline.

The project should request/train a Japanese WakeNet model whose spoken wake
phrase is decided explicitly for this robot. The preferred identity remains
`kibi`; if Espressif recommends a longer phrase for reliable WakeNet training,
that phrase should remain an acoustic trigger only and still map to semantic
`WAKE_WORD_DETECTED word=kibi`.

The previous `EspSrKibiDetector` MultiNet implementation is retained only as a
diagnostic/history reference and is no longer the production wake-word backend.

