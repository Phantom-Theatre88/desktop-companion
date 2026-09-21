#pragma once

// WakeNet test selection.
//
// The current hardware validation word is the official ESP-SR WakeNet model
// "Computer" (wn9_computer_tts). The semantic word is also "computer"; it is
// intentionally NOT disguised as kibi.
//
// The installer writes an ignored WakeWordModelLocal.h only after the matching
// model binary has been generated and installed into the Arduino ESP-SR SDK.
// Until then semantic delivery remains gated off.
#define DESKBOT_WAKE_WORD_TARGET "computer"
#define DESKBOT_EXPECTED_WAKENET_MODEL "wn9_computer_tts"

#if __has_include("WakeWordModelLocal.h")
#include "WakeWordModelLocal.h"
#else
#define DESKBOT_WAKENET_MODEL_INSTALLED 0
#endif
