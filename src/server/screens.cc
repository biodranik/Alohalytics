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

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../event_base.h"

#include "../Bricks/rtti/dispatcher.h"

#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <set>
#include <cmath>

using namespace std;

std::string TimestampToString(uint64_t timestamp_ms, bool day_only = false) {
  const time_t timestamp = static_cast<const time_t>(timestamp_ms / 1000);
  char buf[100];
  const char * format = "%e-%b-%Y %H:%M:%S";
  if (day_only) { format = "%Y-%m-%d"; }
  if (::strftime(buf, 100, format, ::gmtime(&timestamp))) {
    return buf;
  } else {
    return "INVALID_TIME";
  }
}

struct Processor {
  set<string> ids;
  string current_id;
  uint64_t landscape = 0;
  uint64_t portrait = 0;
  uint64_t bad = 0;
  map<int, uint64_t> inches;
  map<string, uint64_t> resolutions;

  void operator()(const AlohalyticsBaseEvent & event) {
  }
  void operator()(const AlohalyticsIdEvent & event) {
    current_id = event.id;
  }
  void operator()(const AlohalyticsKeyValueEvent & event) {
    if (event.key == "$onResume") {
      size_t found = event.value.find(':');
      if (found != string::npos) {
        if (event.value[found + 1] == '-') {
          ++landscape;
        } else {
          ++portrait;
        }
      }
    }
  }

  void operator()(const AlohalyticsKeyPairsEvent & event) {
    if (event.key == "$androidDeviceInfo" && ids.find(current_id) == ids.end() ) {
      ids.insert(current_id);
      auto height_pixels = event.pairs.find("display_height_pixels");
      auto width_pixels = event.pairs.find("display_width_pixels");
      auto xdpi = event.pairs.find("display_xdpi");
      auto ydpi = event.pairs.find("display_ydpi");
      auto end = event.pairs.end();
      if (height_pixels != end && width_pixels != end && xdpi != end && ydpi != end) {
        double w = stoi(width_pixels->second) / stod(xdpi->second);
        double h = stoi(height_pixels->second) / stod(ydpi->second);
        double inc = sqrt(w*w + h*h);
        ++inches[trunc(inc)];
        string const first = width_pixels->second + "x" + height_pixels->second;
        auto f1 = resolutions.find(first);
        if (f1 == resolutions.end()) {
          string const second = height_pixels->second + "x" + width_pixels->second;
          auto f2 = resolutions.find(second);
          if (f2 == resolutions.end()) {
            ++resolutions[first];
          } else {
            ++f2->second;
          }
        } else {
          ++f1->second;
        }
      } else {
        ++bad;
      }
    }
  }
};

int main(int, char **) {
  cereal::BinaryInputArchive ar(std::cin);
  Processor processor;
  std::unique_ptr<AlohalyticsBaseEvent> ptr;
  while (true) {
    try {
      ar(ptr);
    } catch (const cereal::Exception & ex) {
      cout << ex.what() << endl;
      break;
    }
    bricks::rtti::RuntimeDispatcher<AlohalyticsBaseEvent, AlohalyticsIdEvent, AlohalyticsKeyValueEvent,
      AlohalyticsKeyPairsEvent>::DispatchCall(*ptr, processor);
  }

  cout << "Total processed devices: " << processor.ids.size() << endl;
  cout << "Bad devices: " << processor.bad << endl;
  double te = (double)(processor.landscape + processor.portrait);
  cout << "Landscape events: " << (double)processor.landscape * 100 / te << "%" << endl;
  cout << "Portrait events: " << (double)processor.portrait * 100 / te << "%" << endl;
  cout << "Inches:" << endl;
  for (auto i : processor.inches) {
    cout << i.first << ":" << i.second * 100 / (double)processor.ids.size() << endl;
  }
  cout << "Resolutions:" << endl;
  for (auto i : processor.resolutions) {
    cout << i.first << ":" << i.second * 100 / (double)processor.ids.size() << endl;
  }
  return 0;
}
