#ifndef ALOHA_STATS_H
#define ALOHA_STATS_H

#include <string>
#include <map>
#include <thread>
#include <iostream>

#include "http_client.h"
#include "logger.h"

namespace aloha {

typedef std::map<std::string, std::string> TStringMap;

class Stats {
  std::string statistics_server_url_;
  std::string storage_path_;
  bool debug_mode_ = false;

 public:
  Stats(std::string const& statistics_server_url, std::string const& storage_path_with_a_slash_at_the_end)
      : statistics_server_url_(statistics_server_url), storage_path_(storage_path_with_a_slash_at_the_end) {
  }

  void LogEvent(std::string const& event_name) const {
    if (debug_mode_) {
      LOG("LogEvent:", event_name);
    }
    // TODO(dkorolev): Insert real message queue + cereal here.
    PushMessageViaQueue(event_name);
  }

  void LogEvent(std::string const& event_name, std::string const& event_value) const {
    if (debug_mode_) {
      LOG("LogEvent:", event_name, "=", event_value);
    }
    // TODO(dkorolev): Insert real message queue + cereal here.
    PushMessageViaQueue(event_name + "=" + event_value);
  }

  void LogEvent(std::string const& event_name, TStringMap const& value_pairs) const {
    // TODO(dkorolev): Insert real message queue + cereal here.
    std::string merged = event_name + "{";
    for (const auto& it : value_pairs) {
      merged += (it.first + "=" + it.second + ",");
    }
    merged.back() = '}';
    PushMessageViaQueue(merged);
    if (debug_mode_) {
      LOG("LogEvent:", event_name, "=", value_pairs);
    }
  }

  void DebugMode(bool enable) {
    debug_mode_ = enable;
    if (enable) {
      LOG("Alohalytics: Enabled debug mode.");
      LOG("Alohalytics: Server url:", statistics_server_url_);
      LOG("Alohalytics: Storage path:", storage_path_);
    }
  }

  // Forcedly tries to upload all stored records to the server.
  void Upload() {
    if (debug_mode_) {
      LOG("Alohalytics: Uploading data to", statistics_server_url_);
    }
    // TODO
  }

  // TODO(dkorolev): Was needed for unit testing.
  int UniversalDebugAnswer() const {
    return 42;
  }

 private:
  void PushMessageViaQueue(const std::string& message) const {
    std::thread(&SimpleSampleHttpPost, statistics_server_url_, message).detach();
  }

  // TODO temporary stub function
  static void SimpleSampleHttpPost(const std::string& url, const std::string& post_data) {
    HTTPClientPlatformWrapper(url).set_post_body(post_data, "text/plain").RunHTTPRequest();
  }
};

}  // namespace aloha

#endif  // #ifndef ALOHA_STATS_H
