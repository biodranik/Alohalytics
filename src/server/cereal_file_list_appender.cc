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

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../event_base.h"
#include "../gzip_wrapper.h"

#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <fstream>

#include "../Bricks/file/file.h"

int main(int argc, char ** argv) {
  if (argc < 3) {
    std::cerr << "Syntax: " << argv[0] << " <gzipped_files_list> <output_file>" << std::endl;
    return -1;
  }
  std::ifstream flist(argv[1]);
  std::ofstream out(argv[2], std::ios_base::binary);
  cereal::BinaryOutputArchive outstream(out);
  const std::string exc("Failed to read 4 bytes from input stream! Read 0");
  while (flist.good()) {
    std::string path;
    std::getline(flist, path);
    std::string ungzipped;
    try {
      const std::string gzipped = bricks::ReadFileAsString(path);
      ungzipped = alohalytics::Gunzip(gzipped);
      if (ungzipped.empty()) {
        std::cerr << "Error ungzipping " << path << std::endl;
        continue;
      }
    } catch (const std::exception & ex) {
      std::cerr << "Error reading " << path << std::endl;
      continue;
    }
    std::istringstream in(ungzipped);
    cereal::BinaryInputArchive ar(in);
    try {
      while (!in.eof()) {
        std::unique_ptr<AlohalyticsBaseEvent> ptr;
        ar(ptr);
        outstream << ptr;
      }
    } catch (const cereal::Exception & ex) {
//      if (exc != ex.what()) {
        std::cerr << "Cereal exception: " << ex.what() << std::endl;
        continue;
//      }
    }
  }
  return 0;
}
