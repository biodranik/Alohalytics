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
  long user_age_in_days;
  bool operator<(const UserInfo & other) const {
    return other.count < count;
  }
};

struct Processor {
  string current_user_id;
  map<string,UserInfo> users;

  inline void operator()(const AlohalyticsKeyPairsEvent & event) {
    if (event.key == "$androidIds") {
      auto const found = event.pairs.find("google_advertising_id");
      if (found != event.pairs.end()) {
        users[current_user_id].google_id = found->second;
      }
    } else if (event.key == "$update") {
      auto const found = event.pairs.find("userAgeInSeconds");
      if (found != event.pairs.end()) {
        long age = std::stol(found->second) / 86400;
        if (users[current_user_id].user_age_in_days < age) {
          users[current_user_id].user_age_in_days = age;
        }
      }
    }
  }

  inline void operator()(const AlohalyticsKeyValueEvent & event) {
    if (event.key == "$onResume" && event.value == "MWMActivity") {
      users[current_user_id].count += 1;
    }
  }

  inline void operator()(const AlohalyticsBaseEvent & event) {
  }

  inline void operator()(const AlohalyticsIdEvent & event) {
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
    bricks::rtti::RuntimeDispatcher<AlohalyticsBaseEvent, AlohalyticsIdEvent,
        AlohalyticsKeyPairsEvent, AlohalyticsKeyValueEvent>::DispatchCall(*ptr, processor);
  }

  vector<UserInfo> v;
  for (const auto & it : processor.users) {
    if (!it.second.google_id.empty())
      v.push_back(it.second);
  }
  sort(v.begin(), v.end());

  cout << "Map usage count,Days installed,Google Play ID" << endl;
  for (const auto & it : v) {
    cout << it.count << ',' << it.user_age_in_days << ',' << it.google_id << endl;
  }  
  return 0;
}
