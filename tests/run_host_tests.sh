#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/deskrobo-tests.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT
compiler=${CXX:-c++}
flags=(-std=c++17 -Wall -Wextra -Werror -Itests/face/stubs)
base=firmware/cores3_base/src
"$compiler" "${flags[@]}" tests/face/face_test.cpp "$base/face/FaceRenderer.cpp" "$base/ghost/BehaviorEngine.cpp" -o "$test_dir/face"
"$test_dir/face"
"$compiler" "${flags[@]}" tests/vision/vision_test.cpp \
  "$base/vision/CameraVisionInput.cpp" "$base/core/DesktopCompanionRuntime.cpp" \
  "$base/nerve/SynapseRouter.cpp" "$base/ghost/GhostCore.cpp" \
  "$base/ghost/BehaviorEngine.cpp" "$base/ghost/HeartEngine.cpp" \
  "$base/ghost/HeartMicroSdBackup.cpp" "$base/ghost/MemoryEngine.cpp" \
  "$base/reflex/ReflexLayer.cpp" -o "$test_dir/vision"
"$test_dir/vision"
