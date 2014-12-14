#ifndef ALOHA_STATS_H
#define ALOHA_STATS_H

#include <string>
#include <thread>
#include <iostream>

#include "http_client.h"
#include "logger.h"

namespace aloha {

class Stats {
  std::string statistics_server_url_;
  std::string storage_path_;
  bool debug_mode_ = false;

 public:
  Stats(std::string const& statistics_server_url, std::string const& storage_path_with_a_slash_at_the_end)
      : statistics_server_url_(statistics_server_url), storage_path_(storage_path_with_a_slash_at_the_end) {
  }

  bool LogEvent(std::string const& event_name) const {
    LOG("LogEvent:",  event_name);
    // TODO(dkorolev): Insert real message queue + cereal here.
    std::thread(&SimpleSampleHttpPost, statistics_server_url_, event_name).detach();
    return true;
  }

  bool LogEvent(std::string const& event_name, std::string const& event_value) const {
    LOG("LogEvent:", event_name, "with value:", event_value);
    // TODO(dkorolev): Insert real message queue + cereal here.
    std::thread(&SimpleSampleHttpPost, statistics_server_url_, event_name + "=" + event_value).detach();
    return true;
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
    // TODO
  }

 private:
  // TODO temporary stub function
  static void SimpleSampleHttpPost(const std::string& url, const std::string& post_data) {
    HTTPClientPlatformWrapper(url).set_post_body(post_data, "text/plain").RunHTTPRequest();
    HTTPClientPlatformWrapper(url).set_post_body(post_data + post_data, "text/plain").RunHTTPRequest();
  }
};

}  // namespace aloha

#endif  // #ifndef ALOHA_STATS_H
