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

#include "../http_client.h"
#include "../file_manager.h"
#include "../logger.h"

#include <array>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>   // std::cerr
#include <stdexcept>  // std::runtime_error
#include <assert.h>   // assert
#include <stdio.h>    // popen

#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>  // close
#endif

// Used as a test stub for basic HTTP client implementation.
// Make sure that you have curl installed in the PATH.
// TODO(AlexZ): Not a production-ready implementation.
namespace alohalytics {

std::string RunCurl(const std::string & cmd) {
  FILE * pipe = ::popen(cmd.c_str(), "r");
  assert(pipe);
  std::array<char, 8 * 1024> arr;
  std::string result;
  size_t read;
  do {
    read = ::fread(arr.data(), 1, arr.size(), pipe);
    if (read > 0) {
      result.append(arr.data(), read);
    }
  } while (read == arr.size());
  const int err = ::pclose(pipe);
  if (err) {
    throw std::runtime_error("Error " + std::to_string(err) + " while calling " + cmd);
  }
  return result;
}

// TODO(AlexZ): Move this function to File Manager.
std::string GetTmpFileName() {
#ifdef _MSC_VER
  char tmp_file[L_tmpnam];
  const errno_t err = ::tmpnam_s(tmp_file, L_tmpnam);
  if (err != 0) {
    throw std::runtime_error("Error " + std::to_string(err) + ", failed to create temporary file.");
  }
#else
  char tmp_file[] = "/tmp/alohalyticstmp-XXXXXX";
  if (nullptr == ::mktemp(tmp_file)) {  // tmpnam is deprecated and insecure.
    throw std::runtime_error("Error: failed to create temporary file.");
  }
#endif
  return tmp_file;
}

typedef std::vector<std::pair<std::string, std::string>> HeadersT;

HeadersT ParseHeaders(const std::string & raw) {
  std::istringstream stream(raw);
  HeadersT headers;
  std::string line;
  while (std::getline(stream, line)) {
    const auto cr = line.rfind('\r');
    if (cr != std::string::npos) {
      line.erase(cr);
    }
    const auto delims = line.find(": ");
    if (delims != std::string::npos) {
      headers.push_back(std::make_pair(line.substr(0, delims), line.substr(delims + 2)));
    }
  }
  return headers;
}

// Extract HTTP headers via temporary file with -D switch.
// HTTP status code is extracted from curl output (-w switches).
// Redirects are handled recursively. TODO(AlexZ): avoid infinite redirects loop.
bool HTTPClientPlatformWrapper::RunHTTPRequest() {
  const alohalytics::ScopedRemoveFile headers_deleter(GetTmpFileName());
  std::string cmd = "curl -s -w '%{http_code}' -X " + http_method_ + " -D '" + headers_deleter.file + "' ";

  if (!content_type_.empty()) {
    cmd += "-H 'Content-Type: " + content_type_ + "' ";
  }
  if (!content_encoding_.empty()) {
    cmd += "-H 'Content-Encoding: " + content_encoding_ + "' ";
  }
  if (!basic_auth_user_.empty()) {
    cmd += "-u '" + basic_auth_user_ + ":" + basic_auth_password_ + "' ";
  }
  if (!cookies_.empty()) {
    cmd += "-b '" + cookies_ + "' ";
  }

  alohalytics::ScopedRemoveFile body_deleter;
  if (!body_data_.empty()) {
    body_deleter.file = GetTmpFileName();
    // POST body through tmp file to avoid breaking command line.
    if (!(std::ofstream(body_deleter.file) << body_data_).good()) {
      std::cerr << "Error: failed to write into a temporary file." << std::endl;
      return false;
    }
    // TODO(AlexZ): Correctly clean up this internal var to avoid client confusion.
    body_file_ = body_deleter.file;
  }
  if (!body_file_.empty()) {
    // Content-Length is added automatically by curl.
    cmd += "--data-binary '@" + body_file_ + "' ";
  }

  // Use temporary file to receive data from server.
  // If user has specified file name to save data, it is not temporary and is not deleted automatically.
  alohalytics::ScopedRemoveFile received_file_deleter;
  std::string rfile = received_file_;
  if (rfile.empty()) {
    rfile = GetTmpFileName();
    received_file_deleter.file = rfile;
  }
  cmd += "-o " + rfile + " ";

  cmd += "'" + url_requested_ + "'";
  try {
    if (debug_mode_) {
      ALOG("Executing", cmd);
    }
    error_code_ = std::stoi(RunCurl(cmd));
    const HeadersT headers = ParseHeaders(alohalytics::FileManager::ReadFileAsString(headers_deleter.file));
    for (const auto & header : headers) {
      if (header.first == "Set-Cookie") {
        server_cookies_ += header.second + ", ";
      } else if (header.first == "Content-Type") {
        content_type_received_ = header.second;
      } else if (header.first == "Content-Encoding") {
        content_encoding_received_ = header.second;
      } else if (header.first == "Location") {
        url_received_ = header.second;
      }
    }
    server_cookies_ = normalize_server_cookies(std::move(server_cookies_));

    if (url_received_.empty()) {
      url_received_ = url_requested_;
      // Load body contents in final request only (skip redirects).
      if (received_file_.empty()) {
        // Sometimes server can reply with empty body, and it's ok.
        try {
          server_response_ = alohalytics::FileManager::ReadFileAsString(rfile);
        } catch (const std::exception &) {
        }
      }
    } else {
      // Handle HTTP redirect.
      // TODO(AlexZ): Should we check HTTP redirect code here?
      if (debug_mode_) {
        ALOG("HTTP redirect", error_code_, "to", url_received_);
      }
      HTTPClientPlatformWrapper redirect(url_received_);
      redirect.set_cookies(combined_cookies());
      const bool success = redirect.RunHTTPRequest();
      if (success) {
        error_code_ = redirect.error_code();
        url_received_ = redirect.url_received();
        server_cookies_ = move(redirect.server_cookies_);
        server_response_ = move(redirect.server_response_);
        content_type_received_ = move(redirect.content_type_received_);
        content_encoding_received_ = move(redirect.content_encoding_received_);
      } else {
        error_code_ = -1;
      }
      return success;
    }
  } catch (const std::exception & ex) {
    std::cerr << "Exception " << ex.what() << std::endl;
    return false;
  }
  return true;
}

}  // namespace alohalytics
