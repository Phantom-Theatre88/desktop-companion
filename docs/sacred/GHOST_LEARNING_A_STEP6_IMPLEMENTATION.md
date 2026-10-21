# Learning A Step 6 — coherent checkpoints and elapsed-time restoration

2026-10-22 JST. Implementation note, not a new LOCK. Base: Step 5 `7daa3cc`.

## Implemented path

CoreS3 setup now configures Ghost checkpoint persistence before Runtime begins. NVS namespace `desk-ghost` is the primary store; an already mounted microSD is the optional backup. GhostCore owns orchestration, Heart owns state restoration, Memory owns committed experience, Time owns elapsed-time validity, and Relationship is reconstructed by evaluating restored Memory. Behavior decisions/reflex overlays/pending events are transient and never replayed after reboot.

A single coherent checkpoint contains Heart state, baseline, the existing eight committed Memory experiences (including original entry/outcome contexts and appraisal evidence), boot-session identity, saved UTC timestamp if available, and quiet duration. Relationship evidence and Step 4/5 behavior therefore survive restoration without an independent duplicate person archive. This persists the existing working-history learning; it does not implement a new permanent learned personality/policy.

Memory stores original uptime timestamps with a session ID. New records use a new session; old ShortMemory uptime/repetition hints are reset rather than compared across boots. Pending staged events and uncommitted Reflex candidates are excluded.

## File format and failure handling

- Version 1 canonical little-endian, field-by-field binary encoding with CRC32. No raw struct/pointer/size_t/bool-padding serialization. Float representation is checked at compile time.
- Validate version, exact length, bounds, finite/ranged Heart values, enum/boolean values, identity/session metadata, committed status and evidence counts. Decode failure does not mutate caller output or live engines. Unknown schemas are rejected, not silently migrated.
- Two slots per medium. Highest valid primary generation wins; backup is used only if neither primary slot is valid. A corrupt newest primary falls back to the valid previous primary.
- A save targets the inactive primary generation, then reads back identical encoded bytes. Backup runs after primary success, alternating from the backup's OWN last valid slot even when it missed generations. Short writes/corrupt files leave the other valid slot available.
- Primary failure is reported and does not overwrite the independent backup. Backup-only continuing writes while primary is unavailable are not implemented. Backup recovery can repair primary when writes work again.
- First boot and missing/invalid stores continue standalone life. Valid legacy Heart-only NVS data is a fallback when no coherent checkpoint exists; invalid legacy float values are rejected. Legacy six-key data cannot retrospectively acquire coherent Memory or a trusted timestamp.

## Time and restoration

TimeEngine accepts an explicit trusted UTC provider, validates supported epoch range (2000–2100), and distinguishes KNOWN / CLOCK_UNKNOWN / CLOCK_REVERSED. Its monotonic uptime accumulator handles uint32 millisecond wrap, assuming ticks occur within one wrap interval.

Known nonnegative offline time recovers mood/curiosity/attention toward the saved dynamic baseline, using the existing online rates (0.12/0.10/0.18 per five seconds after the quiet delay). Saved quiet duration prevents restarting that recovery delay at each boot. This is provisional continuous recovery tuning, not a new LOCK. Affection, baseline, original experience and evidence are unchanged by offline recovery. Short restart preserves emotional aftereffects; long offline time reduces temporary deviation.

Unknown/invalid/backward clock preserves saved Heart values and experiences, with `restorePending()` true; it never invents downtime or treats restart as a reset. Online monotonic recovery continues. A clock becoming available later does not retroactively guess an unknown interval; later checkpoints can carry a trusted time. Boredom/sleepiness are preserved for subsequent live updates: day/night rhythm and power-off-as-sleep inference are NOT implemented.

Default firmware does not assume an unset system clock is trustworthy. `DESKBOT_TRUST_SYSTEM_TIME=1` may be enabled only after board setup establishes a reliable RTC/system clock. No NTP/network dependency is added.

## Save cadence and hardware boundaries

Dirty Heart/Memory checkpoints are attempted at most once per 60 seconds in Runtime tick, including retries. Event delivery does not perform storage I/O. `GhostCore::saveCheckpoint()` provides an explicit orderly-shutdown/maintenance flush. Abrupt power loss can lose changes since the last successful checkpoint (normally up to the interval, longer if writes fail). Timing and capacity remain adjustable implementation limits.

The microSD adapter checks for an already mounted card and never mounts/reconfigures SPI or claims pins on its own. Current setup does not newly mount SD; absent/unmounted card is a nonfatal backup-unavailable condition. Actual mounting is a board integration task to verify with Camera/Touch/IMU. Restore source and offline-clock status are printed during setup; save status/generation are available through Ghost persistence accessors.

## Executed Mac verification

`bash tests/run_host_tests.sh` — all PASS: face/body, vision/runtime, Memory Step 2, Relationship Step 3, Heart Step 4, Behavior Step 5, persistence Step 6. Steps 3–6 use AddressSanitizer + UndefinedBehaviorSanitizer; warnings are errors. `git diff --check` passed.

Step 6 compiles actual production Ghost/Heart/Memory/Relationship/Behavior, checkpoint codec, legacy HeartPersistence and ESP32 NVS/SD adapter code with host platform/storage substitutes. Tests cover first/legacy boot, finite legacy validation, same-time/short/medium/day-long reboot, clock unavailable/backward, immutable original experience, relationship reconstruction, pending exclusion, session changes, latest CRC corruption, previous-generation fallback, primary repair from backup, partial primary/SD writes, missed backup generations, SD absence, invalid schema with valid CRC, corruption at every encoded byte, every truncation length, invalid snapshot values, periodic writes/retry limiting and millisecond wrap.

A separate executable process writes a real temporary file checkpoint; a NEW process reads it and verifies restored person experience, Relationship and OPEN name-call response. This does not rely on static in-process engine objects surviving reboot.

## Not verified / still separate

No CoreS3 target build/upload or physical power-cycle was run (`arduino-cli` unavailable). Need target compilation, stack/RAM and NVS capacity review, real NVS interrupted-write behavior, SD mounting/read/write/removal, RTC reliability/offline time and sensor/body coexistence checks. Host graphics/storage substitutes do not establish real flash/card guarantees or physical response quality.

Eight-slot retention remains finite; evicted experience/learned consequences do not independently persist. Permanent trend learning, long-term baseline growth, day/night rhythm, identity recognition/correction, broader Heart/Behavior coverage, interpretation revision and Learning B remain future capabilities.

Result: **Step 6 minimum checkpoint/restart path is implemented and Mac-host verified. Hardware, trusted-clock supply and SD mounting remain separate conditions; Learning A as a whole is not complete.**
