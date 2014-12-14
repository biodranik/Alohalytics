#include "http_client.h"

#include <stdio.h> // popen
#include <fstream>

// Used as a test stub for basic HTTP client implementation.

namespace aloha {

std::string RunCurl(const std::string& cmd) {
  //std::cout << "cmdline: " << cmd << std::endl;
  FILE* pipe = ::popen(cmd.c_str(), "r");
  assert(pipe);
  char s[8 * 1024];
  ::fgets(s, sizeof(s)/sizeof(s[0]), pipe);
  const int err = ::pclose(pipe);
  if (err) {
    std::cerr << "Error " << err << " while calling " << cmd << std::endl;
  }
  return s;
}

// Not fully implemented.
bool HTTPClientPlatformWrapper::RunHTTPRequest() {

  // Last 3 chars in server's response will be http status code
  std::string cmd = "curl -s -w '%{http_code}' ";
  if (!content_type_.empty()) {
    cmd += "-H 'Content-Type: application/json' ";
  }
  if (!post_body_.empty()) {
    // POST body through tmp file to avoid breaking command line.
    char tmp_file[L_tmpnam];
    ::tmpnam(tmp_file);
    std::ofstream(tmp_file) << post_body_;
    post_file_ = tmp_file;
  }
  if (!post_file_.empty()) {
    cmd += "--data-binary @" + post_file_ + " ";
  }

  cmd += url_requested_;
  server_response_ = RunCurl(cmd);

  // Clean up tmp file if any was created.
  if (!post_body_.empty() && !post_file_.empty()) {
    ::remove(post_file_.c_str());
  }

  // Extract http status code from the last response line.
  error_code_ = std::stoi(server_response_.substr(server_response_.size() - 3));
  server_response_.resize(server_response_.size() - 4);

  if (!received_file_.empty()) {
    std::ofstream(received_file_) << server_response_;
  }

  return error_code_ == 200;
}

} // namespace aloha
