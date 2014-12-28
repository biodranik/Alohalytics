#ifndef LOGGER_H
#define LOGGER_H

#include <sstream>

#if defined(__OBJC__)
#include <Foundation/Foundation.h>
#elif defined(ANDROID)
#include <android/log.h>
#else
#include <iostream>
#endif

// Pretty-printing for std::pair.
template <typename T, typename U>
std::ostream& operator<<(std::ostream& out, const std::pair<T, U>& p) {
  out << "[" << p.first << "=" << p.second << "]";
  return out;
}

namespace aloha {

class Logger {
  std::ostringstream out_;

 public:
  Logger() {
  }

  Logger(const char* file, int line) {
    out_ << file << ':' << line << ": ";
  }

  ~Logger() {
    out_ << std::endl;
#if defined(__OBJC__)
    @autoreleasepool {
      NSLog(@"%s", out_.str().c_str());
    }
#elif defined(ANDROID)
    __android_log_print(ANDROID_LOG_INFO, "Alohalytics", "%s", out_.str().c_str());
#else
    std::cout << out_.str();
#endif
  }

  template <typename T, typename... ARGS>
  void Log(const T& arg1, const ARGS&... others) {
    out_ << arg1 << ' ';
    Log(others...);
  }

  void Log() {
  }

  template <typename T>
  void Log(const T& t) {
    out_ << t;
  }

  // String specialization to avoid printing every character as a container's element.
  void Log(const std::string& t) {
    out_ << t;
  }

  // Pretty-printing for containers.
  template <template <typename, typename...> class ContainerType, typename ValueType, typename... Args>
  void Log(const ContainerType<ValueType, Args...>& c) {
    out_ << '{';
    size_t index = 0;
    const size_t count = c.size();
    for (const auto& v : c) {
      out_ << v << (++index == count ? '}' : ',');
    }
  }
};

}  // namespace aloha

#if defined(__OBJC__) || defined(ANDROID)
#define TRACE(...) aloha::Logger(__FILE__, __LINE__).Log(__VA_ARGS__)
#define LOG(...) aloha::Logger().Log(__VA_ARGS__)
#else
#define TRACE(...) aloha::Logger(__FILE__, __LINE__).Log(__VA_ARGS__)
#define LOG(...) aloha::Logger().Log(__VA_ARGS__)
#endif

#endif
