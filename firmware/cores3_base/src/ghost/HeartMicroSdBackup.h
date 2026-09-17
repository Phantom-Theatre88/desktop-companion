#pragma once

namespace deskbot {
namespace ghost {

struct HeartState;

using HeartBackupSaveHandler = bool (*)(const HeartState& state, void* context);
using HeartBackupLoadHandler = bool (*)(HeartState& out_state, void* context);

class HeartMicroSdBackup {
 public:
  void setHandlers(HeartBackupSaveHandler save_handler,
                   HeartBackupLoadHandler load_handler,
                   void* context = nullptr);

  bool canSave() const { return save_handler_ != nullptr; }
  bool canLoad() const { return load_handler_ != nullptr; }

  bool save(const HeartState& state) const;
  bool load(HeartState& out_state) const;

 private:
  HeartBackupSaveHandler save_handler_ = nullptr;
  HeartBackupLoadHandler load_handler_ = nullptr;
  void* context_ = nullptr;
};

}  // namespace ghost
}  // namespace deskbot
