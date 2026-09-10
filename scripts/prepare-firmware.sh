#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
target_arg="${1:-build/stackchan}"

if [[ "$target_arg" = /* ]]; then
    target_dir="$target_arg"
else
    target_dir="$repo_root/$target_arg"
fi

case "$target_dir" in
    "$repo_root"/*) ;;
    *)
        printf 'Build directory must be inside the repository: %s\n' "$target_dir" >&2
        exit 1
        ;;
esac

if [[ -e "$target_dir" ]]; then
    printf 'Refusing to overwrite existing build directory: %s\n' "$target_dir" >&2
    exit 1
fi

mkdir -p "$(dirname "$target_dir")"
relative_target="${target_dir#$repo_root/}"
cp -R "$repo_root/upstream/stackchan" "$target_dir"
git -C "$repo_root" apply --directory="$relative_target" "$repo_root/patches/yuki-stackchan-integration.patch"

# Normalize ArduinoJson for ESP-IDF Component Manager.
# The manifest uses the registry name bblanchon/arduinojson, while CMake must
# require the generated component target name bblanchon__arduinojson.
manifest="$target_dir/firmware/main/idf_component.yml"
cmake_file="$target_dir/firmware/main/CMakeLists.txt"
python3 - "$manifest" "$cmake_file" <<'PY'
from pathlib import Path
import sys

manifest = Path(sys.argv[1])
cmake_file = Path(sys.argv[2])

manifest_text = manifest.read_text()
dependency = "  bblanchon/arduinojson: ^6.21.6\n"
if "bblanchon/arduinojson:" not in manifest_text:
    marker = "dependencies:\n"
    if marker not in manifest_text:
        raise SystemExit(f"dependencies section not found in {manifest}")
    manifest_text = manifest_text.replace(marker, marker + dependency, 1)
    manifest.write_text(manifest_text)

cmake_text = cmake_file.read_text()
if "ArduinoJson" in cmake_text:
    cmake_text = cmake_text.replace("                        ArduinoJson\n", "                        bblanchon__arduinojson\n")
    cmake_file.write_text(cmake_text)

if "bblanchon__arduinojson" not in cmake_file.read_text():
    raise SystemExit(f"ArduinoJson component normalization failed in {cmake_file}")
PY

rsync -a "$repo_root/firmware/yuki/" "$target_dir/firmware/"
mkdir -p "$target_dir/firmware/patches"
cp "$repo_root/patches/xiaozhi-esp32.patch" "$target_dir/firmware/patches/"

printf 'Prepared firmware workspace at %s\n' "$target_dir"
