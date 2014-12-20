#ifndef EVENT_BASE_H
#define EVENT_BASE_H

// Define ALOHALYTICS_SERVER when using this header on a server-side.

#include <string>
#include <chrono>

#include "cereal/include/cereal.hpp"
#include "cereal/include/types/base_class.hpp"
#include "cereal/include/types/polymorphic.hpp"

// For easier processing on a server side, every statistics event should derive from this base class.
struct AlohalyticsBaseEvent {
  uint64_t timestamp;

  static uint64_t CurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
  }

  // To avoid unnecessary calls on a server side
#ifndef ALOHALYTICS_SERVER
  AlohalyticsBaseEvent() : timestamp(CurrentTimestamp()) {}
#endif

  template <class Archive> void serialize(Archive& ar) {
    ar(CEREAL_NVP(timestamp));
  }

  // To use polymorphism on a server side.
  virtual ~AlohalyticsBaseEvent() = default;
};
CEREAL_REGISTER_TYPE(AlohalyticsBaseEvent);

// Base for all Android-specific events.
struct AlohalyticsAndroidBaseEvent : AlohalyticsBaseEvent {
  template <class Archive> void serialize(Archive& ar) {
    AlohalyticsBaseEvent::serialize(ar);
  }
};
CEREAL_REGISTER_TYPE(AlohalyticsAndroidBaseEvent);

// Base for all iOS-specific events.
struct AlohalyticsiOSBaseEvent : public AlohalyticsBaseEvent {
  template <class Archive> void serialize(Archive& ar) {
    AlohalyticsBaseEvent::serialize(ar);
  }
};
CEREAL_REGISTER_TYPE(AlohalyticsiOSBaseEvent);

// Delivers Android-specific Ids.
struct AlohalyticsAndroidIdsEvent : public AlohalyticsAndroidBaseEvent {
  std::string google_advertising_id;
  std::string android_id;
  std::string device_id;
  std::string sim_serial_number;

  template <class Archive> void serialize(Archive& ar) {
    AlohalyticsAndroidBaseEvent::serialize(ar);
    ar(CEREAL_NVP(google_advertising_id),
       CEREAL_NVP(android_id),
       CEREAL_NVP(device_id),
       CEREAL_NVP(sim_serial_number));
  }
};
CEREAL_REGISTER_TYPE(AlohalyticsAndroidIdsEvent);

// Base for all user-interface screens (modes).
struct AlohalyticsBaseModeEvent : public AlohalyticsBaseEvent {
  std::string mode_name;

  template <class Archive> void serialize(Archive& ar) {
    AlohalyticsBaseEvent::serialize(ar);
    ar(CEREAL_NVP(mode_name));
  }
};
CEREAL_REGISTER_TYPE(AlohalyticsBaseModeEvent);

// Activity.onResume() on Android.
struct AlohalyticsModeActivatedEvent : public AlohalyticsBaseModeEvent {
  template <class Archive> void serialize(Archive& ar) {
    AlohalyticsBaseModeEvent::serialize(ar);
  }
};
CEREAL_REGISTER_TYPE(AlohalyticsModeActivatedEvent);

// Activity.onPause() on Android.
struct AlohalyticsModeDeactivatedEvent : public AlohalyticsBaseModeEvent {
  template <class Archive> void serialize(Archive& ar) {
    AlohalyticsBaseModeEvent::serialize(ar);
  }
};
CEREAL_REGISTER_TYPE(AlohalyticsModeDeactivatedEvent);

// *** Events compatible with other statistic systems ***
// Simple event with a string name (key) only.
struct AlohalyticsCompatibilityKeyEvent : public AlohalyticsBaseEvent {
  std::string key;

  template <class Archive> void serialize(Archive& ar) {
    AlohalyticsBaseEvent::serialize(ar);
    ar(CEREAL_NVP(key));
  }
};
CEREAL_REGISTER_TYPE(AlohalyticsCompatibilityKeyEvent);

// Simple event with a string key/value pair.
struct AlohalyticsCompatibilityKeyValueEvent : public AlohalyticsCompatibilityKeyEvent {
  std::string value;

  template <class Archive> void serialize(Archive& ar) {
    AlohalyticsCompatibilityKeyEvent::serialize(ar);
    ar(CEREAL_NVP(value));
  }
};
CEREAL_REGISTER_TYPE(AlohalyticsCompatibilityKeyValueEvent);

// Simple event with a string key and map<string, string> value.
struct AlohalyticsCompatibilityKeyPairsEvent : public AlohalyticsCompatibilityKeyEvent {
  std::map<std::string, std::string> pairs;

  template <class Archive> void serialize(Archive& ar) {
    AlohalyticsCompatibilityKeyEvent::serialize(ar);
    ar(CEREAL_NVP(pairs));
  }
};
CEREAL_REGISTER_TYPE(AlohalyticsCompatibilityKeyPairsEvent);

#endif  // EVENT_BASE_H
