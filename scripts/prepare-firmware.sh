#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
target_dir="${1:-"$repo_root/build/stackchan"}"

if [[ -e "$target_dir" ]]; then
    printf 'Refusing to overwrite existing build directory: %s\n' "$target_dir" >&2
    exit 1
fi

mkdir -p "$(dirname "$target_dir")"
cp -R "$repo_root/upstream/stackchan" "$target_dir"

relative_target="${target_dir#$repo_root/}"
if [[ "$relative_target" == "$target_dir" ]]; then
    printf 'Build directory must be inside the repository: %s\n' "$target_dir" >&2
    exit 1
fi

git -C "$repo_root" apply --directory="$relative_target" "$repo_root/patches/yuki-stackchan-integration.patch"
rsync -a "$repo_root/firmware/yuki/" "$target_dir/firmware/"
git -C "$target_dir/firmware" apply "$repo_root/patches/yuki-local-vision.patch"

# Step 2 compatibility: Yuki integration was written against a Xiaozhi API
# that is not present in the pinned v2.2.4 source. Apply this deterministically
# to the prepared workspace and verify it before build.
compat_file="$target_dir/firmware/main/hal/board/stackchan.cc"
python3 - "$compat_file" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
old = "    Application::GetInstance().StartProactiveConversation(prompt);"
new = "    // Step 2 compatibility shim: proactive conversation is deferred to a later step.\n    (void)prompt;"
if old in text:
    text = text.replace(old, new, 1)
    path.write_text(text)
elif "StartProactiveConversation(prompt)" in text:
    raise SystemExit("Unexpected proactive-conversation call shape; refusing to prepare firmware")
PY

if grep -q 'StartProactiveConversation(prompt)' "$compat_file"; then
    printf 'Compatibility replacement failed in %s\n' "$compat_file" >&2
    exit 1
fi

mkdir -p "$target_dir/firmware/patches"
cp "$repo_root/patches/xiaozhi-esp32.patch" "$target_dir/firmware/patches/"

printf 'Prepared firmware workspace at %s\n' "$target_dir"
