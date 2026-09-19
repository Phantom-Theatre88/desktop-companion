#include "CameraVisionInput.h"

namespace deskbot {
namespace vision {
namespace {
// Perception tuning, not personality rules. About two seconds between captures.
constexpr uint32_t kMaxFrameGapMs = 6500;
constexpr uint32_t kEventCooldownMs = 3000;
constexpr int kBrightnessThreshold = 24;
constexpr int kCellChangeThreshold = 20;
constexpr float kMotionFraction = 0.12f;
int magnitude(int v) { return v < 0 ? -v : v; }
}

void CameraVisionInput::reset() {
  have_previous_ = have_event_ = pending_ = false;
  last_summary_ = VisionFrameSummary{};
}

bool CameraVisionInput::takeNeuron(nerve::SemanticNeuron& out) {
  if (!pending_) return false;
  out = pending_neuron_;
  pending_ = false;
  return true;
}

void CameraVisionInput::onCameraFrame(const device::CameraFrameView& frame) {
  pending_ = false;
  VisionFrameSummary summary;
  summary.timestamp_ms = frame.timestamp_ms;
  summary.width = frame.width;
  summary.height = frame.height;
  summary.bytes = frame.bytes;
  // Division avoids overflow on malformed dimensions. Do not retain raw data.
  summary.valid = frame.data && frame.width >= kColumns && frame.height >= kRows &&
      (frame.bytes / 2 / frame.width) >= frame.height;
  if (!summary.valid) {
    reset();
    last_summary_ = summary;
    return;
  }

  uint8_t grid[kCells];
  uint32_t sum = 0;
  for (size_t row = 0; row < kRows; ++row) {
    for (size_t col = 0; col < kColumns; ++col) {
      // Four samples per cell reduce dependence on a single noisy pixel.
      uint16_t cell = 0;
      for (size_t dy = 1; dy <= 3; dy += 2) {
        for (size_t dx = 1; dx <= 3; dx += 2) {
          const size_t x = ((col * 4 + dx) * frame.width) / (kColumns * 4);
          const size_t y = ((row * 4 + dy) * frame.height) / (kRows * 4);
          cell += lumaAt(frame, x, y);
        }
      }
      grid[row * kColumns + col] = cell / 4;
      sum += cell / 4;
    }
  }
  summary.average_luma = sum / kCells;
  const uint32_t gap = frame.timestamp_ms - previous_ms_;
  const bool comparable = have_previous_ && gap > 0 && gap <= kMaxFrameGapMs &&
      frame.width == previous_width_ && frame.height == previous_height_;
  if (comparable) {
    summary.luma_change = static_cast<int>(summary.average_luma) - previous_mean_;
    size_t changed = 0;
    float weighted_x = 0.0f;
    float weighted_y = 0.0f;
    float direction_weight = 0.0f;
    float fallback_x = 0.0f;
    float fallback_y = 0.0f;
    uint16_t horizontal_changed[3] = {0, 0, 0};
    uint16_t horizontal_cells[3] = {0, 0, 0};
    uint16_t vertical_changed[3] = {0, 0, 0};
    uint16_t vertical_cells[3] = {0, 0, 0};

    for (size_t row = 0; row < kRows; ++row) {
      for (size_t col = 0; col < kColumns; ++col) {
        const size_t i = row * kColumns + col;
        const size_t horizontal_region = (col * 3) / kColumns;
        const size_t vertical_region = (row * 3) / kRows;
        ++horizontal_cells[horizontal_region];
        ++vertical_cells[vertical_region];

        // Remove a global illumination shift before classifying spatial change.
        const int residual =
            static_cast<int>(grid[i]) - previous_[i] - summary.luma_change;
        if (magnitude(residual) < kCellChangeThreshold) {
          continue;
        }

        ++changed;
        ++horizontal_changed[horizontal_region];
        ++vertical_changed[vertical_region];
        const float nx =
            (static_cast<float>(col) + 0.5f) /
                static_cast<float>(kColumns) * 2.0f - 1.0f;
        const float ny =
            (static_cast<float>(row) + 0.5f) /
                static_cast<float>(kRows) * 2.0f - 1.0f;
        fallback_x += nx;
        fallback_y += ny;

        // Prefer cells that are visually salient in the CURRENT frame. This
        // reduces the tendency of frame differencing to look halfway between
        // an object's old and new positions after it moves.
        const float current_contrast = static_cast<float>(
            magnitude(static_cast<int>(grid[i]) -
                      static_cast<int>(summary.average_luma)));
        const float weight = current_contrast + 1.0f;
        weighted_x += nx * weight;
        weighted_y += ny * weight;
        direction_weight += weight;
      }
    }

    summary.motion_score = static_cast<float>(changed) / kCells;
    summary.motion_left = horizontal_cells[0]
        ? static_cast<float>(horizontal_changed[0]) / horizontal_cells[0] : 0.0f;
    summary.motion_center = horizontal_cells[1]
        ? static_cast<float>(horizontal_changed[1]) / horizontal_cells[1] : 0.0f;
    summary.motion_right = horizontal_cells[2]
        ? static_cast<float>(horizontal_changed[2]) / horizontal_cells[2] : 0.0f;
    summary.motion_top = vertical_cells[0]
        ? static_cast<float>(vertical_changed[0]) / vertical_cells[0] : 0.0f;
    summary.motion_middle = vertical_cells[1]
        ? static_cast<float>(vertical_changed[1]) / vertical_cells[1] : 0.0f;
    summary.motion_bottom = vertical_cells[2]
        ? static_cast<float>(vertical_changed[2]) / vertical_cells[2] : 0.0f;

    if (changed > 0) {
      if (direction_weight > static_cast<float>(changed) * 2.0f) {
        summary.motion_x = weighted_x / direction_weight;
        summary.motion_y = weighted_y / direction_weight;
      } else {
        summary.motion_x = fallback_x / static_cast<float>(changed);
        summary.motion_y = fallback_y / static_cast<float>(changed);
      }
    }
    nerve::NeuronType type = nerve::NeuronType::NONE;
    float strength = 0;
    if (magnitude(summary.luma_change) >= kBrightnessThreshold) {
      type = summary.luma_change > 0 ? nerve::NeuronType::BRIGHTER : nerve::NeuronType::DARKER;
      strength = static_cast<float>(magnitude(summary.luma_change)) / 255.0f;
    } else if (summary.motion_score >= kMotionFraction) {
      type = nerve::NeuronType::MOTION_DETECTED;
      strength = summary.motion_score;
    }
    if (type != nerve::NeuronType::NONE &&
        (!have_event_ || frame.timestamp_ms - last_event_ms_ >= kEventCooldownMs)) {
      nerve::NeuronPayload payload;
      payload.scalar = strength;
      if (type == nerve::NeuronType::MOTION_DETECTED) {
        // Semantic normalized direction, never raw camera pixel coordinates.
        payload.x = static_cast<int32_t>(summary.motion_x * 1000.0f);
        payload.y = static_cast<int32_t>(summary.motion_y * 1000.0f);
      }
      pending_neuron_ = nerve::makeNeuron(type, nerve::NeuronSource::CAMERA_M5,
                                          frame.timestamp_ms, 1.0f, payload);
      pending_ = have_event_ = true;
      last_event_ms_ = frame.timestamp_ms;
    }
  }
  for (size_t i = 0; i < kCells; ++i) previous_[i] = grid[i];
  previous_mean_ = summary.average_luma;
  previous_ms_ = frame.timestamp_ms;
  previous_width_ = frame.width;
  previous_height_ = frame.height;
  have_previous_ = true;
  last_summary_ = summary;
}

uint8_t CameraVisionInput::lumaAt(const device::CameraFrameView& frame, size_t x, size_t y) {
  const size_t i = (y * frame.width + x) * 2;
  // esp32-camera RGB565 buffers carry the high byte first (fmt2rgb888).
  const uint16_t pixel = (static_cast<uint16_t>(frame.data[i]) << 8) |
                         static_cast<uint16_t>(frame.data[i + 1]);
  const uint16_t r = ((pixel >> 11) & 31) * 255u / 31u;
  const uint16_t g = ((pixel >> 5) & 63) * 255u / 63u;
  const uint16_t b = (pixel & 31) * 255u / 31u;
  return (r * 30u + g * 59u + b * 11u) / 100u;
}

}  // namespace vision
}  // namespace deskbot
