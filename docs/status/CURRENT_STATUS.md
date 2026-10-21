# Current implementation status — Learning A Step 3

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

## Existing full-suite failure (not a Step 3 pass)

`bash tests/run_host_tests.sh` cannot compile at the base: FaceRenderer uses drawing APIs missing from the old M5GFX host shim. A temporary shim alignment exposed an unused local and then an outdated assertion at face_test.cpp:59 expecting Touch lower-lid changes from Behavior. Current Behavior deliberately delegates immediate sensor expression to Reflex→Body Output. Root cause is test drift across that responsibility transfer, not the Relationship integration. Exploratory unrelated modifications were reverted. Full face/vision suite remains unverified/failing; correcting it needs tests of the current BodyOutput composition rather than restoring the obsolete Behavior ownership.

## Hardware-only / not verified

No CoreS3 build/upload or hardware execution this task (arduino-cli unavailable). Need target compilation, RAM/stack review, Touch/IMU/Camera/Face coexistence, Reflex priority and standalone life verification on hardware. Actual person recognition/attribution is not implemented by this Step; Mac tests inject stable identities intentionally.

## Remaining work

Step 3's minimal Memory→person-specific Relationship evaluation is host-tested. Learning A and the overall robot are incomplete. Memory is still the existing volatile eight-record working history; evaluation loses evidence on eviction/restart. Persistent learning, retention/forgetting, identity correction, trust calibration, Heart/Behavior integration and elapsed-time recovery remain future work. This does not supersede the sacred LOCKs or the separate documentation repository's hardware status.
