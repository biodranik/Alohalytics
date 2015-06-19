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

  void operator()(const AlohalyticsBaseEvent & event) {
    if (!current_id.empty()) {
      ids[current_id].emplace(event.ToString());
    }
  }

  map<string, set<string>> ids;
  string current_id;

  void operator()(const AlohalyticsIdEvent & event) {
    if (event.id.find("I:") == 0) {
      ids.emplace(event.id, set<string>());
      current_id = event.id;
    } else {
      current_id.clear();
    }
  }

  void operator()(const AlohalyticsKeyPairsEvent & event) {
    if (!current_id.empty()) {
/*      if (event.key == "$iosDeviceIds") {
        auto const found = event.pairs.find("advertisingIdentifier");
        if (found != event.pairs.end()) {
          ids_ios.insert(found->second);
        }
      } else if (event.key == "$androidIds") {
        auto const found = event.pairs.find("google_advertising_id");
        if (found != event.pairs.end()) {
          ids_android.insert(found->second);
        }
      }*/
      ids[current_id].emplace(event.ToString());
    }
  }

  uint64_t minimum_timestamp;
  Processor() {
    struct std::tm date = {};
    date.tm_mday = 11;
    date.tm_mon = 3;
    date.tm_year = 115;
    minimum_timestamp = std::mktime(&date) * 1000;
  }

  bool PassesFilter(uint64_t time) {
    return time > minimum_timestamp;
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
    bricks::rtti::RuntimeDispatcher<AlohalyticsBaseEvent, AlohalyticsIdEvent, AlohalyticsKeyPairsEvent>::DispatchCall(*ptr, processor);
  }

  cout << "Total unique users: " << processor.ids.size() << endl;
  for (const auto & i : processor.ids) {
    cout << "            " << i.first << endl;
    for (const auto & j : i.second) {
      cout << j << endl;
    }
  }
  return 0;
}
