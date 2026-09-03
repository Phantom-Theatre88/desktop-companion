import os
import subprocess
import json


def ensure_step2_xiaozhi_compat(repo_path):
    """Add a no-op compatibility method only when pinned Xiaozhi lacks it.

    Yuki integration calls Application::StartProactiveConversation(), but
    Xiaozhi v2.2.4 has no such API. Step 2 does not use proactive speech, so a
    local no-op method is sufficient and avoids coupling expression/vision work
    to a later conversation feature.
    """
    application_h = os.path.join(repo_path, "main", "application.h")
    if not os.path.exists(application_h):
        return

    with open(application_h, "r", encoding="utf-8") as f:
        text = f.read()

    if "StartProactiveConversation" in text:
        return

    anchor = "    void ToggleChatState();\n"
    if anchor not in text:
        raise RuntimeError(
            "Cannot install Step 2 Xiaozhi compatibility shim: application.h layout changed"
        )

    shim = (
        "    // Desktop Companion Step 2 compatibility shim.\n"
        "    // Proactive conversation is implemented in a later step.\n"
        "    void StartProactiveConversation(const std::string& prompt) { (void)prompt; }\n\n"
    )
    text = text.replace(anchor, anchor + shim, 1)

    with open(application_h, "w", encoding="utf-8") as f:
        f.write(text)

    print("Installed Step 2 Xiaozhi compatibility shim")


def clone_or_update_repo(
    repo_url, path, ref=None, with_submodules=False, patch_path=None
):
    if not os.path.exists(path):
        subprocess.run(["git", "clone", repo_url, path], check=True)
    else:
        subprocess.run(["git", "-C", path, "fetch"], check=True)

    if ref:
        subprocess.run(["git", "-C", path, "checkout", ref], check=True)

    if with_submodules:
        subprocess.run(
            ["git", "-C", path, "submodule", "update", "--init", "--recursive"],
            check=True,
        )

    if patch_path:
        patch_full_path = (
            patch_path
            if os.path.isabs(patch_path)
            else os.path.join(os.getcwd(), patch_path)
        )
        check_result = subprocess.run(
            ["git", "-C", path, "apply", "--check", patch_full_path]
        )
        if check_result.returncode == 0:
            subprocess.run(["git", "-C", path, "apply", patch_full_path], check=True)
            print(f"Applied patch {patch_path} to {path}")
        else:
            print(f"Patch {patch_path} cannot be applied cleanly to {path}, skipped.")

    if repo_url.rstrip("/").endswith("78/xiaozhi-esp32.git"):
        ensure_step2_xiaozhi_compat(path)


def fetch_dependencies():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_path = os.path.join(script_dir, "repos.json")

    with open(config_path) as f:
        repos = json.load(f)

    for repo in repos:
        repo_path = os.path.join(script_dir, repo["path"])
        branch = repo.get("branch")
        with_submodules = repo.get("with_submodules", False)
        patch = repo.get("patch")
        if patch and not os.path.isabs(patch):
            patch = os.path.join(script_dir, patch)
        clone_or_update_repo(repo["url"], repo_path, branch, with_submodules, patch)


if __name__ == "__main__":
    fetch_dependencies()
