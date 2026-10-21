#pragma once
#include "MemoryEngine.h"

namespace deskbot { namespace ghost {
struct GhostCheckpoint {
  uint64_t generation = 0;
  uint64_t unix_seconds = 0;
  uint32_t quiet_seconds = 0;
  HeartState heart{}, baseline{};
  MemorySnapshot memory{};
};
// Slot I/O returns exact bytes; missing/failed reads return zero. Backends never
// interpret snapshots. Primary and backup use the same versioned codec.
using CheckpointRead = size_t (*)(unsigned slot, uint8_t* data, size_t capacity, void*);
using CheckpointWrite = bool (*)(unsigned slot, const uint8_t* data, size_t size, void*);
struct CheckpointStore {
  CheckpointRead read = nullptr;
  CheckpointWrite write = nullptr;
  void* context = nullptr;
};
enum class CheckpointSource : uint8_t { NONE, PRIMARY, BACKUP };
class GhostPersistence {
 public:
  static constexpr size_t kMaxBytes = 4096;
  void configure(CheckpointStore primary, CheckpointStore backup = {}) {
    primary_ = primary; backup_ = backup;
  }
  bool load(GhostCheckpoint& out);
  bool save(const GhostCheckpoint& snapshot);
  CheckpointSource source() const { return source_; }
  bool primarySaveOk() const { return primary_save_ok_; }
  bool backupSaveOk() const { return backup_save_ok_; }
  uint64_t generation() const { return generation_; }
  static size_t encode(const GhostCheckpoint& snapshot, uint8_t* data, size_t capacity);
  static bool decode(const uint8_t* data, size_t size, GhostCheckpoint& out);
 private:
  bool loadStore(const CheckpointStore& store, GhostCheckpoint& out);
  bool writeStore(const CheckpointStore& store, unsigned slot, const uint8_t* data,
                  size_t size);
  CheckpointStore primary_{}, backup_{};
  CheckpointSource source_ = CheckpointSource::NONE;
  uint64_t generation_ = 0;
  bool primary_save_ok_ = false, backup_save_ok_ = false;
  // Keep buffers off the embedded task stack. No dynamic allocation.
  GhostCheckpoint saving_{};
  uint8_t buffer_[kMaxBytes]{};
  uint8_t verification_[kMaxBytes]{};
};
} }
