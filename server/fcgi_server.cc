/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

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

// clang-format off
// This FastCGI server implementation is designed to store statistics received from remote clients.
// By default, everything is stored in <specified folder>/alohalytics_messages file.
// If you have tons of data, it is better to use logrotate utility to archive it (see logrotate.conf).

// Validity checks for requests should be mostly done on nginx side (see nginx.conf):
// $request_method should be POST only
// $content_length should be set and greater than zero (we re-check it below anyway)
// $content_type should be set to application/alohalytics-binary-blob (except of monitoring uri)
// $http_content_encoding should be set to gzip (except of monitoring uri)

// Monitoring URI is a simple is-server-alive check. POST any content-type there and get it back without any modifications.

// This binary shoud be spawn as a FastCGI app, for example:
// $ spawn-fcgi [-n] -a 127.0.0.1 -p <port number> -P /path/to/pid.file -- ./fcgi_server /dir/to/store/received/data /monitoring/uri [/optional/path/to/log.file]
// pid.file can be used by logrotate (see logrotate.conf).
// clang-format on

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include <fcgiapp.h>
#include <fcgio.h>

#include "../src/logger.h"

#include "statistics_receiver.h"

using namespace std;

// Can be used as a basic check on the client-side if it has connected to the right server.
static const string kBodyTextForGoodServerReply = "Mahalo";
static const string kBodyTextForBadServerReply = "Hohono";

// We always reply to our clients that we have received everything they sent, even if it was a complete junk.
// The difference is only in the body of the reply.
// Custom content type is used for monitoring.
void Reply200OKWithBody(FCGX_Stream * out, const string & body, const char * content_type = "text/plain") {
  FCGX_FPrintF(out, "Status: 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n%s\n", content_type,
               body.size(), body.c_str());
}

// Global variables to correctly reopen data and log files after signals from logrotate utility.
volatile sig_atomic_t gReceivedSIGHUP = 0;
volatile sig_atomic_t gReceivedSIGUSR1 = 0;
// Redirects all cout output into a file if good log_file_path was given in constructor.
// Can always ReopenLogFile() if needed (e.g. for log rotation).
struct CoutToFileRedirector {
  char const * path;
  unique_ptr<ofstream> log_file;
  streambuf * original_cout_buf;

  CoutToFileRedirector(const char * log_file_path) : path(log_file_path), original_cout_buf(cout.rdbuf()) {
    ReopenLogFile();
  }
  void ReopenLogFile() {
    if (!path) {
      return;
    }
    log_file.reset(nullptr);
    log_file.reset(new ofstream(path, ofstream::out | ofstream::app));
    if (log_file->good()) {
      cout.rdbuf(log_file->rdbuf());
    } else {
      ALOG("ERROR: Could not open log file", path, "for writing.");
    }
  }
  // Restore original cout streambuf.
  ~CoutToFileRedirector() { cout.rdbuf(original_cout_buf); }
};

int main(int argc, char * argv[]) {

  if (argc < 3) {
    ALOG("Usage:", argv[0], "<directory to store received data> "
                            "</special/uri/for/monitoring> "
                            "[optional path to error log file]");
    ALOG("  - Monitoring URI always replies with the same body and content-type which has been received.");
    ALOG("  - Errors are logged to stdout if error log file has not been specified.");
    ALOG("  - SIGHUP reopens main data file and SIGUSR1 reopens debug log file for logrotate utility.");
    ALOG("  - SIGTERM gracefully shutdowns server daemon.");
    return -1;
  }

  const string kStorageDirectory = argv[1];
  if (!alohalytics::FileManager::IsDirectoryWritable(kStorageDirectory)) {
    ALOG("ERROR: Directory", kStorageDirectory, "is not writable.");
    return -1;
  }

  const string kMonitoringURI = argv[2];
  if (kMonitoringURI.empty() || kMonitoringURI.front() != '/') {
    ALOG("ERROR: Given monitoring URI", kMonitoringURI, "shoud start with a slash.");
    return -1;
  }

  int result = FCGX_Init();
  if (0 != result) {
    ALOG("ERROR: FCGX_Init has failed with code", result);
    return result;
  }

  FCGX_Request request;
  result = FCGX_InitRequest(&request, 0, FCGI_FAIL_ACCEPT_ON_INTR);
  if (0 != result) {
    ALOG("ERROR: FCGX_InitRequest has failed with code", result);
    return result;
  }

  // Redirect cout into a file if it was given in the command line.
  CoutToFileRedirector log_redirector(argc > 3 ? argv[3] : nullptr);
  // Correctly reopen data file on SIGHUP for logrotate.
  if (SIG_ERR == ::signal(SIGHUP, [](int) { gReceivedSIGHUP = SIGHUP; })) {
    ALOG("WARNING: Could not set SIGHUP handler. Logrotate will not work correctly.");
  }
  // Correctly reopen debug log file on SIGUSR1 for logrotate.
  if (SIG_ERR == ::signal(SIGUSR1, [](int) { gReceivedSIGUSR1 = SIGUSR1; })) {
    ALOG("WARNING: Could not set SIGUSR1 handler. Logrotate will not work correctly.");
  }
  // NOTE: On most systems, when we get a signal, FCGX_Accept_r blocks even with a FCGI_FAIL_ACCEPT_ON_INTR flag set
  // in the request. Looks like on these systems default signal function installs the signals with the SA_RESTART flag
  // set (see man sigaction for more details) and syscalls are automatically restart themselves if a signal occurs.
  // To "fix" this behavior and gracefully shutdown our server, we use a trick from
  // W. Richard Stevens, Stephen A. Rago, "Advanced Programming in the UNIX Environment", 2nd edition, page 329.
  // It is also described here: http://comments.gmane.org/gmane.comp.web.fastcgi.devel/942
  for (auto signo : {SIGTERM, SIGINT}) {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;
#endif
    act.sa_handler = [](int) { FCGX_ShutdownPending(); };
    const int result = sigaction(signo, &act, nullptr);
    if (result != 0) {
      ALOG("WARNING: Could not set", signo, "signal handler");
    }
  }

  alohalytics::StatisticsReceiver receiver(kStorageDirectory);
  string gzipped_body;
  long long content_length;
  const char * remote_addr_str = nullptr;
  const char * request_uri_str = nullptr;
  const char * user_agent_str = nullptr;
  ALOG("FastCGI Server instance is ready to serve clients' requests.");
  while (FCGX_Accept_r(&request) >= 0) {
    // Correctly reopen data file in the queue.
    if (gReceivedSIGHUP == SIGHUP) {
      receiver.ReopenDataFile();
      gReceivedSIGHUP = 0;
    }
    // Correctly reopen debug log file.
    if (gReceivedSIGUSR1 == SIGUSR1) {
      log_redirector.ReopenLogFile();
      gReceivedSIGUSR1 = 0;
    }

    try {
      remote_addr_str = FCGX_GetParam("REMOTE_ADDR", request.envp);
      if (!remote_addr_str) {
        ALOG("WARNING: Missing REMOTE_ADDR. Please check your http server configuration.");
      }
      request_uri_str = FCGX_GetParam("REQUEST_URI", request.envp);
      if (!request_uri_str) {
        ALOG("WARNING: Missing REQUEST_URI. Please check your http server configuration.");
      }
      user_agent_str = FCGX_GetParam("HTTP_USER_AGENT", request.envp);
      if (!user_agent_str) {
        ALOG("WARNING: Missing HTTP User-Agent. Please check your http server configuration.");
      }

      const char * content_length_str = FCGX_GetParam("HTTP_CONTENT_LENGTH", request.envp);
      content_length = 0;
      if (!content_length_str || ((content_length = atoll(content_length_str)) <= 0)) {
        ALOG("WARNING: Request is ignored due to invalid or missing Content-Length header", content_length_str,
              remote_addr_str, request_uri_str, user_agent_str);
        Reply200OKWithBody(request.out, kBodyTextForBadServerReply);
        continue;
      }
      // TODO(AlexZ): Should we make a better check for Content-Length or basic exception handling would be enough?
      gzipped_body.resize(content_length);
      if (fcgi_istream(request.in).read(&gzipped_body[0], content_length).fail()) {
        ALOG("WARNING: Request is ignored because it's body could not be read.", remote_addr_str, request_uri_str,
              user_agent_str);
        Reply200OKWithBody(request.out, kBodyTextForBadServerReply);
        continue;
      }

      // Handle special URI for server monitoring.
      if (request_uri_str && request_uri_str == kMonitoringURI) {
        const char * content_type_str = FCGX_GetParam("HTTP_CONTENT_TYPE", request.envp);
        // Reply with the same content and content-type.
        Reply200OKWithBody(request.out, gzipped_body, content_type_str ? content_type_str : "text/plain");
        continue;
      }

      // Process and store received body.
      // This call can throw different exceptions.
      receiver.ProcessReceivedHTTPBody(gzipped_body,
                                       AlohalyticsBaseEvent::CurrentTimestamp(),
                                       remote_addr_str ? remote_addr_str : "",
                                       user_agent_str ? user_agent_str : "",
                                       request_uri_str ? request_uri_str : "");
      Reply200OKWithBody(request.out, kBodyTextForGoodServerReply);
    } catch (const exception & ex) {
      ALOG("WARNING: Exception was thrown:", ex.what(), remote_addr_str, request_uri_str, user_agent_str);
      // TODO(AlexZ): Log "bad" received body for investigation.
      Reply200OKWithBody(request.out, kBodyTextForBadServerReply);
    }
  }
  ALOG("Shutting down FastCGI server instance.");
  return 0;
}
