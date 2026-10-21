#!/usr/bin/env python3
"""Mac host test for the production MemoryEngine.cpp logic (no M5 hardware).
Run from repository root: python3 tests/host/test_memory_step2.py
Uses temporary header shims ONLY for Arduino/Heart; production MemoryEngine.cpp
and its own header, NerveTypes.h, ReflexLayer.h are copied unchanged.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
src = root / "firmware/cores3_base/src"
with tempfile.TemporaryDirectory(prefix="kibi-memory-step2-") as tmp:
    work = Path(tmp)
    for rel in ("ghost/MemoryEngine.cpp", "ghost/MemoryEngine.h",
                "ghost/HeartEventEvidence.h",
                "nerve/NerveTypes.h", "reflex/ReflexLayer.h"):
        dest = work / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src / rel, dest)
    (work / "Arduino.h").write_text(
        "#pragma once\n#include <cstdint>\n#include <cstddef>\n")
    # MemoryEngine.h includes HeartEngine.h from the same directory.
    # Supply just the data structures it actually needs; never replace
    # MemoryEngine production implementation.
    (work / "ghost/HeartEngine.h").write_text(
        "#pragma once\n#include <Arduino.h>\n"
        "namespace deskbot { namespace ghost {\n"
        "struct HeartState { float mood=0.60f, affection=0.55f, "
        "curiosity=0.65f, boredom=0.15f, sleepiness=0.15f, attention=0.50f; };\n"
        "struct HeartContext { HeartState state{}, baseline{}; "
        "uint32_t captured_ms=0; };\n"
        "} }\n")
    (work / "test.cpp").write_text(r"""
#include <cassert>
#include <iostream>
#include "ghost/MemoryEngine.h"
using namespace deskbot;
int main() {
  ghost::MemoryEngine m;
  m.begin(0);
  ghost::MemoryRecord found;
  assert(!m.recallLatest(nerve::NeuronType::TOUCH, found));
  const auto touch = nerve::makeNeuron(nerve::NeuronType::TOUCH,
                                      nerve::NeuronSource::TOUCH, 100);
  ghost::HeartContext before, after;
  before.state.mood = 0.60f;
  after.state.mood = 0.63f;
  m.beginNeuronExperience(touch, before);
  assert(!m.recallLatest(nerve::NeuronType::TOUCH, found));
  assert(m.commitNeuronExperience(after));
  assert(!m.commitNeuronExperience(after));
  assert(m.recallLatest(nerve::NeuronType::TOUCH, found));
  assert(found.heart_context.state.mood == 0.60f);
  assert(found.outcome_context.state.mood == 0.63f);
  assert(found.has_outcome);
  assert(m.workingExperienceCount() == 1);
  auto shake = nerve::makeNeuron(nerve::NeuronType::SHAKE,
                                 nerve::NeuronSource::IMU, 200);
  m.beginNeuronExperience(shake, before);
  assert(m.recallLatest(nerve::NeuronType::TOUCH, found));
  assert(!m.recallLatest(nerve::NeuronType::SHAKE, found));
  assert(m.commitNeuronExperience(after));
  assert(m.recallLatest(nerve::NeuronType::SHAKE, found));
  // 8-slot working history: oldest item is evicted, newest remains.
  for (int i=0; i<9; ++i) {
    auto n = nerve::makeNeuron(nerve::NeuronType::BRIGHTER,
                               nerve::NeuronSource::ENV, 300+i);
    m.beginNeuronExperience(n, before);
    assert(m.commitNeuronExperience(after));
  }
  assert(m.workingExperienceCount() == 8);
  assert(!m.recallLatest(nerve::NeuronType::TOUCH, found));
  assert(m.recallLatest(nerve::NeuronType::BRIGHTER, found));
  assert(found.occurred_ms == 308);
  std::cout << "PASS: Memory Step 2 host tests\n";
}
""")
    executable = work / "memory_test"
    command = ["clang++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
               "-I", str(work), str(work / "ghost/MemoryEngine.cpp"),
               str(work / "test.cpp"), "-o", str(executable)]
    print("Compiling actual MemoryEngine.cpp with temporary platform shims...")
    subprocess.run(command, check=True)
    subprocess.run([str(executable)], check=True)
