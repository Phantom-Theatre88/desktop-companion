#include "CameraVisionInput.h"

namespace deskbot {
namespace vision {
namespace {
// Perception tuning, not personality rules. About two seconds between captures.
constexpr uint32_t kMaxFrameGapMs = 6500;
constexpr uint32_t kEventCooldownMs = 1000;
constexpr int kBrightnessThreshold = 24;
constexpr int kCellChangeThreshold = 20;
constexpr size_t kCandidateBlobMinCells = 5;
int magnitude(int v) { return v < 0 ? -v : v; }

struct MotionBlob {
  size_t cells = 0;
  size_t min_col = 0;
  size_t max_col = 0;
  size_t min_row = 0;
  size_t max_row = 0;
  float center_x = 0.0f;
  float center_y = 0.0f;
};

size_t neighborCount8(const bool* mask, size_t row, size_t col) {
  size_t count = 0;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) continue;
      const int y = static_cast<int>(row) + dy;
      const int x = static_cast<int>(col) + dx;
      if (x < 0 || y < 0 || x >= 16 || y >= 12) {
        continue;
      }
      if (mask[static_cast<size_t>(y) * 16 + static_cast<size_t>(x)]) {
        ++count;
      }
    }
  }
  return count;
}
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

    bool raw_mask[kCells]{};
    bool supported_mask[kCells]{};
    bool cleaned_mask[kCells]{};

    uint16_t horizontal_changed[3] = {0, 0, 0};
    uint16_t horizontal_cells[3] = {0, 0, 0};
    uint16_t vertical_changed[3] = {0, 0, 0};
    uint16_t vertical_cells[3] = {0, 0, 0};

    // Stage 1: raw spatial difference after removing global illumination shift.
    for (size_t row = 0; row < kRows; ++row) {
      for (size_t col = 0; col < kColumns; ++col) {
        const size_t i = row * kColumns + col;
        const int residual =
            static_cast<int>(grid[i]) - previous_[i] - summary.luma_change;
        if (magnitude(residual) >= kCellChangeThreshold) {
          raw_mask[i] = true;
          ++summary.raw_changed_cells;
        }
      }
    }

    // Stage 2a: remove isolated one-cell noise. A changed cell needs at least
    // one changed neighbour in the surrounding 3x3 area.
    for (size_t row = 0; row < kRows; ++row) {
      for (size_t col = 0; col < kColumns; ++col) {
        const size_t i = row * kColumns + col;
        supported_mask[i] =
            raw_mask[i] && neighborCount8(raw_mask, row, col) >= 1;
      }
    }

    // Stage 2b: do not invent motion cells. The 16x12 grid is coarse, so
    // hole-filling can merge separate movements into one oversized blob.
    // Keep only supported raw cells; diagonal continuity is handled by the
    // 8-neighbour blob pass below.
    for (size_t i = 0; i < kCells; ++i) {
      cleaned_mask[i] = supported_mask[i];
    }

    // Stage 3: 8-neighbour connected components. This preserves actual changed
    // into explicit candidate blobs instead of averaging every changed cell.
    bool visited[kCells]{};
    size_t queue[kCells]{};
    size_t blob_count = 0;
    MotionBlob target_blob;
    bool have_target_blob = false;

    for (size_t start_cell = 0; start_cell < kCells; ++start_cell) {
      if (!cleaned_mask[start_cell] || visited[start_cell]) {
        continue;
      }

      MotionBlob blob;
      const size_t start_row = start_cell / kColumns;
      const size_t start_col = start_cell % kColumns;
      blob.min_col = blob.max_col = start_col;
      blob.min_row = blob.max_row = start_row;

      size_t head = 0;
      size_t tail = 0;
      queue[tail++] = start_cell;
      visited[start_cell] = true;
      float sum_col = 0.0f;
      float sum_row = 0.0f;

      while (head < tail) {
        const size_t index = queue[head++];
        const size_t row = index / kColumns;
        const size_t col = index % kColumns;

        ++blob.cells;
        ++summary.cleaned_changed_cells;
        sum_col += static_cast<float>(col);
        sum_row += static_cast<float>(row);

        const size_t hregion = (col * 3) / kColumns;
        const size_t vregion = (row * 3) / kRows;
        ++horizontal_changed[hregion];
        ++vertical_changed[vregion];

        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            const int ny = static_cast<int>(row) + dy;
            const int nx = static_cast<int>(col) + dx;
            if (nx < 0 || ny < 0 ||
                nx >= static_cast<int>(kColumns) ||
                ny >= static_cast<int>(kRows)) {
              continue;
            }
            const size_t n =
                static_cast<size_t>(ny) * kColumns +
                static_cast<size_t>(nx);
            if (cleaned_mask[n] && !visited[n]) {
              visited[n] = true;
              queue[tail++] = n;
            }
          }
        }
      }

      if (blob.cells == 0) {
        continue;
      }

      ++blob_count;
      const float center_col = sum_col / static_cast<float>(blob.cells);
      const float center_row = sum_row / static_cast<float>(blob.cells);
      blob.center_x =
          (center_col + 0.5f) / static_cast<float>(kColumns) * 2.0f - 1.0f;
      blob.center_y =
          (center_row + 0.5f) / static_cast<float>(kRows) * 2.0f - 1.0f;

      if (blob.cells >= kCandidateBlobMinCells) {
        ++summary.candidate_blob_count;
        if (!have_target_blob || blob.cells > target_blob.cells) {
          target_blob = blob;
          have_target_blob = true;
        }
      }
    }

    summary.blob_count = static_cast<uint16_t>(blob_count);

    for (size_t col = 0; col < kColumns; ++col) {
      ++horizontal_cells[(col * 3) / kColumns];
    }
    for (size_t row = 0; row < kRows; ++row) {
      ++vertical_cells[(row * 3) / kRows];
    }
    // Convert region cell counts from one-dimensional spans to full grid cell
    // counts so the ratios remain comparable with earlier diagnostics.
    for (size_t r = 0; r < 3; ++r) {
      horizontal_cells[r] *= kRows;
      vertical_cells[r] *= kColumns;
    }

    summary.motion_score =
        static_cast<float>(summary.cleaned_changed_cells) /
        static_cast<float>(kCells);
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

    // Stage 4: provisional tracking target. This is perception tuning, not a
    // personality LOCK. Keep only the largest qualifying blob; retaining all
    // 192 possible blobs would waste several KB of the Arduino task stack.
    if (have_target_blob) {
      summary.target_blob_cells =
          static_cast<uint16_t>(target_blob.cells);
      summary.motion_x = target_blob.center_x;
      summary.motion_y = target_blob.center_y;
    }

    nerve::NeuronType type = nerve::NeuronType::NONE;
    float strength = 0.0f;
    if (magnitude(summary.luma_change) >= kBrightnessThreshold) {
      type = summary.luma_change > 0
          ? nerve::NeuronType::BRIGHTER
          : nerve::NeuronType::DARKER;
      strength =
          static_cast<float>(magnitude(summary.luma_change)) / 255.0f;
    } else if (have_target_blob) {
      type = nerve::NeuronType::MOTION_DETECTED;
      strength = static_cast<float>(target_blob.cells) /
                 static_cast<float>(kCells);
    }

    if (type != nerve::NeuronType::NONE &&
        (!have_event_ ||
         frame.timestamp_ms - last_event_ms_ >= kEventCooldownMs)) {
      nerve::NeuronPayload payload;
      payload.scalar = strength;
      if (type == nerve::NeuronType::MOTION_DETECTED) {
        // Semantic normalized blob-center direction, never raw pixel position.
        payload.x =
            static_cast<int32_t>(summary.motion_x * 1000.0f);
        payload.y =
            static_cast<int32_t>(summary.motion_y * 1000.0f);
      }
      pending_neuron_ =
          nerve::makeNeuron(type, nerve::NeuronSource::CAMERA_M5,
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
