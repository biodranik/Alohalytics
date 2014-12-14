// Core statistics engine.
#include "../../src/aloha_stats.h"
// dflags is optional and is used here just for command line parameters parsing.
#include "dflags.h"

#include <iostream>
#include <regex>

// Just for command line parsing.
using std::regex;
using std::sregex_token_iterator;

using namespace aloha;

DEFINE_string(url, "http://httpbin.org/post", "Statistics server url.");
DEFINE_string(event, "", "Records given event.");
DEFINE_string(value, "", "Records event with specified value (event=value).");
DEFINE_string(pairs,
              "",
              "Records event with pairs of values separated by comma (event{key1=value1,key2=value2}).");
DEFINE_string(storage,
              "build/",
              "Path to directory (with a slash at the end) to temporarily store recorded events.");
DEFINE_bool(debug, true, "Enables debug mode for statistics engine.");
DEFINE_bool(upload, false, "If true, triggers event to forcebly upload all statistics to the server.");

using namespace std;

int main(int argc, char** argv) {
  ParseDFlags(&argc, &argv);

  Stats stats(FLAGS_url, FLAGS_storage);
  if (FLAGS_debug) {
    stats.DebugMode(true);
  }

  if (!FLAGS_event.empty()) {
    if (!FLAGS_value.empty()) {
      stats.LogEvent(FLAGS_event, FLAGS_value);
    } else if (!FLAGS_pairs.empty()) {
      TStringMap map;
      const regex re("[=,]+");
      sregex_token_iterator reg_end;
      string key;
      for (auto it = sregex_token_iterator(FLAGS_pairs.begin(), FLAGS_pairs.end(), re, -1); it != reg_end;
           ++it) {
        if (key.empty()) {
          key = *it;
          map[key] = "";
        } else {
          map[key] = *it;
          key.clear();
        }
      }
      stats.LogEvent(FLAGS_event, map);
    } else {
      stats.LogEvent(FLAGS_event);
    }
  }

  if (FLAGS_upload) {
    stats.Upload();
  }

  return 0;
}
