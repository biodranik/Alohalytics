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
#include <string>
#include <zlib.h>

#include "logger.h"

namespace alohalytics {

static constexpr size_t kGzipBufferSize = 32768;

struct GzipErrorException : public std::exception {
int err_;
std::string msg_;
GzipErrorException(int err, const char * msg) : err_(err), msg_(msg ? msg : "") {}
virtual char const * what() const noexcept { return ("ERROR " + std::to_string(err_) + " while gzipping with zlib. " + msg_).c_str(); }
};

struct GunzipErrorException : public std::exception {
int err_;
std::string msg_;
GunzipErrorException(int err, const char * msg) : err_(err), msg_(msg ? msg : "") {}
virtual char const * what() const noexcept { return ("ERROR " + std::to_string(err_) + " while gunzipping with zlib. " + msg_).c_str(); }
};

// Throws GzipErrorException.
inline std::string Gzip(const std::string& data_to_compress) {
  z_stream z = {};
  int res = ::deflateInit2(&z, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
  if (Z_OK == res) {
    z.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data_to_compress.data()));
    z.avail_in = data_to_compress.size();
    std::string compressed;
    char buffer[std::min(data_to_compress.size(), kGzipBufferSize)];
    do {
      z.next_out = reinterpret_cast<Bytef*>(buffer);
      z.avail_out = sizeof(buffer) / sizeof(buffer[0]);
      res = ::deflate(&z, Z_FINISH);
      if (compressed.size() < z.total_out) {
        compressed.append(buffer, z.total_out - compressed.size());
      }
    } while (Z_OK == res);
    ::deflateEnd(&z);
    if (Z_STREAM_END == res) {
      return compressed;
    }
  }
  throw GzipErrorException(res, z.msg);
}

// Returns empty string on error.
inline std::string Gunzip(const std::string& data_to_decompress)
{
  z_stream z = {};
  int res = ::inflateInit2(&z, 16 + MAX_WBITS);
  if (Z_OK == res) {
    z.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data_to_decompress.data()));
    z.avail_in = data_to_decompress.size();
    std::string decompressed;
    char buffer[std::min(data_to_decompress.size(), kGzipBufferSize)];
    do {
      z.next_out = reinterpret_cast<Bytef*>(buffer);
      z.avail_out = sizeof(buffer) / sizeof(buffer[0]);
      res = ::inflate(&z, Z_NO_FLUSH);
      if (decompressed.size() < z.total_out) {
        decompressed.append(buffer, z.total_out - decompressed.size());
      }
    } while (Z_OK == res);
    ::inflateEnd(&z);
    if (Z_STREAM_END == res) {
      return decompressed;
    }
  }
  throw GunzipErrorException(res, z.msg);
}

}  // namespace alohalytics
