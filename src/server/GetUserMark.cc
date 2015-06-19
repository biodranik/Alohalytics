/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

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

// Small demo which prints raw (ungzipped) cereal stream from stdin.

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../event_base.h"

#include "../Bricks/rtti/dispatcher.h"

#include <iostream>
#include <iomanip>
#include <typeinfo>

using namespace std;

struct Processor {
  map<string,uint64_t> ids;
  map<string,uint64_t> events;
  string current_id;
  void PrintTime(const AlohalyticsBaseEvent & event) {
    const time_t timestamp = static_cast<const time_t>(event.timestamp / 1000);
    char buf[100];
    if (::strftime(buf, 100, "%e-%b-%Y %H:%M:%S", ::localtime(&timestamp))) {
      std::cout << buf;
    } else {
      std::cout << "INVALID_TIME";
    }
  }
  void operator()(const AlohalyticsBaseEvent & event) {
//    PrintTime(event);
//    std::cout << "Unhandled event of type " << typeid(event).name() << std::endl;
  }
  void operator()(const AlohalyticsIdEvent & event) {
    ++ids[event.id];
    current_id = event.id;
//    PrintTime(event);
//    std::cout << " ID: " << event.id << std::endl;
  }
  void operator()(const AlohalyticsKeyPairsEvent & event) {
    if (event.key == "$GetUserMark") {
      auto const found = event.pairs.find("type");
      if (found != event.pairs.end()) {
        ++events[found->second];
      }
    }
  }
/*  void operator()(const AlohalyticsKeyValueEvent & event) {
    PrintTime(event);
    std::cout << ' ' << event.key << " = " << event.value << std::endl;
  }
  void operator()(const AlohalyticsKeyPairsEvent & event) {
    PrintTime(event);
    std::cout << ' ' << event.key << " [ ";
    for (const auto & pair : event.pairs) {
      std::cout << pair.first << '=' << pair.second << ' ';
    }
    std::cout << ']' << std::endl;
  }
*/};

int main(int, char **) {
  cereal::BinaryInputArchive ar(std::cin);
  Processor processor;
  while (true) {
    std::unique_ptr<AlohalyticsBaseEvent> ptr;
    try {
      ar(ptr);
    } catch (const cereal::Exception & ex) {
      cout << "Cereal exception" << endl;
      break;
    }
    try {
      bricks::rtti::RuntimeDispatcher<AlohalyticsBaseEvent,
                                      AlohalyticsKeyPairsEvent,
                                      AlohalyticsIdEvent
>::DispatchCall(*ptr, processor);
    } catch (const bricks::rtti::UnrecognizedPolymorphicType & ex) {
    }
  }

  map<uint64_t, string, std::greater<uint64_t>> reversed;

  for (auto const & type : processor.events) {
    reversed[type.second] = type.first;
  }
  for (auto const & type : reversed) {
    cout << type.first << " " << type.second << endl;
  }

  return 0;
}
