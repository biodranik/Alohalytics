/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2015 Maps.Me

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
#include "../Alohalytics/src/event_base.h"
#include "../include/processor.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <set>

#define DLLEXPORT extern "C"

struct UserInfo {
  char os_t;
  float lat;
  float lon;
  char raw_uid[32];
};

struct EventTime {
  uint64_t client_created;
  uint64_t server_upload;
};

void set_pairs(const AlohalyticsKeyPairsEvent * event, std::vector<const char *> & pairs) {
  int i = 0;
  for (const std::pair<std::string, std::string> & p : event->pairs) {
    pairs[i++] = p.first.c_str();
    pairs[i++] = p.second.c_str();
  }
}

void set_geo(const alohalytics::Location location, UserInfo & user_info) {
  if (location.HasLatLon()) {
    user_info.lat = (float)location.latitude_deg_;
    user_info.lon = (float)location.longitude_deg_;
  }
}

std::string compress_uid(std::string uid) {
  uid.erase(0, 2);
  // TODO: think about just using fixed positions instead of more general attempt with searching
  uid.erase(std::remove(uid.begin(), uid.end(), '-'), uid.end());
  return uid;
}

// We need a C interface, so prepare yourself for C-style structures in the params of
// this function (called from a Python code with a ctypes wrapper) and
// callback function (actual Python function - also a ctypes wrapper)

DLLEXPORT void iterate(void (*callback)(const char *, const EventTime &, const UserInfo &, const char **, int),
                       const char ** events,
                       int events_num) {
  std::set<std::string> process_keys;
  for (int i = 0; i < events_num; i++) {
    process_keys.insert(events[i]);
  }

  const AlohalyticsIdServerEvent * previous = nullptr;
  alohalytics::Processor(
      [&previous, callback, &process_keys](const AlohalyticsIdServerEvent * se, const AlohalyticsKeyEvent * e) {

        if (!process_keys.empty() && !process_keys.count(e->key)) {
          return;
        }

        const EventTime event_time = {.client_created = e->timestamp, .server_upload = se->server_timestamp};
        UserInfo user_info = {.os_t = 0, .lat = 0.0, .lon = 0.0};
        switch (se->id[0]) {
          case 'A': {
            user_info.os_t = 1;
            break;
          }
          case 'I': {
            user_info.os_t = 2;
            break;
          }
        };

        std::memcpy(user_info.raw_uid, compress_uid(se->id).c_str(), 32);

        const char * key = e->key.c_str();

        const AlohalyticsKeyValueEvent * kve = dynamic_cast<const AlohalyticsKeyValueEvent *>(e);
        if (kve) {
          const char * values[1] = {kve->value.c_str()};
          callback(key, event_time, user_info, values, 1);
          return;
        }

        const AlohalyticsKeyPairsEvent * kpe = dynamic_cast<const AlohalyticsKeyPairsEvent *>(e);
        if (kpe) {
          std::vector<const char *> pairs(kpe->pairs.size() * 2);
          set_pairs(kpe, pairs);
          callback(key, event_time, user_info, pairs.data(), pairs.size());
          return;
        }

        const AlohalyticsKeyLocationEvent * kle = dynamic_cast<const AlohalyticsKeyLocationEvent *>(e);
        if (kle) {
          set_geo(kle->location, user_info);
          callback(key, event_time, user_info, NULL, 0);
          return;
        }

        const AlohalyticsKeyPairsLocationEvent * kple = dynamic_cast<const AlohalyticsKeyPairsLocationEvent *>(e);
        if (kple) {
          std::vector<const char *> pairs(kple->pairs.size() * 2);
          set_pairs(kple, pairs);
          set_geo(kple->location, user_info);
          callback(key, event_time, user_info, pairs.data(), pairs.size());
          return;
        }
      });
}
