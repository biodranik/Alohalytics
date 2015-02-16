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

// Small demo which ungzips and prints cereal stream from file.

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../../src/event_base.h"
#include "../../src/gzip_wrapper.h"

#include "../../src/Bricks/file/file.h"
#include "../../src/Bricks/rtti/dispatcher.h"

#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <fstream>

struct Processor {
  void PrintTime(const AlohalyticsBaseEvent &event) {
    const time_t timestamp = static_cast<const time_t>(event.timestamp / 1000);
    // std::put_time is not implemented in gcc4.8 yet.
    // std::cout << std::put_time(std::localtime(&timestamp), "%e-%b-%Y %H:%M:%S");
    char buf[100];
    if (::strftime(buf, 100, "%e-%b-%Y %H:%M:%S", ::localtime(&timestamp))) {
      std::cout << buf;
    } else {
      std::cout << "INVALID_TIME";
    }
  }
  void operator()(const AlohalyticsBaseEvent &event) {
    PrintTime(event);
    std::cout << "Unhandled event of type " << typeid(event).name() << std::endl;
  }
  void operator()(const AlohalyticsIdEvent &event) {
    PrintTime(event);
    std::cout << " ID: " << event.id << std::endl;
  }
  void operator()(const AlohalyticsKeyEvent &event) {
    PrintTime(event);
    std::cout << ' ' << event.key << std::endl;
  }
  void operator()(const AlohalyticsKeyValueEvent &event) {
    PrintTime(event);
    std::cout << ' ' << event.key << " = " << event.value << std::endl;
  }
  void operator()(const AlohalyticsKeyPairsEvent &event) {
    PrintTime(event);
    std::cout << ' ' << event.key << " [ ";
    for (const auto &pair : event.pairs) {
      std::cout << pair.first << '=' << pair.second << ' ';
    }
    std::cout << ']' << std::endl;
  }
  void operator()(const AlohalyticsKeyPairsLocationEvent &event) {
    PrintTime(event);
    std::cout << ' ' << event.key << " [ ";
    for (const auto &pair : event.pairs) {
      std::cout << pair.first << '=' << pair.second << ' ';
    }
    std::cout << "] " << event.location.ToDebugString() << std::endl;
  }
};

int main(int argc, char ** argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <gzipped_cereal_file>" << std::endl;
    return -1;
  }
  std::string ungzipped;
  try {
    {
      const std::string gzipped = bricks::ReadFileAsString(argv[1]);
      ungzipped = alohalytics::Gunzip(gzipped);
    }
    std::istringstream in_stream(ungzipped);
    cereal::BinaryInputArchive ar(in_stream);
    Processor processor;
    std::unique_ptr<AlohalyticsBaseEvent> ptr;
    while (static_cast<std::streamoff>(ungzipped.size()) > in_stream.tellg()) {
      ar(ptr);
      bricks::rtti::RuntimeDispatcher<AlohalyticsBaseEvent, AlohalyticsKeyPairsLocationEvent,
                                      AlohalyticsKeyValueLocationEvent, AlohalyticsKeyLocationEvent,
                                      AlohalyticsKeyPairsEvent, AlohalyticsIdEvent, AlohalyticsKeyValueEvent,
                                      AlohalyticsKeyEvent>::DispatchCall(*ptr, processor);
    }
  } catch (const std::exception & ex) {
    std::cerr << "Exception: " << ex.what() << std::endl;
    return -1;
  }
  return 0;
}
