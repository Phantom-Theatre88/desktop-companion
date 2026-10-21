# Learning A Step 5 — Behavior from experience and relationship

2026-10-22 JST. Implementation note, not a new LOCK. Base: Step 4 `ab945d0`.

## Minimum production path

The supported path is the existing **WAKE_WORD_DETECTED acknowledgement**. Behavior selects STANDARD / OPEN / MEASURED / MIXED from prior experience and Relationship evidence, then combines that decision with current Heart attention/curiosity to generate the existing continuous eye and neck response. No new canned face, random policy, sixth Ghost core or renderer-side judgment is introduced. Autonomous action learning and all-stimulus coverage are not claimed.

Ghost passes Behavior the SAME value `HeartEventEvidence` snapshot already obtained before the current Memory experience was staged. It does not query the just-committed event as its own history. Memory owns the original event, entry/outcome contexts and appraisal evidence. Relationship owns the person-specific evaluation; Behavior holds only a transient decision, not a second event/person archive. Existing LOCK 35 entry HeartContext remains unchanged.

## Selection and output

- Unknown or mismatched identity/type: STANDARD. Anonymous records are not interpersonal history.
- Valid exact-type/same-person prior: signs of recorded mood/affection deltas contribute positive and/or negative evidence.
- Valid Relationship counts: positive/negative signs contribute independently, including experience of different event types with that person.
- Positive only: OPEN; negative only: MEASURED; both: MIXED; neither: STANDARD. These names describe response selection, not trust, intent, Comfort or Social.
- Provisional response scales: STANDARD 1.00, OPEN 1.15, MEASURED 0.80, MIXED 0.95. They scale the existing current-Heart-derived acknowledgement drive, which is clamped to [0,1]. Duration remains 1100 ms; eye/neck geometry and body limits remain downstream. These tunings are not personality LOCKs.
- Missing/non-finite prior data and invalid relationship counts are independently ignored. Mixed evidence is not erased by averaging it into a trust score.

The decision is replaced by each new call, reset on expiry/begin, and cancelled by existing Ghost-delivered Touch/PICKED_UP/SHAKE events. Low-level vision does not invent or replace a caller. BodyOutputComposer still puts active direct/safety reflex above interpersonal Behavior and interpersonal Behavior above ambient vision. Reflex-only LIFT_STARTED retains the existing Body overlay semantics; no new Ghost route is added.

An explicit active flag replaces the old start-time-zero sentinel, so an acknowledgement at handling time zero works. Age uses unsigned elapsed handling time and works across uint32 clock wrap. Source occurrence timestamp is not substituted for handling time.

## Executed Mac verification

`bash tests/run_host_tests.sh`: all PASS (face/body, vision/runtime, Memory Step 2, Relationship Step 3, Heart Step 4, Behavior Step 5). Apple clang, C++17, warnings-as-errors. Step 3/4/5 run with AddressSanitizer and UndefinedBehaviorSanitizer. `git diff --check` passed.

Step 5 tests use production Runtime/Synapse/Ghost/Heart/Memory/Relationship/Behavior/Reflex/BodyOutputComposer/FaceRenderer code, with host graphics and persistence substitutes. They verify:

- Identical current Heart + same input, different prior/relationship evidence → different choice AND eye/neck output.
- Same choice + different Heart attention → different expression strength.
- Prior-only, Relationship-only, mixed, no evidence; unknown/other-person/type mismatch/non-finite/invalid counts.
- Pre-event evidence count, original entry/outcome contexts, one new committed record.
- Caller replacement, evidence eviction, finite repeated output, begin/reset, expiry, start time zero and clock wrap.
- Ambient vision cannot bury the acknowledgement; active SHAKE still overrides its output. Runtime SHAKE cancels the old acknowledgement.

## Separate unfinished scope

CoreS3 target build/upload, visible tuning and Touch/IMU/Camera/Face coexistence have not been verified on hardware. Actual person recognition is not implemented; existing adapters leave person_id=0, so tests inject identities and the deployed default remains STANDARD. Graphics substitutes do not validate actual pixels or servo motion.

Memory is still volatile eight-record working history. Step 6 must address time, persistence, elapsed-time restoration and preservation of learning. Comfort/Social state implementation, long-term baseline learning, intent inference and Learning B remain separate. The decision here is transient and is not a persistent learned policy.

Result: **Step 5 minimum WAKE_WORD production path and Mac host regressions complete; hardware unverified.**
