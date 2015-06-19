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

// This snippet calculates how many ids from input file were found in the events flow.

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../event_base.h"
#include "../file_manager.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>

using namespace std;
using namespace alohalytics;

int main(int argc, char ** argv) {
  if (argc < 3) {
    cout << "Usage: cat <data files> | " << argv[0] << " <-android or -ios> <file_with_ids>" << endl;
    return -1;
  }
  auto const start_time = chrono::system_clock::now();
  const bool is_android = (string(argv[1]).find("android") != string::npos);
  set<string> ids_from_file;
  {
    string id;
    istringstream ids_stream(FileManager::ReadFileAsString(argv[2]));
    while(ids_stream.good()) {
      getline(ids_stream, id);
      if (!id.empty()) {
        // Google Advertising Ids are always in lower case.
        transform(id.begin(), id.end(), id.begin(), (int (*)(int))tolower);
        ids_from_file.emplace(std::move(id));
      }
    }
    cout << "Loaded " << ids_from_file.size() << " ids from " << argv[2] << endl;
  }
  set<string> matched_ids, all_ids;
  cereal::BinaryInputArchive ar(std::cin);
  unique_ptr<AlohalyticsBaseEvent> ptr;
  uint64_t total_events_processed = 0;

  const auto lambda = [&ids_from_file, &matched_ids, &all_ids](AlohalyticsKeyPairsEvent const & event, const char * event_name, const char * key) {
      if (event.key == event_name) {
        const auto found_key = event.pairs.find(key);
        if (found_key != event.pairs.end()) {
          all_ids.insert(found_key->second);
          const auto matched = ids_from_file.find(found_key->second);
          if (matched != ids_from_file.end()) {
            matched_ids.insert(*matched);
          }
        }
      }
  };
  while (true) {
    try {
      ar(ptr);
    } catch (const cereal::Exception & ex) {
      if (string("Failed to read 4 bytes from input stream! Read 0") != ex.what()) {
        // The exception above is a normal one, Cereal lacks to detect the end of the stream.
        cout << ex.what() << endl;
      }
      break;
    }
    AlohalyticsKeyPairsEvent const * kv_event = dynamic_cast<AlohalyticsKeyPairsEvent const *>(ptr.get());
    if (kv_event) {
      if (is_android) {
        lambda(*kv_event, "$androidIds", "google_advertising_id");
      } else {
        lambda(*kv_event, "$iosDeviceIds", "advertisingIdentifier");
      }
    }
    ++total_events_processed;
  }
  cerr << "Total processing time: " << std::chrono::duration_cast<std::chrono::seconds>(chrono::system_clock::now() - start_time).count() << " seconds." << endl;
  cerr << "Total events processed: " << total_events_processed << endl;
  cerr << "Total unique ids processed: " << all_ids.size() << endl;
  cout << "Matched " << matched_ids.size() << " ids." << endl;
  return 0;
}
