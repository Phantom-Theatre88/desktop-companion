#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/../.."
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/heart-step4.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT
base=firmware/cores3_base/src
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -Itests/face/stubs \
  tests/host/heart_step4_test.cpp "$base/ghost/MemoryEngine.cpp" \
  "$base/ghost/GhostCore.cpp" "$base/ghost/HeartEngine.cpp" \
  "$base/reflex/ReflexLayer.cpp" "$base/ghost/BehaviorEngine.cpp" "$base/ghost/HeartMicroSdBackup.cpp" \
  -o "$test_dir/relationship"
"$test_dir/relationship"
