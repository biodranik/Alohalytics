#ifndef ALOHA_STATS_H
#define ALOHA_STATS_H

// Used to avoid cereal compilation issues on iOS/MacOS when check() macro is defined.
#ifdef __APPLE__
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif

#include <string>
#include <map>

#include "logger.h"
#include "message_queue.h"
#include "event_base.h"
#include "stats_uploader.h"
#include "FileStorageQueue/fsq.h"

#include "cereal/include/archives/binary.hpp"
#include "cereal/include/types/string.hpp"
#include "cereal/include/types/map.hpp"

namespace aloha {

typedef std::map<std::string, std::string> TStringMap;

// Used for cereal smart-pointers polymorphic serialization.
struct NoOpDeleter {
template <typename T> void operator()(T *) {}
};

class Stats {
  StatsUploader uploader_;
  friend MessageQueue<Stats>;
  mutable MessageQueue<Stats> message_queue_;
  fsq::FSQ<fsq::Config<StatsUploader>> file_storage_queue_;
  // TODO: Use this id for every file sent to identify received files on the server.
  const std::string installation_id_;
  bool debug_mode_ = false;

  // Process messages passed from UI to message queue's own thread.
  void OnMessage(const std::string& message, size_t /*unused_dropped_events*/ = 0) {
    file_storage_queue_.PushMessage(message);
  }

 public:
  // @param[in] statistics_server_url where we should send statistics.
  // @param[in] storage_path_with_a_slash_at_the_end where we aggregate our events.
  // @param[in] installation_id some id unique for this app installation.
  Stats(const std::string& statistics_server_url,
        const std::string& storage_path_with_a_slash_at_the_end,
        const std::string& installation_id)
      : uploader_(statistics_server_url),
        message_queue_(*this),
        file_storage_queue_(uploader_, storage_path_with_a_slash_at_the_end),
        installation_id_(installation_id) {
  }

  void LogEvent(std::string const& event_name) const {
    if (debug_mode_) {
      LOG("LogEvent:", event_name);
    }
    AlohalyticsKeyEvent event;
    event.key = event_name;
    std::ostringstream sstream;
    {
      // unique_ptr is used to correctly serialize polymorphic types.
      cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event);
    }
    PushMessageViaQueue(sstream.str());
  }

  void LogEvent(std::string const& event_name, std::string const& event_value) const {
    if (debug_mode_) {
      LOG("LogEvent:", event_name, "=", event_value);
    }
    AlohalyticsKeyValueEvent event;
    event.key = event_name;
    event.value = event_value;
    std::ostringstream sstream;
    {
      cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event);
    }
    PushMessageViaQueue(sstream.str());
  }

  void LogEvent(std::string const& event_name, TStringMap const& value_pairs) const {
    if (debug_mode_) {
      LOG("LogEvent:", event_name, "=", value_pairs);
    }
    AlohalyticsKeyPairsEvent event;
    event.key = event_name;
    event.pairs = value_pairs;
    std::ostringstream sstream;
    {
      cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event);
    }
    PushMessageViaQueue(sstream.str());
  }

  void DebugMode(bool enable) {
    debug_mode_ = enable;
    uploader_.set_debug_mode(debug_mode_);
    if (enable) {
      LOG("Alohalytics: Enabled debug mode.");
      LOG("Alohalytics: Server url:", uploader_.GetURL());
      LOG("Alohalytics: Storage path:", file_storage_queue_.WorkingDirectory());
      LOG("Alohalytics: Installation ID:", installation_id_);
    }
  }

  // Forcedly tries to upload all stored records to the server.
  void Upload() {
    if (debug_mode_) {
      LOG("Alohalytics: Uploading data to", uploader_.GetURL());
    }
    file_storage_queue_.ForceProcessing();
  }

 private:
  void PushMessageViaQueue(std::string&& message) const {
    // Asynchronous call, returns immediately.
    message_queue_.PushMessage(std::move(message));
  }
};

}  // namespace aloha

#endif  // #ifndef ALOHA_STATS_H
