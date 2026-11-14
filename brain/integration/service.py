"""AKARI Brain Step 1: LangGraph job service invoked by an OpenClaw tool.

OpenClaw owns conversation and long-term memories. LangGraph receives context per job.
"""
from __future__ import annotations
import asyncio
import json
import os
import re
import secrets
from contextlib import asynccontextmanager
from pathlib import Path
from typing import TypedDict
from urllib.request import Request, urlopen

from fastapi import FastAPI, Header, HTTPException
from pydantic import BaseModel, Field
from langgraph.graph import START, END, StateGraph
from langgraph.checkpoint.sqlite.aio import AsyncSqliteSaver


class JobState(TypedDict, total=False):
    job_id: str
    goal: str
    context: str
    constraints: list[str]
    status: str
    result: str
    error: str
    attempts: int
    verification: str


class JobRequest(BaseModel):
    job_id: str = Field(pattern=r"^[A-Za-z0-9_-]{8,100}$")
    goal: str = Field(min_length=1, max_length=4000)
    context: str = Field(default="", max_length=16000)
    constraints: list[str] = Field(default_factory=list, max_length=20)


def build_prompt(state: JobState) -> str:
    constraints = "\n".join("- " + item for item in state.get("constraints", []))
    return (
        "You are a local AKARI worker. Complete the task from supplied context, "
        "do not invent external checks or file changes, reply in Japanese.\n\n"
        "Task:\n" + state["goal"] + "\nContext supplied by OpenClaw:\n"
        + state.get("context", "") + "\nConstraints:\n" + constraints
    )


def local_ollama(prompt: str) -> str:
    model = os.environ.get("AKARI_OLLAMA_MODEL", "").strip()
    if not model:
        raise RuntimeError("AKARI_OLLAMA_MODEL not configured")
    # Intentionally loopback only: do not leak OpenClaw context to an external AI.
    data = json.dumps({"model": model, "prompt": prompt, "stream": False}).encode()
    request = Request(
        "http://127.0.0.1:11434/api/generate", data=data,
        headers={"Content-Type": "application/json"}, method="POST",
    )
    with urlopen(request, timeout=120) as response:
        result = json.load(response)
    text = result.get("response")
    if not isinstance(text, str) or not text.strip():
        raise RuntimeError("Local model returned no text")
    return text.strip()


def build_graph(checkpointer):
    async def prepare(state: JobState) -> JobState:
        return {"status": "running", "attempts": 0, "error": "", "result": ""}

    async def execute(state: JobState) -> JobState:
        error = ""
        for attempt in (1, 2):
            try:
                result = await asyncio.to_thread(local_ollama, build_prompt(state))
                return {"status": "verifying", "result": result, "attempts": attempt}
            except Exception as exc:
                error = f"{type(exc).__name__}: {exc}"
                if attempt == 1:
                    await asyncio.sleep(0.2)
        return {"status": "failed", "error": error, "attempts": 2}

    async def finish(state: JobState) -> JobState:
        if state["status"] == "failed":
            return {"verification": "worker_failed"}
        return {"status": "completed", "verification": "nonempty_worker_response"}

    graph = StateGraph(JobState)
    for name, node in (("prepare", prepare), ("execute", execute), ("finish", finish)):
        graph.add_node(name, node)
    graph.add_edge(START, "prepare")
    graph.add_edge("prepare", "execute")
    graph.add_edge("execute", "finish")
    graph.add_edge("finish", END)
    return graph.compile(checkpointer=checkpointer)


@asynccontextmanager
async def lifespan(app: FastAPI):
    data_dir = Path(os.environ.get("AKARI_DATA_DIR", "./.akari-local-data"))
    data_dir.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("LANGGRAPH_STRICT_MSGPACK", "true")
    async with AsyncSqliteSaver.from_conn_string(str(data_dir / "jobs.sqlite")) as saver:
        await saver.setup()
        app.state.graph = build_graph(saver)
        app.state.running = {}
        app.state.lock = asyncio.Lock()
        yield
        await asyncio.gather(*app.state.running.values(), return_exceptions=True)


app = FastAPI(title="AKARI Brain Step 1", lifespan=lifespan)


def authorize(token: str | None) -> None:
    expected = os.environ.get("AKARI_API_TOKEN", "")
    if not expected:
        raise HTTPException(503, "AKARI_API_TOKEN is not configured")
    if token is None or not secrets.compare_digest(token, expected):
        raise HTTPException(401, "Unauthorized")


def graph_config(job_id: str) -> dict:
    return {"configurable": {"thread_id": job_id}}


def view(job_id: str, values: dict, pending=()) -> dict:
    return {
        "job_id": job_id,
        "status": values.get("status", "queued"),
        "result": values.get("result", ""),
        "error": values.get("error", ""),
        "attempts": values.get("attempts", 0),
        "verification": values.get("verification", ""),
        "resumable": bool(pending),
    }


async def run(job_id: str, initial: JobState | None) -> None:
    try:
        await app.state.graph.ainvoke(initial, graph_config(job_id), durability="sync")
    except Exception:
        # Latest successful checkpoint remains readable; do not claim success.
        pass
    finally:
        app.state.running.pop(job_id, None)


@app.post("/jobs", status_code=202)
async def submit(body: JobRequest, x_akari_token: str | None = Header(None)):
    authorize(x_akari_token)
    async with app.state.lock:
        if body.job_id in app.state.running:
            return {"job_id": body.job_id, "status": "running", "duplicate": True}
        prior = await app.state.graph.aget_state(graph_config(body.job_id))
        if prior.values:
            return {**view(body.job_id, prior.values, prior.next), "duplicate": True}
        initial: JobState = body.model_dump()
        initial["status"] = "queued"
        app.state.running[body.job_id] = asyncio.create_task(run(body.job_id, initial))
    return {"job_id": body.job_id, "status": "queued", "duplicate": False}


@app.get("/jobs/{job_id}")
async def status(job_id: str, x_akari_token: str | None = Header(None)):
    authorize(x_akari_token)
    if not re.fullmatch(r"[A-Za-z0-9_-]{8,100}", job_id):
        raise HTTPException(400, "Invalid job ID")
    snapshot = await app.state.graph.aget_state(graph_config(job_id))
    if snapshot.values:
        return view(job_id, snapshot.values, snapshot.next)
    if job_id in app.state.running:
        return {"job_id": job_id, "status": "queued"}
    raise HTTPException(404, "Unknown job")


@app.post("/jobs/{job_id}/resume", status_code=202)
async def resume(job_id: str, x_akari_token: str | None = Header(None)):
    authorize(x_akari_token)
    if not re.fullmatch(r"[A-Za-z0-9_-]{8,100}", job_id):
        raise HTTPException(400, "Invalid job ID")
    async with app.state.lock:
        if job_id in app.state.running:
            return {"job_id": job_id, "status": "running"}
        snapshot = await app.state.graph.aget_state(graph_config(job_id))
        if not snapshot.values:
            raise HTTPException(404, "Unknown job")
        if not snapshot.next:
            return view(job_id, snapshot.values, snapshot.next)
        app.state.running[job_id] = asyncio.create_task(run(job_id, None))
    return {"job_id": job_id, "status": "resuming"}
