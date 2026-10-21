# Learning A Step 4 — Heart reception from prior experience

2026-10-22 JST. Implementation note, not a new LOCK. Base: Step 3 `aad35fb`.

## Implemented scope

The first supported experience-dependent reception is **TOUCH**. Heart now uses the event-entry mood, exact same-type/same-person prior reception, and Relationship's person-specific positive/negative evidence. This is the minimum production path for Step 4, not a claim that every stimulus or the whole learning system is complete.

Ghost reads committed Memory and evaluates Relationship BEFORE staging this event. It passes a value `HeartEventEvidence` snapshot to Heart and stores that same evidence in the new MemoryRecord, alongside the original event-entry and outcome Heart contexts. Memory owns these facts; Relationship holds no duplicate archive; Heart does not search or modify Memory.

- Known identity: exact type/person prior reception can affect this Touch. Other event types for the same person can inform the Relationship evaluation.
- Unknown identity: no interpersonal experience/relationship appraisal, including anonymous-to-anonymous. A different person's experiences cannot affect the event.
- Current mood relative to baseline affects the mood response only. Mood is not Comfort; affection is not Social. No new Comfort/Social state, trust score, intention inference or permanent personality update is introduced.
- Positive relationship evidence can modestly strengthen the affection increment. Negative evidence can temper the mood increment. Mixed evidence can do both rather than being cancelled into a single trust number.
- SHAKE, Reflex intents, safety arbitration, baseline/recovery and other stimulus tables are unchanged. Touch appraisal only scales its mood/affection increments; it never reverses their signs or changes the direct body reaction.

## Provisional implementation tuning

Touch mood scale = 1 + 0.10 × bounded(entry mood − entry baseline mood) + bounded(prior mood delta) − 0.10 × negative/count.

Touch affection scale = 1 + bounded(prior affection delta) + 0.10 × positive/count.

Entry difference is bounded to ±0.5; prior deltas to ±0.1; final scales to 0.75–1.25. Evidence terms apply only for matching known person/type. Missing, mismatched or non-finite prior evidence is ignored. Invalid counts are ignored. Existing habituation and state clamping still apply. These small, adjustable coefficients are not new LOCKs or learned trust parameters. Event content remains the dominant base table.

## Executed validation

Run `bash tests/run_host_tests.sh`. Mac arm64 Apple clang 21, C++17, warnings as errors. All suites passed: face/body, vision/runtime, Memory Step 2, Relationship Step 3 and Heart Step 4. Step 3/4 include AddressSanitizer and UndefinedBehaviorSanitizer.

Step 4 tests cover: same input with positive/negative/no experience; same-person different-type Relationship evidence; known/unknown/other-person separation; mixed reception; event-entry state dependence; pre-stage appraisal (no self-reference); preserved original Memory and event contexts; stored appraisal evidence; malformed numeric/count/mismatched inputs; SHAKE/Reflex invariance; 1000 repeated events staying finite and bounded; baseline unchanged; reset of transient appraisal.

## Separate limitations

No CoreS3 target build/upload/hardware observation. Platform graphics and persistent storage remain host substitutes. The actual person-recognition producer is still absent; tests inject identities. Existing adapters keep person_id=0 and therefore cannot use person-specific reception until attribution is supplied.

Memory remains the existing volatile eight-record history. Permanent learning, days/weeks baseline adaptation, forgetting/retention, elapsed-time restart recovery and persistence are not implemented here (Step 6/later scope). Step 5 will connect experience/relationship directly to Behavior. Real-world reception tuning and CoreS3 Touch/IMU/Camera/Face coexistence require hardware validation.

Result: **Step 4 minimum production path implemented and Mac-host verified; hardware unverified.**
