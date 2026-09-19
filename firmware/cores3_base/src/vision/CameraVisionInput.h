#pragma once

#include <Arduino.h>
#include "../device/CoreS3CameraDriver.h"
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace vision {

struct VisionFrameSummary {
  bool valid = false;
  uint32_t timestamp_ms = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  size_t bytes = 0;
  uint8_t average_luma = 0;
  float motion_score = 0.0f;  // fraction of changed grid cells, not person confidence
  float motion_x = 0.0f;      // semantic direction: left -1 .. right +1
  float motion_y = 0.0f;      // semantic direction: up -1 .. down +1
  float motion_left = 0.0f;
  float motion_center = 0.0f;
  float motion_right = 0.0f;
  float motion_top = 0.0f;
  float motion_middle = 0.0f;
  float motion_bottom = 0.0f;
  uint16_t raw_changed_cells = 0;
  uint16_t cleaned_changed_cells = 0;
  uint16_t blob_count = 0;
  uint16_t candidate_blob_count = 0;
  uint16_t target_blob_cells = 0;
  int16_t luma_change = 0;
};

class CameraVisionInput : public device::CameraFrameConsumer {
 public:
  void onCameraFrame(const device::CameraFrameView& frame) override;
  const VisionFrameSummary& lastSummary() const { return last_summary_; }
  bool takeNeuron(nerve::SemanticNeuron& out);
  // Re-arm after capture failure or known camera/body movement. First frame
  // establishes a baseline and never emits an event.
  void reset();

 private:
  static constexpr size_t kColumns = 16, kRows = 12;
  static constexpr size_t kCells = kColumns * kRows;
  static uint8_t lumaAt(const device::CameraFrameView& frame, size_t x, size_t y);
  VisionFrameSummary last_summary_{};
  uint8_t previous_[kCells]{};
  uint16_t previous_width_ = 0, previous_height_ = 0;
  uint32_t previous_ms_ = 0, last_event_ms_ = 0;
  uint8_t previous_mean_ = 0;
  bool have_previous_ = false, have_event_ = false, pending_ = false;
  nerve::SemanticNeuron pending_neuron_{};
};

}  // namespace vision
}  // namespace deskbot
