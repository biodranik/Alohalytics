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

// This snippet simply dumps to stdout every matched event with a given key.

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../event_base.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

using namespace std;

int main(int argc, char ** argv) {
  if (argc < 2) {
    cout << "Usage: cat <data files> | " << argv[0] << " <key>" << endl;
    return -1;
  }
  const string key_to_find(argv[1]);
  string user_id;
  cereal::BinaryInputArchive ar(std::cin);
  unique_ptr<AlohalyticsBaseEvent> ptr;
  uint64_t total_events_processed = 0, events_matched = 0;
  auto const start_time = chrono::system_clock::now();
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
    AlohalyticsIdServerEvent const * id_event = dynamic_cast<AlohalyticsIdServerEvent const *>(ptr.get());
    if (id_event) {
      user_id = id_event->id;
    } else {
      AlohalyticsKeyEvent const * key_event = dynamic_cast<AlohalyticsKeyEvent const *>(ptr.get());
      if (key_event && key_event->key == key_to_find) {
        cout << user_id << " " << key_event->ToString() << endl;
        ++events_matched;
      }
    }
    ++total_events_processed;
  }
  cerr << "Total processing time: " << std::chrono::duration_cast<std::chrono::seconds>(chrono::system_clock::now() - start_time).count() << " seconds." << endl;
  cerr << "Total events processed: " << total_events_processed << endl;
  cerr << "Found " << events_matched << " events with a key " << key_to_find << endl;
  return 0;
}
