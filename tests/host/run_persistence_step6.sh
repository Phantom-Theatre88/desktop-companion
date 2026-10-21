#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/../.."
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/persistence-step6.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT
base=firmware/cores3_base/src
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Itests/face/stubs -Itests/host/storage_stubs \
  tests/host/persistence_step6_test.cpp "$base/ghost/GhostCore.cpp" \
  "$base/ghost/GhostPersistence.cpp" "$base/ghost/MemoryEngine.cpp" \
  "$base/ghost/HeartEngine.cpp" "$base/ghost/HeartPersistence.cpp" \
  "$base/ghost/HeartMicroSdBackup.cpp" "$base/ghost/BehaviorEngine.cpp" \
  -o "$test_dir/persistence"
"$test_dir/persistence"
"$test_dir/persistence" write "$test_dir/durable"
"$test_dir/persistence" read "$test_dir/durable"
