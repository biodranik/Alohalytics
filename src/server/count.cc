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
#include "../event_base.h"
#include "../gzip_wrapper.h"

#include "../Bricks/file/file.h"

#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <fstream>

using namespace std;

int main(int argc, char ** argv) {
  if (argc < 3) {
    cout << "Usage: " << argv[0] << " <gzipped_cereal_file> <merged_cereal_file>" << endl;
    return -1;
  }
  int count = 0;
  try {
    string ungzipped;
    string result;
    {
      const string gzipped = bricks::ReadFileAsString(argv[1]);
      ungzipped = alohalytics::Gunzip(gzipped);
    }
    {
      istringstream in_stream(ungzipped);
      cereal::BinaryInputArchive in_ar(in_stream);
      unique_ptr<AlohalyticsBaseEvent> ptr;
      const size_t bytes_to_read = ungzipped.size();
      while (bytes_to_read > in_stream.tellg()) {
        in_ar(ptr);
        ++count;
      }
    }
  } catch (const exception & ex) {
    cerr << argv[1] << " " << typeid(ex).name() << " " << ex.what() << endl;
    return -1;
  }
  cout << count << endl;
  return 0;
}
