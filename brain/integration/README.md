# AKARI Brain Step 1 — OpenClaw × LangGraph

**Status: implementation pushed; end-to-end runtime verification pending.**
Current GitHub branch: heart-engine. This is a Brain integration prototype, not a new canon/LOCK.
The existing brain/step1/canon_guard.py stays unchanged as an optional canon-reading utility.

## Responsibility split
- OpenClaw: conversation, memory retrieval, interpretation, and supplying goal/context/constraints.
- LangGraph: per-job planning steps, local worker execution, retries, checkpoint/resume, status reporting.
- Worker for the first local test: Ollama at 127.0.0.1:11434; no external-model forwarding.
- Ghost, NERVA, Character: unchanged; their links are not implemented here.

## Mac setup
Requires Python 3.11+, running Ollama with a locally installed model, and a compatible OpenClaw installation.

1. From the repository root:

```sh
cd brain/integration
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
export AKARI_OLLAMA_MODEL='your-installed-model'
export AKARI_API_TOKEN='a-long-random-locally-generated-secret'
export LANGGRAPH_STRICT_MSGPACK=true
uvicorn service:app --host 127.0.0.1 --port 8765
```

2. In a second terminal, install/build the OpenClaw plugin, and register the local folder per the OpenClaw version's official plugin instructions. Pass the same API token into the OpenClaw process:

```sh
cd brain/integration/openclaw-plugin
npm install
npm run build
```

The plugin targets the newer OpenClaw plugin SDK and requires a compatible Node.js/OpenClaw version. Do not assume the user's installed versions satisfy these requirements. Its runtime is not yet verified.

3. OpenClaw exposes:
- akari_delegate(goal, context, constraints, optional job_id) → immediate job ID
- akari_job_status(job_id) → current status and result/error
- akari_resume(job_id) → resume a partially checkpointed graph

OpenClaw should first retrieve relevant memory and pass it explicitly as context. This bridge itself does not automatically search or write OpenClaw memory. After successful completion OpenClaw must record the result through its normal memory workflow; the plugin only returns the status and result.

## HTTP smoke test without OpenClaw

```sh
curl -X POST http://127.0.0.1:8765/jobs \
  -H "X-AKARI-Token: $AKARI_API_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"job_id":"akari-test-001","goal":"与えられた情報を三行で整理して","context":"8bit図鑑は既存の完成が優先","constraints":["不明なことを推測しない"]}'
curl -H "X-AKARI-Token: $AKARI_API_TOKEN" http://127.0.0.1:8765/jobs/akari-test-001
```

A successful result returns status=completed and verification=nonempty_worker_response; this is **not a fact-check**. No model or failure returns failed, not a pretend completion. The same job_id is not intentionally executed twice.

## Known limits
- The SQLite AsyncSqliteSaver is for a single-machine prototype; not production-scale multi-worker.
- An OS crash before the first checkpoint can lose an accepted queued job; non-idempotent external side effects are not supported.
- A crash during a node can rerun that node. For Step 1 the worker only generates text; it does not modify files.
- Local token-protected loopback service only. Do not send sensitive memory to untrusted external AI.
- True OpenClaw memory search, Mac runtime, plugin build, crash-restart recovery, and parallel conversation must be verified on actual devices before declaring Step 1 complete.
- No existing LOCK changed.

## Tests
```sh
python3 -m unittest discover -s tests -v
python3 -m compileall -q service.py
```
