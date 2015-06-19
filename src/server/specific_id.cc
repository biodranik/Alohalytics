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
#include <algorithm>

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

struct UserInfo {
  uint64_t count;
  string google_id;
  bool operator<(const UserInfo & other) const {
    return other.count < count;
  }
};

struct Processor {
  string current_user_id;

  inline void operator()(const AlohalyticsBaseEvent & event) {
    const string s = event.ToString();
    if (s.find("eefa764e-e619-431c-8ac4-8267642c3e82") != string::npos) {
      cout << current_user_id << endl;
    }
  }

  inline void operator()(const AlohalyticsIdEvent & event) {
//    if (event.id == "A:a4bf637c-ba24-47af-becc-42740ef17008")
//      print = true;
//    else
//      print = false;
    current_user_id = event.id;
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
      cerr << ex.what() << endl;
      break;
    }
    bricks::rtti::RuntimeDispatcher<AlohalyticsBaseEvent, AlohalyticsIdEvent/*,
        AlohalyticsKeyPairsEvent, AlohalyticsKeyValueEvent*/>::DispatchCall(*ptr, processor);
  }

  return 0;
}
