"""AKARI Brain Step 1: deterministic, read-only canonical LOCK check.

No LLM and no write access. This is a small guardrail, not a complete
semantic conflict resolver or a finished long-term memory system.
"""
import json
import os
import urllib.error
import urllib.parse
import urllib.request

REPO = "Phantom-Theatre88/desktop-companion"
BRANCH = "heart-engine"
PREFIX = "docs/sacred/"
CURRENT = {
    "architecture": "LOCK_AKARI_BRAIN_OPENCLAW_LANGGRAPH_ARCHITECTURE_2026-11-14.md",
    "goal": "LOCK_DESKTOP_AI_THREE_PILLARS_IMPLEMENTATION_2026-11-09.md",
    "stack": "LOCK_DESKTOP_AI_INITIAL_TECH_STACK_2026-11-09.md",
    "name": "LOCK_AKARI_PROJECT_NAME_2026-11-12.md",
}
AKARI_PREFIXES = ("LOCK_AKARI_", "LOCK_DESKTOP_AI_")


class CanonError(RuntimeError):
    pass


def _get(url, token=None):
    headers = {"User-Agent": "akari-step1/0.1"}
    if token:
        headers["Authorization"] = "Bearer " + token
    try:
        with urllib.request.urlopen(
            urllib.request.Request(url, headers=headers), timeout=15
        ) as response:
            data = response.read(1_000_001)
        if len(data) > 1_000_000:
            raise CanonError("Source too large")
        return data.decode("utf-8")
    except (urllib.error.URLError, UnicodeError) as exc:
        raise CanonError("GitHub source inaccessible: " + str(exc)) from exc


def fetch_text(path, token=None):
    if not path.startswith(PREFIX) or ".." in path.split("/"):
        raise CanonError("Path outside sacred directory")
    url = (
        "https://raw.githubusercontent.com/" + REPO + "/" + BRANCH
        + "/" + urllib.parse.quote(path, safe="/")
    )
    return _get(url, token)


def list_sacred_names(token=None):
    url = (
        "https://api.github.com/repos/" + REPO
        + "/contents/docs/sacred?ref=" + BRANCH
    )
    try:
        listing = json.loads(_get(url, token))
        if not isinstance(listing, list):
            raise CanonError("Cannot list sacred directory")
        return [item["name"] for item in listing if item.get("type") == "file"]
    except (ValueError, KeyError, TypeError) as exc:
        raise CanonError("Cannot validate sacred file inventory") from exc


def evaluate(texts, filenames=None):
    missing = [role for role, name in CURRENT.items() if not texts.get(name, "").strip()]
    if missing:
        raise CanonError("Required canonical source missing: " + ", ".join(missing))
    a, g, s, n = (texts[CURRENT[k]] for k in ("architecture", "goal", "stack", "name"))
    checks = {
        "design_lock": all(x in a for x in ("設計方針LOCK", "OpenClaw", "LangGraph")),
        "three_pillars": all(x in g for x in ("Brain", "Ghost", "Character")),
        "python": "| Brain | **Python** |" in s,
        "websocket": "ローカルWebSocket" in s,
        "name": "正式名称：AKARI" in n,
        "hardware_undecided": "最終OS、ハードウェア" in a and "未決定" in a,
        "tool_selection_not_final": "製品採用の最終確定" in a,
    }
    if not all(checks.values()):
        raise CanonError(
            "Canonical text changed: " + ", ".join(k for k, ok in checks.items() if not ok)
        )
    if filenames is not None:
        unknown = sorted(
            file for file in filenames
            if file.startswith(AKARI_PREFIXES)
            and file.endswith(".md") and file not in CURRENT.values()
        )
        if unknown:
            raise CanonError("Unreviewed AKARI source(s): " + ", ".join(unknown))
    return {
        "status": "source_assertions_verified",
        "repo": REPO,
        "branch": BRANCH,
        "source_paths": {key: PREFIX + value for key, value in CURRENT.items()},
        "facts": {
            "project": "AKARI",
            "brain_language": "Python",
            "interpillar_transport": "local WebSocket (initial LOCK)",
            "brain_design": "OpenClaw + LangGraph + AKARI memory (first integration candidate)",
            "final_hardware": "undecided",
            "final_openclaw_langgraph_adoption": "undecided",
        },
        "limits": "No semantic conflict resolution, date-based authority inference, or write access.",
    }


def main():
    token = os.environ.get("GITHUB_TOKEN")
    try:
        filenames = list_sacred_names(token)
        texts = {name: fetch_text(PREFIX + name, token) for name in CURRENT.values()}
        result = evaluate(texts, filenames)
        print(json.dumps(result, ensure_ascii=False, indent=2))
        return 0
    except CanonError as exc:
        print(json.dumps({"status": "blocked", "reason": str(exc)}, ensure_ascii=False))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
