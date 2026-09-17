#pragma once

namespace deskbot {
namespace ghost {

struct HeartState;

class HeartPersistence {
 public:
  bool hasPrimarySnapshot();
  bool loadPrimary(HeartState& out_state);
  bool savePrimary(const HeartState& state);
};

}  // namespace ghost
}  // namespace deskbot
