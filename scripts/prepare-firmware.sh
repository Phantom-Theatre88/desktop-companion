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
rsync -a "$repo_root/firmware/yuki/" "$target_dir/firmware/"
mkdir -p "$target_dir/firmware/patches"
cp "$repo_root/patches/xiaozhi-esp32.patch" "$target_dir/firmware/patches/"

printf 'Prepared firmware workspace at %s\n' "$target_dir"
