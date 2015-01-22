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

#include "alohalytics.h"
#include "http_client.h"
#include "logger.h"
#include "message_queue.h"
#include "event_base.h"
#include "FileStorageQueue/fsq.h"

#include "cereal/include/archives/binary.hpp"
#include "cereal/include/types/string.hpp"
#include "cereal/include/types/map.hpp"

#include <list>
#include <string>
#include <map>

namespace alohalytics {

typedef std::map<std::string, std::string> TStringMap;

// Used for cereal smart-pointers polymorphic serialization.
struct NoOpDeleter {
  template <typename T>
  void operator()(T*) {}
};

class Stats final {
  std::string upload_url_;
  // Stores already serialized and ready-to-append event with unique client id.
  // NOTE: Statistics will not be uploaded if unique client id was not set.
  std::string unique_client_id_event_;
  MessageQueue<Stats> message_queue_;
  typedef fsq::FSQ<fsq::Config<Stats>> TFileStorageQueue;
  // TODO(AlexZ): Refactor storage queue so it can store messages in memory if no file directory was set.
  std::unique_ptr<TFileStorageQueue> file_storage_queue_;
  // Used to store events if no storage path was set.
  // Flushes all data to file storage and is not used any more if storage path was set.
  typedef std::list<std::string> TMemoryContainer;
  TMemoryContainer memory_storage_;
  bool debug_mode_ = false;

  // Use alohalytics::Stats::Instance() to access statistics engine.
  Stats() : message_queue_(*this) {}

  static bool UploadBuffer(const std::string& url, std::string&& buffer, bool debug_mode) {
    HTTPClientPlatformWrapper request(url);
    request.set_debug_mode(debug_mode);
    request.set_post_body(std::move(buffer), "application/alohalytics-binary-blob");

    try {
      if (request.RunHTTPRequest()) {
        if (debug_mode) {
          ALOG(buffer.size(), "bytes were successfully uploaded");
        }
        return true;
      } else {
        if (debug_mode) {
          ALOG("HTTP error", request.error_code(), "while uploading.");
        }
      }
    } catch (const std::exception& ex) {
      if (debug_mode) {
        ALOG("Exception while trying to upload file", ex.what());
      }
    }
    return false;
  }

  // Logs if debug_mode is enabled.
  template <typename... ARGS>
  void DebugLog(ARGS... args) const {
    if (debug_mode_) {
      alohalytics::Logger().Log(args...);
    }
  }

 public:
  // Processes messages passed from UI in message queue's own thread.
  // TODO(AlexZ): Refactor message queue to make this method private.
  void OnMessage(const std::string& message, size_t dropped_events) {
    if (dropped_events) {
      DebugLog("Warning:", dropped_events, "were dropped from the queue.");
    }
    if (file_storage_queue_) {
      file_storage_queue_->PushMessage(message);
    } else {
      auto& container = memory_storage_;
      container.push_back(message);
      static const size_t kMaxEventsInMemory = 2048;
      if (container.size() > kMaxEventsInMemory) {
        container.pop_front();
      }
    }
  }

  // Called by file storage engine to upload file with collected data.
  // Should return true if upload has been successful.
  // TODO(AlexZ): Refactor FSQ to make this method private.
  bool OnFileReady(const std::string& full_path_to_file, uint64_t file_size) {
    if (upload_url_.empty()) {
      DebugLog("Warning: upload server url was not set and file of", file_size, "bytes was not uploaded.");
      return false;
    }
    if (unique_client_id_event_.empty()) {
      DebugLog("Warning: unique client ID is not set so statistics was not uploaded.");
      return false;
    }

    DebugLog("Trying to upload statistics file", full_path_to_file, "to", upload_url_);

    // Append unique installation id in the beginning of each file sent to the server.
    // It can be empty so all stats data will become anonymous.
    // TODO(AlexZ): Refactor fsq to silently add it in the beginning of each file.
    std::ifstream fi(full_path_to_file, std::ifstream::in | std::ifstream::binary);
    std::string buffer(unique_client_id_event_);
    const size_t id_size = unique_client_id_event_.size();
    buffer.resize(id_size + file_size);
    fi.read(&buffer[id_size], file_size);
    if (fi.good()) {
      fi.close();
      return UploadBuffer(upload_url_, std::move(buffer), debug_mode_);
    } else {
      DebugLog("Can't load file with size", file_size, "into memory");
    }
    return false;
  }

  static Stats& Instance() {
    static Stats alohalytics;
    return alohalytics;
  }

  // Easier integration if enabled.
  Stats& SetDebugMode(bool enable) {
    debug_mode_ = enable;
    DebugLog("Enabled debug mode.");
    return *this;
  }

  // If not set, collected data will never be uploaded.
  Stats& SetServerUrl(const std::string& url_to_upload_statistics_to) {
    upload_url_ = url_to_upload_statistics_to;
    DebugLog("Set upload url:", url_to_upload_statistics_to);
    return *this;
  }

  // If not set, data will be stored in memory only.
  Stats& SetStoragePath(const std::string& full_path_to_storage_with_a_slash_at_the_end) {
    DebugLog("Set storage path:", full_path_to_storage_with_a_slash_at_the_end);
    auto& fsq = file_storage_queue_;
    fsq.reset(nullptr);
    if (!full_path_to_storage_with_a_slash_at_the_end.empty()) {
      fsq.reset(new TFileStorageQueue(*this, full_path_to_storage_with_a_slash_at_the_end));
      if (debug_mode_) {
        const TFileStorageQueue::Status status = fsq->GetQueueStatus();
        DebugLog("Active file size:", status.appended_file_size);
        const size_t count = status.finalized.queue.size();
        if (count) {
          DebugLog(
              count, "files with total size of", status.finalized.total_size, "bytes are waiting for upload.");
        }
      }
      const size_t memory_events_count = memory_storage_.size();
      if (memory_events_count) {
        DebugLog("Save", memory_events_count, "in-memory events into the file storage.");
        for (const auto& msg : memory_storage_) {
          fsq->PushMessage(msg);
        }
        memory_storage_.clear();
      }
    }
    return *this;
  }

  // If not set, data will be uploaded without any unique id.
  Stats& SetClientId(const std::string& unique_client_id) {
    DebugLog("Set unique client id:", unique_client_id);
    if (unique_client_id.empty()) {
      unique_client_id_event_.clear();
    } else {
      AlohalyticsIdEvent event;
      event.id = unique_client_id;
      std::ostringstream sstream;
      { cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event); }
      unique_client_id_event_ = sstream.str();
    }
    return *this;
  }

  void LogEvent(std::string const& event_name) {
    DebugLog("LogEvent:", event_name);
    AlohalyticsKeyEvent event;
    event.key = event_name;
    std::ostringstream sstream;
    {
      // unique_ptr is used to correctly serialize polymorphic types.
      cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event);
    }
    message_queue_.PushMessage(std::move(sstream.str()));
  }

  void LogEvent(std::string const& event_name, std::string const& event_value) {
    DebugLog("LogEvent:", event_name, "=", event_value);
    AlohalyticsKeyValueEvent event;
    event.key = event_name;
    event.value = event_value;
    std::ostringstream sstream;
    { cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event); }
    message_queue_.PushMessage(std::move(sstream.str()));
  }

  void LogEvent(std::string const& event_name, TStringMap const& value_pairs) {
    DebugLog("LogEvent:", event_name, "=", value_pairs);
    AlohalyticsKeyPairsEvent event;
    event.key = event_name;
    event.pairs = value_pairs;
    std::ostringstream sstream;
    { cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event); }
    message_queue_.PushMessage(std::move(sstream.str()));
  }

  // Forcedly tries to upload all stored data to the server.
  void Upload() {
    if (upload_url_.empty()) {
      DebugLog("Warning: upload server url is not set, nothing was uploaded.");
      return;
    }
    if (unique_client_id_event_.empty()) {
      DebugLog("Warning: unique client ID is not set so statistics was not uploaded.");
      return;
    }
    if (file_storage_queue_) {
      file_storage_queue_->ForceProcessing();
    } else {
      std::string buffer = unique_client_id_event_;
      // TODO(AlexZ): thread-safety?
      TMemoryContainer copy;
      copy.swap(memory_storage_);
      for (const auto& evt : copy) {
        buffer.append(evt);
      }
      if (!UploadBuffer(upload_url_, std::move(buffer), debug_mode_)) {
        // If failed, merge events we tried to upload with possible new ones.
        memory_storage_.splice(memory_storage_.end(), std::move(copy));
      }
    }
  }
};

inline void LogEvent(std::string const& event_name) {
  Stats::Instance().LogEvent(event_name);
}

inline void LogEvent(std::string const& event_name, std::string const& event_value) {
  Stats::Instance().LogEvent(event_name, event_value);
}

inline void LogEvent(std::string const& event_name, TStringMap const& value_pairs) {
  Stats::Instance().LogEvent(event_name, value_pairs);
}

}  // namespace alohalytics

#endif  // #ifndef ALOHALYTICS_H
