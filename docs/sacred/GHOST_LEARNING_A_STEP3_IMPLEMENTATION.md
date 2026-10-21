# Learning A Step 3 — Memory / Relationship boundary

Implementation note, not a new LOCK. See Step 1 audit and 2026-10-21 Ghost LOCKs.

Memory owns facts and recorded reception. Relationship visits committed Memory records for a supplied person ID and returns a value snapshot describing positive/negative experience evidence. Querying never rewrites facts or reception. Unknown identity never means the owner or the last seen person. Both positive and negative evidence can coexist. No coefficients, trust defaults, recognition confidence thresholds or permanent retention rules are fixed here.

The existing eight-record working history bounds this evaluation. No extra event archive/person table is introduced in Relationship. Future Memory retention and Relationship learning can extend this boundary; persistent learned state is not claimed by this implementation.

Run `bash tests/host/run_relationship_step3.sh` from the repository root. Tests compile the actual production engines and real Heart structures; only platform and HeartPersistence storage are replaced on the host. Hardware and full-suite limitations are recorded in `docs/status/CURRENT_STATUS.md`.
