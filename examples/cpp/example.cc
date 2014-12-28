// Core statistics engine.
#include "../../src/aloha_stats.h"
// dflags is optional and is used here just for command line parameters parsing.
#include "dflags.h"

#include <iostream>
#include <thread>
#include <chrono>

DEFINE_string(server_url, "http://localhost:8080/", "Statistics server url.");
DEFINE_string(event, "TestEvent", "Records given event.");
DEFINE_string(values,
              "",
              "Records event with single value (--values singleValue) or value pairs (--values "
              "key1=value1,key2=value2).");
DEFINE_string(storage,
              "build/",
              "Path to directory (with a slash at the end) to temporarily store recorded events.");
DEFINE_bool(debug, true, "Enables debug mode for statistics engine.");
DEFINE_bool(upload, false, "If true, triggers event to forcebly upload all statistics to the server.");
DEFINE_double(sleep, 2.5, "The number of seconds to sleep before terminating.");

using namespace std;

int main(int argc, char** argv) {
  ParseDFlags(&argc, &argv);

  aloha::Stats stats(FLAGS_server_url, FLAGS_storage, "Some Unique ID here");
  if (FLAGS_debug) {
    stats.DebugMode(true);
  }

  if (!FLAGS_event.empty()) {
    if (!FLAGS_values.empty()) {
      std::string values = FLAGS_values;
      for (auto& c : values) {
        if (c == '=' || c == ',') {
          c = ' ';
        }
      }
      std::string key;
      aloha::TStringMap map;
      std::istringstream is(values);
      std::string it;
      while (is >> it) {
        if (key.empty()) {
          key = it;
          map[key] = "";
        } else {
          map[key] = it;
          key.clear();
        }
      }
      if (map.size() == 1 && map.begin()->second == "") {
        // Event with one value.
        stats.LogEvent(FLAGS_event, map.begin()->first);
      } else {
        // Event with many key=value pairs.
        stats.LogEvent(FLAGS_event, map);
      }
    } else {
      // Simple event.
      stats.LogEvent(FLAGS_event);
    }
  }

  if (FLAGS_upload) {
    stats.Upload();
  }

  this_thread::sleep_for(chrono::milliseconds(static_cast<uint32_t>(1e3 * FLAGS_sleep)));

  return 0;
}
