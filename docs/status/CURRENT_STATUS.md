# Current implementation status — Learning A Step 6

2026-10-22 JST. Base: Step 5 `7daa3cc`. Previous Step 5/4/3 records follow below.

## Step 6 result

**Minimum coherent persistence/time/restoration path implemented and Mac-host verified. CoreS3 target and hardware remain unverified.**

- CoreS3 setup connects primary two-slot NVS checkpoints and optional already-mounted SD backup.
- Heart/baseline and eight committed Memory experiences restore together with schema/CRC/value validation. Relationship is re-evaluated from Memory; transient Behavior/Reflex state is not replayed.
- Known offline time recovers temporary mood/curiosity/attention; affection and original experience remain intact. Unknown/backward clock preserves state with temporal correction pending. Session IDs prevent old uptime hints being reused across boots.
- Dirty periodic saving/retry is rate-limited to 60 seconds; explicit checkpoint flush is available. Latest corrupt/partial save falls back to the previous valid primary or independent backup.
- Existing Heart-only data is a validated migration fallback. Firmware does not guess UTC or mount/reconfigure SD implicitly.

Executed `bash tests/run_host_tests.sh`: face/body, vision/runtime and Step 2–6 all PASS, including separate-process file save/read and sanitizer-enabled persistence/failure tests. `git diff --check` passed.

Still separate: physical target build/power cycle, NVS/SD guarantees and resources, trusted RTC/system clock supply (`DESKBOT_TRUST_SYSTEM_TIME` defaults off), SD mounting, actual identity recognition, day/night rhythm, independent retained learning after history eviction, long-term baseline growth, broader stimulus/behavior coverage and Learning B. Default firmware restores experience without a clock; offline emotional correction requires a trusted provider. See `docs/sacred/GHOST_LEARNING_A_STEP6_IMPLEMENTATION.md` for exact conditions.

---

## Historical Step 5 result

2026-10-22 JST. Base: Step 4 `ab945d0`. Previous Step 4/3 records are preserved below.

## Step 5 result

**Minimum Step 5 implementation and Mac host verification are complete; CoreS3 hardware remains unverified.**

- Behavior uses the same pre-event Memory/Relationship evidence snapshot as Heart.
- WAKE_WORD_DETECTED acknowledgement selects standard/open/measured/mixed reception, then combines it with current Heart to drive existing continuous eye/neck output.
- Unknown identities and mismatched evidence stay standard. Contradictory experience remains mixed. No separate event archive, trust inference or permanent learned policy is added.
- Decisions expire, reset, and are replaced for each caller. Direct Ghost-delivered body events cancel them. Safety/direct Body overlays retain priority; ambient vision stays below interpersonal Behavior.
- Handling time zero and uint32 wrap are covered by an explicit active flag.

Executed `bash tests/run_host_tests.sh`: face/body, vision/runtime, Memory Step 2, Relationship Step 3, Heart Step 4, Behavior Step 5 all PASS; Step 3–5 include address/undefined-behavior sanitizers. `git diff --check` passed.

Scope: existing name-call acknowledgement only. Person recognition, other response paths, actual appearance/servo motion, CoreS3 target build/coexistence and persistent learning remain unverified or unimplemented. Step 6 (time/persistence/restart restoration) is next. See `docs/sacred/GHOST_LEARNING_A_STEP5_IMPLEMENTATION.md` for the exact path and limitations.

---

## Historical Step 4 result

2026-10-22 JST. Step 4 base: Step 3 `aad35fb`. Existing Step 3 record is preserved below.

## Step 4 result

**Minimum Step 4 implementation and Mac host regression verification are complete; CoreS3 hardware is unverified.**

- Ghost snapshots prior Memory and Relationship before staging the current event.
- Heart's TOUCH reception uses same-person/same-type prior reception, person-specific Relationship evidence and event-entry mood. Unknown identities do not share interpersonal evidence.
- Memory records the evidence used and the before/after Heart contexts without rewriting the original experience.
- Positive and negative evidence remain independent; Mood/Comfort and Affection/Social are not conflated. Safety/reflex and baseline updates remain outside this appraisal.
- Scope is TOUCH mood/affection reception. Other stimuli keep their existing tables. This is not completion of all Heart learning capabilities.

Executed: `bash tests/run_host_tests.sh` — face/body, vision/runtime, Memory Step 2, Relationship Step 3, Heart Step 4 all PASS. Step 3/4 use address/undefined-behavior sanitizers. `git diff --check` passed.

See `docs/sacred/GHOST_LEARNING_A_STEP4_IMPLEMENTATION.md` for tuning, validation, and exact limitations. Step 5 (Behavior directly using experience/relationship), Step 6 (time/persistence/restart), identity recognition, long-term learning and hardware validation remain open.

---

## Historical Step 3 result

2026-10-22 JST. Base: heart-engine `a437c554d9a0409826b0ca1a9c3ea2dba0e844b6`.

## Implemented and executed on Mac

- Memory remains sole owner of experience; committed person-specific evidence can be visited through a const boundary. Pending events, unknown identity and reflex candidates are excluded.
- SemanticNeuron carries an optional stable `person_id` (0 = unknown). Attribution is supplied explicitly upstream; sensor source/presence/vision confidence do not identify a person. Existing adapters leave identity unknown.
- Relationship binds to Memory and evaluates a person on demand. No duplicated events, persistent person cache, event subscription, or stale evaluation after history eviction.
- Evaluation reports available experience count and positive/negative reception evidence from recorded mood/affection changes. Mixed reception can count in both directions. This descriptive evaluation is not trust, intent, Comfort, Social, or a learned affection score. Non-finite reception is not classified.
- Ghost binds its own Memory on begin. Existing event-entry Heart snapshot and stage/update/commit ordering remain unchanged. Heart/Behavior consuming relationship evaluations belongs to subsequent Steps.

## Validation

Darwin arm64, Apple clang 21.0.0. Executed successfully:

- `python3 tests/host/test_memory_step2.py`
- `bash tests/host/run_relationship_step3.sh` (C++17, warnings as errors, AddressSanitizer + UndefinedBehaviorSanitizer)
- `git diff --check`

Step 3 checks: unbound/empty Memory, unknown identity, separate people, pending exclusion, double commit, positive/negative/mixed evidence, immutable original reception, eight-slot eviction, Memory reset, real Ghost→Heart→Memory integration and Ghost restart.

## Full host regression repaired and passed

The unmodified Step 2 base `a437c55` was extracted into an isolated temporary directory. Its original suite reproduced the missing drawing-API compilation failure. Investigation confirmed test drift: immediate expression moved from Behavior to Reflex→BodyOutputComposer, directed motion uses eye silhouettes/neck rather than gaze translation, bindings grew to 29, and Vision cooldown is 1000 ms.

Updated tests now exercise actual Reflex→Body composition, direct response expiry, directional silhouette/neck response, interpersonal priority and safety-over-Touch/Vision arbitration. Autonomous tests inspect the current shape/neck output. The vision test uses 999 ms for the within-cooldown case and checks composed body output. Host shims now cover the auxiliary drawing APIs and the current HeartPersistence boundary. One unused FaceRenderer local was removed to satisfy warnings-as-errors; runtime behavior is unchanged.

The repaired face/body and vision regression tests passed against BOTH the Step 2 base and the Step 3 implementation (same test changes and unused-local cleanup). Then `bash tests/run_host_tests.sh` passed on Step 3, including face/body, vision/runtime, Memory Step 2 and sanitizer-enabled Relationship Step 3. Auxiliary rendering shims do not validate actual pixels of mouth/props/effects.

**Learning A Step 3 is complete within the agreed Mac host scope.** Hardware verification remains separate. This is not completion of Learning A as a whole.

## Hardware-only / not verified

No CoreS3 build/upload or hardware execution this task (arduino-cli unavailable). Need target compilation, RAM/stack review, Touch/IMU/Camera/Face coexistence, Reflex priority and standalone life verification on hardware. Actual person recognition/attribution is not implemented by this Step; Mac tests inject stable identities intentionally.

## Remaining work

Step 3's minimal Memory→person-specific Relationship evaluation and host regressions have passed. Learning A and the overall robot are incomplete. Memory is still the existing volatile eight-record working history; evaluation loses evidence on eviction/restart. Persistent learning, retention/forgetting, identity correction, trust calibration, Heart/Behavior integration and elapsed-time recovery remain future work. This does not supersede the sacred LOCKs or the separate documentation repository's hardware status.
