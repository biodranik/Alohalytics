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
#include <fstream>

// Used for cereal smart-pointers polymorphic serialization.
struct NoOpDeleter {
  template <typename T>
  void operator()(T*) {}
};

struct Processor {
  cereal::BinaryOutputArchive ar;
  Processor(std::ostream & stream) : ar(stream) {}

  void operator()(const AlohalyticsBaseEvent &event) {
    std::cerr << "Unhandled event of type " << typeid(event).name() << std::endl;
  }
  void operator()(const AlohalyticsIdEvent &event) {
    AlohalyticsIdEvent2 e2;
    e2.timestamp = event.timestamp;
    e2.id = event.id;
    ar << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&e2);
  }
  void operator()(const AlohalyticsKeyEvent &event) {
    AlohalyticsKeyEvent2 e2;
    e2.timestamp = event.timestamp;
    e2.key = event.key;
    ar << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&e2);
  }
  void operator()(const AlohalyticsKeyValueEvent &event) {
    AlohalyticsKeyValueEvent2 e2;
    e2.timestamp = event.timestamp;
    e2.key = event.key;
    e2.value = event.value;
    ar << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&e2);
  }
  void operator()(const AlohalyticsKeyPairsEvent &event) {
    AlohalyticsKeyPairsEvent2 e2;
    e2.timestamp = event.timestamp;
    e2.key = event.key;
    e2.pairs = event.pairs;
    ar << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&e2);
  }
  void operator()(const AlohalyticsKeyPairsLocationEvent &event) {
    std::cerr << "Got a location event? It should not be there in v1." << std::endl;
  }
};



int main(int argc, char ** argv) {
  if (argc < 2) {
    std::cerr << "Please specify output file for binary cereal log." << std::endl;
    return -1;
  }
  std::ofstream out(argv[1], std::ios_base::binary | std::ios_base::ate);
  cereal::BinaryInputArchive ar(std::cin);
  const std::string exc("Failed to read 4 bytes from input stream! Read 0");
  Processor processor(out);
  try {
    while (std::cin.good()) {
      std::unique_ptr<AlohalyticsBaseEvent> ptr;
      ar(ptr);
      bricks::rtti::RuntimeDispatcher<AlohalyticsBaseEvent, AlohalyticsKeyPairsLocationEvent,
                                      AlohalyticsKeyPairsEvent, AlohalyticsIdEvent, AlohalyticsKeyValueEvent,
                                      AlohalyticsKeyEvent>::DispatchCall(*ptr, processor);
    }
  } catch (const cereal::Exception & ex) {
    if (exc != ex.what()) {
      std::cerr << "Cereal exception: " << ex.what() << std::endl;
      return -1;
    }
  }
  return 0;
}
