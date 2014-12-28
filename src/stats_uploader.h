#ifndef STATS_UPLOADER_H
#define STATS_UPLOADER_H

#include <string>

#include "http_client.h"
#include "FileStorageQueue/fsq.h"
#include "logger.h"


namespace aloha {

// Collects statistics and uploads it to the server.
class StatsUploader {
 public:
  explicit StatsUploader(const std::string& upload_url) : upload_url_(upload_url) {
  }

  template <typename T_TIMESTAMP>
  fsq::FileProcessingResult OnFileReady(const fsq::FileInfo<T_TIMESTAMP>& file, T_TIMESTAMP /*now*/) {
    if (debug_mode_) {
      LOG("Alohalytics: trying to upload statistics file", file.full_path_name, "to", upload_url_);
    }
    HTTPClientPlatformWrapper request(upload_url_);
    request.set_post_file(file.full_path_name, "application/alohalytics-binary-blob");
    bool success = false;
    try {
      success = request.RunHTTPRequest();
    } catch (const std::exception & ex) {
      if (debug_mode_) {
        LOG("Alohalytics: exception while trying to upload file", ex.what());
      }
    }
    if (success) {
      if (debug_mode_) {
        LOG("Alohalytics: file was successfully uploaded.");
      }
      return fsq::FileProcessingResult::Success;
    } else {
      if (debug_mode_) {
        LOG("Alohalytics: error while uploading file", request.error_code());
      }
      return fsq::FileProcessingResult::FailureNeedRetry;
    }
  }

  const std::string& GetURL() const {
    return upload_url_;
  }

  void set_debug_mode(bool debug_mode) { debug_mode_ = debug_mode; }

 private:
  const std::string upload_url_;
  bool debug_mode_ = false;
};

} // namespace aloha

#endif // STATS_UPLOADER_H
