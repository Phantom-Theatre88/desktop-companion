#pragma once

#include "HeartMicroSdBackup.h"

namespace deskbot {
namespace ghost {

struct HeartState;

class HeartPersistence {
 public:
  bool hasPrimarySnapshot();
  bool loadPrimary(HeartState& out_state);
  bool savePrimary(const HeartState& state);

  void setMicroSdBackupHandlers(HeartBackupSaveHandler save_handler,
                                HeartBackupLoadHandler load_handler,
                                void* context = nullptr);
  bool microSdBackupAvailable() const;
  bool saveBackup(const HeartState& state) const;
  bool loadBackup(HeartState& out_state) const;

 private:
  HeartMicroSdBackup micro_sd_backup_{};
};

}  // namespace ghost
}  // namespace deskbot
