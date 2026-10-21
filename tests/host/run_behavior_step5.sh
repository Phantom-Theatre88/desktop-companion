#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/../.."
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/behavior-step5.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT
base=firmware/cores3_base/src
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -Itests/face/stubs \
  tests/host/behavior_step5_test.cpp "$base/core/DesktopCompanionRuntime.cpp" "$base/nerve/SynapseRouter.cpp" \
  "$base/body/BodyOutputComposer.cpp" "$base/face/FaceRenderer.cpp" \
  "$base/ghost/MemoryEngine.cpp" \
  "$base/ghost/GhostCore.cpp" "$base/ghost/HeartEngine.cpp" \
  "$base/reflex/ReflexLayer.cpp" "$base/ghost/BehaviorEngine.cpp" "$base/ghost/HeartMicroSdBackup.cpp" \
  -o "$test_dir/relationship"
"$test_dir/relationship"
