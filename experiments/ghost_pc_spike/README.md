# Ghost PC runtime spike (NOT a new LOCK)

Purpose: validate a small, hardware-independent Python 3 standard-library implementation against the existing Ghost sacred design. **This is a platform/language feasibility experiment, not a decision to replace C++ or an implementation of full Ghost learning.**

Run from this directory: `PYTHONPATH=. python3 -m unittest discover -s tests -v`

Implemented and tested locally (2026-11-09): five engine interfaces; separate experience/relationship evidence responsibilities; pre/post Heart snapshots; no attribution of anonymous events; duplicate event guard; minimal reinterpretation; monotonic-in-process clock. Six tests passed in local Python environment; **GitHub-hosted test run not yet verified**.

Not implemented: persistent storage, timeline across power-off, interpretation history, long-term learning, trust scoring, autonomous behavior policies, external Brain/Character messages, M5 event compatibility, data protection and encryption. The `actor_id` and `event_id` fields are **experimental transport candidates**, not approved schema. This experiment must not be mistaken for the completed Ghost.

Technology assessment: Python stdlib makes logic and database integration easier and portable to Windows/Linux/macOS, but introduces interpreter/runtime dependency and potential GIL or scheduling limits. Portable C++ would reuse legacy logic and suit high-frequency control but needs Arduino/platform isolation; Python+C++ adds integration complexity. Select final language only after measuring required tick frequency, actual integration, persistence, and packaging on target PC.
