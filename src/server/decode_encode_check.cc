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

// Decerialize and cerialize back, compare if equal.

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../event_base.h"

#include "../Bricks/file/file.h"

#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <fstream>

using namespace std;

int main(int argc, char ** argv) {
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " <cereal_file>" << endl;
    return -1;
  }
  try {
    string result;
    const string original = bricks::ReadFileAsString(argv[1]);
    istringstream in_stream(original);
    cereal::BinaryInputArchive in_ar(in_stream);
    unique_ptr<AlohalyticsBaseEvent> ptr;
    ostringstream out_stream;
    const size_t bytes_to_read = original.size();
    while (bytes_to_read > in_stream.tellg()) {
      in_ar(ptr);
      // Cereal does not check if binary data is valid.
      if (ptr.get() == nullptr) {
        cerr << "this == 0 for " << argv[1] << endl;
        return -3;
      }
      // Detect segfault by causing it if binary data is invalid.
      (void)ptr->ToString();
      cereal::BinaryOutputArchive(out_stream) << ptr;
    }
    result = out_stream.str();
    ofstream(std::string(argv[1]) + ".created").write(result.data(), result.size());
    
    if (result != original) {
      cerr << "Decoded/encoded file is not equal to original " << result.size() << ":" << original.size();
      cerr << " for file " << argv[1] << endl;
      return -2;
    }
  } catch (const exception & ex) {
    cerr << argv[1] << " " << typeid(ex).name() << " " << ex.what() << endl;
    return -1;
  }
  return 0;
}
