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

# Vision2 cutover:
# - keep the legacy YukiVision source available as an asset
# - do NOT start legacy YukiVision at boot
# - start/enable the independent YukiVision2 person detector instead
# - route any later runtime re-enable (e.g. LISTENING state) to Vision2 as well
# - remove the old coordinate-remapping/debug injection from the prepare step
#
# This gives Vision2 the camera/person-recognition validation path without
# deleting the legacy implementation, so rollback/comparison remains possible.
display_file="$target_dir/firmware/main/hal/board/stackchan_display.cc"

python3 - "$display_file" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()

vision2_include = '#include <stackchan/vision2/yuki_vision2.h>'
if vision2_include not in text:
    anchor = '#include <stackchan/vision/yuki_vision.h>'
    if anchor not in text:
        raise SystemExit('Unable to locate Yuki vision include insertion point')
    text = text.replace(anchor, anchor + '\n' + vision2_include, 1)

# The integration patch currently starts the legacy Vision task beside
# curiosity. Replace that boot sequence with Vision2. Also accept a workspace
# shape that still contains the previous boot-time EnableYukiVision() line.
legacy_sequences = [
    '    StartYukiVision();\n    EnableYukiVision();\n    StartYukiCuriosity();',
    '    StartYukiVision();\n    StartYukiCuriosity();',
]
vision2_sequence = (
    '    StartYukiVision2();\n'
    '    EnableYukiVision2();\n'
    '    StartYukiCuriosity();'
)

if vision2_sequence not in text:
    for old in legacy_sequences:
        if old in text:
            text = text.replace(old, vision2_sequence, 1)
            break
    else:
        raise SystemExit('Unable to locate legacy YukiVision boot sequence')

# The original integration also re-enables vision when entering LISTENING.
# After the boot cutover, any remaining legacy runtime enable must be routed
# to Vision2; otherwise the old detector is silently revived later.
text = text.replace('    EnableYukiVision();', '    EnableYukiVision2();')

path.write_text(text)
PY

# Verify the generated workspace really boots Vision2 and does not boot or
# re-enable the legacy detector anywhere in this display integration.
python3 - "$display_file" <<'PY'
from pathlib import Path
import sys

text = Path(sys.argv[1]).read_text()
required = '    StartYukiVision2();\n    EnableYukiVision2();\n    StartYukiCuriosity();'
if required not in text:
    raise SystemExit('Boot-time Vision2 Start/Enable sequence is missing')
if '    StartYukiVision();' in text:
    raise SystemExit('Legacy YukiVision is still started at boot')
if '    EnableYukiVision();' in text:
    raise SystemExit('Legacy YukiVision is still enabled at runtime')
if '#include <stackchan/vision2/yuki_vision2.h>' not in text:
    raise SystemExit('YukiVision2 header was not inserted')
PY

# Compatibility: Yuki integration was written against a Xiaozhi API that is
# not present in the pinned v2.2.4 source. Apply this deterministically to the
# prepared workspace and verify it before build.
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

printf 'Prepared firmware workspace with YukiVision2 at %s\n' "$target_dir"