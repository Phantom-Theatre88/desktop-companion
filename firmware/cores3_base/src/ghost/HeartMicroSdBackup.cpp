#include "HeartMicroSdBackup.h"

#include "HeartEngine.h"

namespace deskbot {
namespace ghost {

void HeartMicroSdBackup::setHandlers(HeartBackupSaveHandler save_handler,
                                     HeartBackupLoadHandler load_handler,
                                     void* context) {
  save_handler_ = save_handler;
  load_handler_ = load_handler;
  context_ = context;
}

bool HeartMicroSdBackup::save(const HeartState& state) const {
  if (save_handler_ == nullptr) {
    return false;
  }
  return save_handler_(state, context_);
}

bool HeartMicroSdBackup::load(HeartState& out_state) const {
  if (load_handler_ == nullptr) {
    return false;
  }
  return load_handler_(out_state, context_);
}

}  // namespace ghost
}  // namespace deskbot
