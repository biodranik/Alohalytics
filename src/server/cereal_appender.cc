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

#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <fstream>

using namespace std;

int main(int argc, char ** argv) {
  if (argc < 2) {
    cerr << "Please specify output file for binary cereal log." << endl;
    return -1;
  }
  ofstream out(argv[1], ios_base::out | ios_base::app | ios_base::binary);
  try {
    std::unique_ptr<AlohalyticsBaseEvent> ptr;
    cereal::BinaryInputArchive input(std::cin);
    while (std::cin.good()) {
      input(ptr);
      cereal::BinaryOutputArchive(out) << ptr;
    }
  } catch (const cereal::Exception & ex) {
    const std::string exc("Failed to read 4 bytes from input stream! Read 0");
    if (exc != ex.what()) {
      std::cerr << "Cereal exception: " << ex.what() << std::endl;
      return -1;
    }
  }
  return 0;
}
