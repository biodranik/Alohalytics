#ifndef EVENT_BASE_H
#define EVENT_BASE_H

#include <string>

#include "cereal/include/cereal/cereal.hpp"
#include "cereal/include/cereal/types/base_class.hpp"
#include "cereal/include/cereal/types/polymorphic.hpp"

// For easier processing on a server side, every statistics event should derive from this base class.
struct AlohalyticsBaseEvent {
  // Stores event's timestamp in UTC milliseconds:
  // uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
  //             std::chrono::system_clock::now().time_since_epoch()).count();
  uint64_t timestamp;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(CEREAL_NVP(timestamp));
  }

  // To use polymorphism on a server side.
  virtual ~AlohalyticsBaseEvent() = default;
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsBaseEvent, "a");

// Base for all Android-specific events.
struct AlohalyticsAndroidBaseEvent : public AlohalyticsBaseEvent {
  template <class Archive>
  void serialize(Archive& ar) {
    ar(cereal::base_class<AlohalyticsBaseEvent>(this));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsAndroidBaseEvent, "ab1");

// Base for all iOS-specific events.
struct AlohalyticsiOSBaseEvent : public AlohalyticsBaseEvent {
  template <class Archive>
  void serialize(Archive& ar) {
    ar(cereal::base_class<AlohalyticsBaseEvent>(this));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsiOSBaseEvent, "ab2");

// Delivers Android-specific Ids.
struct AlohalyticsAndroidIdsEvent : public AlohalyticsAndroidBaseEvent {
  std::string google_advertising_id;
  std::string android_id;
  std::string device_id;
  std::string sim_serial_number;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(cereal::base_class<AlohalyticsAndroidBaseEvent>(this),
        CEREAL_NVP(google_advertising_id),
        CEREAL_NVP(android_id),
        CEREAL_NVP(device_id),
        CEREAL_NVP(sim_serial_number));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsAndroidIdsEvent, "ab1c1");

// Activity.onResume() on Android.
struct AlohalyticsModeActivatedEvent : public AlohalyticsBaseEvent {
  std::string mode_name;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(cereal::base_class<AlohalyticsBaseEvent>(this), CEREAL_NVP(mode_name));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsModeActivatedEvent, "ab3");

// Activity.onPause() on Android.
struct AlohalyticsModeDeactivatedEvent : public AlohalyticsBaseEvent {
  std::string mode_name;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(cereal::base_class<AlohalyticsBaseEvent>(this), CEREAL_NVP(mode_name));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsModeDeactivatedEvent, "ab4");

// *** Events compatible with other statistic systems ***
// Simple event with a string name (key) only.
struct AlohalyticsCompatibilityKeyEvent : public AlohalyticsBaseEvent {
  std::string key;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(cereal::base_class<AlohalyticsBaseEvent>(this), CEREAL_NVP(key));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsCompatibilityKeyEvent, "ab5");

// Simple event with a string key/value pair.
struct AlohalyticsCompatibilityKeyValueEvent : public AlohalyticsCompatibilityKeyEvent {
  std::string value;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(cereal::base_class<AlohalyticsCompatibilityKeyEvent>(this), CEREAL_NVP(value));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsCompatibilityKeyValueEvent, "ab5c1");

// Simple event with a string key and map<string, string> value.
struct AlohalyticsCompatibilityKeyPairsEvent : public AlohalyticsCompatibilityKeyEvent {
  std::map<std::string, std::string> pairs;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(cereal::base_class<AlohalyticsCompatibilityKeyEvent>(this), CEREAL_NVP(pairs));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsCompatibilityKeyPairsEvent, "ab5c2");

#endif  // EVENT_BASE_H
