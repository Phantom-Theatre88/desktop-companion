#include "HeartPersistence.h"

#include <Preferences.h>

#include "HeartEngine.h"

namespace deskbot {
namespace ghost {
namespace {

constexpr const char* kNamespace = "desk-heart";
constexpr const char* kInitializedKey = "initialized";
constexpr const char* kMoodKey = "mood";
constexpr const char* kAffectionKey = "affection";
constexpr const char* kCuriosityKey = "curiosity";
constexpr const char* kBoredomKey = "boredom";
constexpr const char* kSleepinessKey = "sleepiness";
constexpr const char* kAttentionKey = "attention";

bool openPreferences(Preferences& preferences, bool read_only) {
  return preferences.begin(kNamespace, read_only);
}

}  // namespace

bool HeartPersistence::hasPrimarySnapshot() {
  Preferences preferences;
  if (!openPreferences(preferences, true)) {
    return false;
  }

  const bool initialized = preferences.getBool(kInitializedKey, false);
  preferences.end();
  return initialized;
}

bool HeartPersistence::loadPrimary(HeartState& out_state) {
  Preferences preferences;
  if (!openPreferences(preferences, true)) {
    return false;
  }

  if (!preferences.getBool(kInitializedKey, false)) {
    preferences.end();
    return false;
  }

  out_state.mood = preferences.getFloat(kMoodKey, out_state.mood);
  out_state.affection = preferences.getFloat(kAffectionKey, out_state.affection);
  out_state.curiosity = preferences.getFloat(kCuriosityKey, out_state.curiosity);
  out_state.boredom = preferences.getFloat(kBoredomKey, out_state.boredom);
  out_state.sleepiness = preferences.getFloat(kSleepinessKey, out_state.sleepiness);
  out_state.attention = preferences.getFloat(kAttentionKey, out_state.attention);

  preferences.end();
  return true;
}

bool HeartPersistence::savePrimary(const HeartState& state) {
  Preferences preferences;
  if (!openPreferences(preferences, false)) {
    return false;
  }

  bool ok = true;
  ok = preferences.putFloat(kMoodKey, state.mood) == sizeof(float) && ok;
  ok = preferences.putFloat(kAffectionKey, state.affection) == sizeof(float) && ok;
  ok = preferences.putFloat(kCuriosityKey, state.curiosity) == sizeof(float) && ok;
  ok = preferences.putFloat(kBoredomKey, state.boredom) == sizeof(float) && ok;
  ok = preferences.putFloat(kSleepinessKey, state.sleepiness) == sizeof(float) && ok;
  ok = preferences.putFloat(kAttentionKey, state.attention) == sizeof(float) && ok;
  ok = preferences.putBool(kInitializedKey, true) == sizeof(uint8_t) && ok;

  preferences.end();
  return ok;
}

}  // namespace ghost
}  // namespace deskbot
