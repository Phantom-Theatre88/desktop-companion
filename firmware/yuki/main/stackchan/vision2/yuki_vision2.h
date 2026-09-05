/*
 * YukiVision2: independent person-detection path for the desktop companion.
 *
 * This intentionally lives beside the legacy YukiVision implementation so we
 * can validate person detection without changing the existing face-following
 * behaviour.
 */
#pragma once

#include <cstdint>

namespace stackchan {

struct YukiPersonObservation {
    bool detected = false;
    int frame_width = 0;
    int frame_height = 0;
    int x1 = -1;
    int y1 = -1;
    int x2 = -1;
    int y2 = -1;
    int center_x = -1;
    int center_y = -1;
    float confidence = 0.0f;
    uint32_t seen_at_ms = 0;
};

// Creates the Vision2 task. It starts paused until EnableYukiVision2() is called.
void StartYukiVision2();
void EnableYukiVision2();
void DisableYukiVision2();

// Returns the latest observation. Coordinates are always in the exact RGB888
// frame coordinate system passed to PedestrianDetect; no rotation/swap hacks
// are applied to the detection result.
YukiPersonObservation GetYukiPersonObservation();

// Convenience helper for reflex/Heart Engine code.
bool YukiPersonSeenRecently(uint32_t within_ms);

}  // namespace stackchan
