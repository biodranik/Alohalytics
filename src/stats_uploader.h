#ifndef STATS_UPLOADER_H
#define STATS_UPLOADER_H

#include <string>

#include "http_client.h"
#include "FileStorageQueue/fsq.h"

namespace aloha {

// Collects statistics and uploads it to the server.
class StatsUploader {
 public:
  explicit StatsUploader(const std::string& upload_url) : upload_url_(upload_url) {
  }
  void OnMessage(const std::string& message, size_t /*unused_dropped_events*/ = 0) const {
    // TODO(AlexZ): temporary stub.
    HTTPClientPlatformWrapper(upload_url_).set_post_body(message, "application/alohalytics-binary-blob").RunHTTPRequest();
  }
  const std::string& GetURL() const {
    return upload_url_;
  }

 private:
  const std::string upload_url_;
};

} // namespace aloha

#endif // STATS_UPLOADER_H
