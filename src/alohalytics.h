/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

#ifndef ALOHALYTICS_H
#define ALOHALYTICS_H

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
  template <typename T>
  void operator()(T*) {}
};

class Stats final {
  StatsUploader uploader_;
  friend class MessageQueue<Stats>;
  mutable MessageQueue<Stats> message_queue_;
  fsq::FSQ<fsq::Config<StatsUploader>> file_storage_queue_;
  bool debug_mode_ = false;

  // Process messages passed from UI to message queue's own thread.
  void OnMessage(const std::string& message, size_t /*unused_dropped_events*/ = 0) {
    file_storage_queue_.PushMessage(message);
  }

  // Used to mark each file sent to the server.
  static std::string InstallationIdEvent(const std::string& installation_id) {
    AlohalyticsIdEvent event;
    event.id = installation_id;
    std::ostringstream sstream;
    { cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event); }
    return sstream.str();
  }

 public:
  // @param[in] statistics_server_url where we should send statistics.
  // @param[in] storage_path_with_a_slash_at_the_end where we aggregate our events.
  // @param[in] installation_id some id unique for this app installation.
  Stats(const std::string& statistics_server_url,
        const std::string& storage_path_with_a_slash_at_the_end,
        const std::string& installation_id)
      : uploader_(statistics_server_url, InstallationIdEvent(installation_id)),
        message_queue_(*this),
        file_storage_queue_(uploader_, storage_path_with_a_slash_at_the_end) {}

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
    message_queue_.PushMessage(sstream.str());
  }

  void LogEvent(std::string const& event_name, std::string const& event_value) const {
    if (debug_mode_) {
      LOG("LogEvent:", event_name, "=", event_value);
    }
    AlohalyticsKeyValueEvent event;
    event.key = event_name;
    event.value = event_value;
    std::ostringstream sstream;
    { cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event); }
    message_queue_.PushMessage(sstream.str());
  }

  void LogEvent(std::string const& event_name, TStringMap const& value_pairs) const {
    if (debug_mode_) {
      LOG("LogEvent:", event_name, "=", value_pairs);
    }
    AlohalyticsKeyPairsEvent event;
    event.key = event_name;
    event.pairs = value_pairs;
    std::ostringstream sstream;
    { cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event); }
    message_queue_.PushMessage(sstream.str());
  }

  void DebugMode(bool enable) {
    debug_mode_ = enable;
    uploader_.set_debug_mode(debug_mode_);
    if (enable) {
      LOG("Alohalytics: Enabled debug mode.");
      LOG("Alohalytics: Server url:", uploader_.GetURL());
      LOG("Alohalytics: Storage path:", file_storage_queue_.WorkingDirectory());
    }
  }

  // Forcedly tries to upload all stored records to the server.
  void Upload() {
    if (debug_mode_) {
      LOG("Alohalytics: Uploading data to", uploader_.GetURL());
    }
    file_storage_queue_.ForceProcessing();
  }
};

}  // namespace aloha

#endif  // #ifndef ALOHALYTICS_H
