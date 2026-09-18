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
