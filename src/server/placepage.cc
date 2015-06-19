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
  map<string, pair<uint64_t, uint64_t>> ids;
  string current_id;
  uint64_t total_pp_opens = 0;
  uint64_t total_ppp_opens = 0;

  bool IsAndroid() const { return current_id.find("A:") != string::npos; }

  void operator()(const AlohalyticsBaseEvent & event) {
  }
  void operator()(const AlohalyticsIdEvent & event) {
    current_id = event.id;
    if (IsAndroid()) {
      ids.insert(make_pair(current_id, make_pair(0, 0)));
    }
  }
  void operator()(const AlohalyticsKeyValueEvent & event) {
    if (event.key == "$onClick" & event.value == "ppOpen" && IsAndroid()) {
      ++ids[current_id].second;
      ++total_pp_opens;
    }
  }

  void operator()(const AlohalyticsKeyPairsEvent & event) {
    if (event.key == "$GetUserMark" && IsAndroid()) {
      ++ids[current_id].first;
      ++total_ppp_opens;
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
    bricks::rtti::RuntimeDispatcher<AlohalyticsBaseEvent, AlohalyticsIdEvent, AlohalyticsKeyValueEvent, AlohalyticsKeyPairsEvent>::DispatchCall(*ptr, processor);
  }

  cout << "Total processed devices: " << processor.ids.size() << endl;
  uint64_t users_who_opened_pp = 0;
  uint64_t users_who_opened_ppp = 0;
  for (auto const i : processor.ids) {
    if (i.second.second) {
      ++users_who_opened_pp;
    }
    if (i.second.first) {
      ++users_who_opened_ppp;
    }
  }
  cout << "Users who opened pp: " << users_who_opened_pp << " (" << users_who_opened_pp * 100. / (double)processor.ids.size() << "%)" << endl;
  cout << "Users who opened ppp: " << users_who_opened_ppp << " (" << users_who_opened_ppp * 100. / (double)processor.ids.size() << "%)" << endl;
  cout << "Total pp opens: " << processor.total_pp_opens / 2 << endl;
  cout << "Total ppp opens: " << processor.total_ppp_opens << endl;
  return 0;
}
