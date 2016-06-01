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
#include "../../src/event_base.h"
#include "../../../include/processor.h" // TODO: This is actually not from Alohalytics repo

#include <algorithm>
#include <iostream>
#include <string>
#include <set>

#define DLLEXPORT extern "C"

// Public struct used in Python
struct UserInfo
{
  char os_t;
  float lat;
  float lon;
  char raw_uid[32];
};

// Public struct used in Python
struct EventTime
{
  uint64_t client_created;
  uint64_t server_upload;
};

typedef void (*callbackFunction)(char const *, EventTime const &, UserInfo const &, char const **, int);

// Helper function that convert KeyPairs event into list of (key, value) for use in Python
void SetPairs(AlohalyticsKeyPairsEvent const * event, std::vector<char const *> & pairs)
{
  int i = 0;
  for (std::pair<std::string, std::string> const & kp : event->pairs)
  {
    pairs[i++] = kp.first.c_str();
    pairs[i++] = kp.second.c_str();
  }
}

// Helper function for setting up geo coords
void SetGeo(alohalytics::Location const kLocation, UserInfo & userInfo)
{
  if (kLocation.HasLatLon()) {
    userInfo.lat = (float)kLocation.latitude_deg_;
    userInfo.lon = (float)kLocation.longitude_deg_;
  }
}

/*
Helper function for setting up os type as a num for use in Python
NOTE: C char will convert into python string - so we do not want that
*/
void SetOSType(char osType, UserInfo& userInfo)
{
  switch (osType)
  {
    case 'A':
    {
      userInfo.os_t = 1;
      break;
    }
    case 'I':
    {
      userInfo.os_t = 2;
      break;
    }
  };
}

std::string CompressUID(std::string uid)
{
  uid.erase(0, 2);
  // TODO: think about just using fixed positions instead of more general attempt with searching
  uid.erase(std::remove(uid.begin(), uid.end(), '-'), uid.end());
  return uid;
}

/*
This is public function used in Python as a main loop
NOTE: We need a C interface, so prepare yourself for C-style structures in the params of
this function (called from a Python code with a ctypes wrapper) and
callback function (actual Python function - also a ctypes wrapper)
*/

DLLEXPORT void iterate(callbackFunction callback, char const ** kEventNames, int eventNamesNum)
{
  std::set<std::string> processKeys;
  for (int i = 0; i < eventNamesNum; i++)
  {
    processKeys.insert(kEventNames[i]);
  }

  AlohalyticsIdServerEvent const * kPrevious = nullptr;
  alohalytics::Processor(
      [&kPrevious, callback, &processKeys](AlohalyticsIdServerEvent const * kIdEvent, AlohalyticsKeyEvent const * kEvent)
      {

        if (!processKeys.empty() && !processKeys.count(kEvent->key))
        {
          return;
        }

        EventTime const kEventTime = {
          .client_created = kEvent->timestamp,
          .server_upload = kIdEvent->server_timestamp
        };

        UserInfo userInfo = {
          .os_t = 0,
          .lat = 0.0,
          .lon = 0.0
        };

        SetOSType(kIdEvent->id[0], userInfo);

        std::memcpy(userInfo.raw_uid, CompressUID(kIdEvent->id).c_str(), 32);

        char const * kKey = kEvent->key.c_str();

        AlohalyticsKeyValueEvent const * kve = dynamic_cast<AlohalyticsKeyValueEvent const *>(kEvent);
        if (kve)
        {
          char const * kValues[1] = {kve->value.c_str()};
          callback(kKey, kEventTime, userInfo, kValues, 1);
          return;
        }

        AlohalyticsKeyPairsEvent const * kpe = dynamic_cast<AlohalyticsKeyPairsEvent const *>(kEvent);
        if (kpe)
        {
          std::vector<char const *> pairs(kpe->pairs.size() * 2);
          SetPairs(kpe, pairs);
          callback(kKey, kEventTime, userInfo, pairs.data(), pairs.size());
          return;
        }

        AlohalyticsKeyLocationEvent const * kle = dynamic_cast<AlohalyticsKeyLocationEvent const *>(kEvent);
        if (kle)
        {
          SetGeo(kle->location, userInfo);
          callback(kKey, kEventTime, userInfo, NULL, 0);
          return;
        }

        AlohalyticsKeyPairsLocationEvent const * kple = dynamic_cast<AlohalyticsKeyPairsLocationEvent const *>(kEvent);
        if (kple)
        {
          std::vector<char const *> pairs(kple->pairs.size() * 2);
          SetPairs(kple, pairs);
          SetGeo(kple->location, userInfo);
          callback(kKey, kEventTime, userInfo, pairs.data(), pairs.size());
          return;
        }
      });
}
