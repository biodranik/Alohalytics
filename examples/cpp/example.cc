// Core statistics engine.
#include "../../src/aloha_stats.h"
// dflags is optional and is used here just for command line parameters parsing.
#include "dflags.h"

#include <iostream>
#include <regex>

DEFINE_string(server_url, "http://localhost:8080/", "Statistics server url.");
DEFINE_string(event, "TestEvent", "Records given event.");
DEFINE_string(
    values,
    "",
    "Records event with single value (--values singleValue) or value pairs (--values key1=value1,key2=value2).");
DEFINE_string(storage,
              "build/",
              "Path to directory (with a slash at the end) to temporarily store recorded events.");
DEFINE_bool(debug, true, "Enables debug mode for statistics engine.");
DEFINE_bool(upload, false, "If true, triggers event to forcebly upload all statistics to the server.");

using namespace std;

int main(int argc, char** argv) {
  ParseDFlags(&argc, &argv);

  aloha::Stats stats(FLAGS_server_url, FLAGS_storage);
  if (FLAGS_debug) {
    stats.DebugMode(true);
  }

  if (!FLAGS_event.empty()) {
    if (!FLAGS_values.empty()) {
      aloha::TStringMap map;
      const std::regex re("[=,]+");
      std::string key;
      for (auto it = std::sregex_token_iterator(FLAGS_values.begin(), FLAGS_values.end(), re, -1);
           it != std::sregex_token_iterator();
           ++it) {
        if (key.empty()) {
          key = *it;
          map[key] = "";
        } else {
          map[key] = *it;
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

  return 0;
}
