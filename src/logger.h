#include <sstream>

#if defined(__OBJC__)
  #include <Foundation/Foundation.h>
#elif defined(ANDROID)
  #include <android/log.h>
#else
  #include <iostream>
#endif

namespace aloha {

class Logger {
  std::ostringstream out_;

public:
  Logger() {
  }

  Logger(const char * file, int line) {
    out_ << file << ":" << line << ": ";
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

  template <typename T, typename ... ARGS>
  void Log(const T& arg1, const ARGS& ... others) {
    //Log(out_, arg1);
    out_ << arg1 << " ";
    Log(others...);
  }

//  template<typename T>
//  void Log(const T& t) {
//    Log(out_, t);
//  }

  void Log() {
  }

  template <typename T>
  void Log(const T& t) {
    out_ << t;
  }
};

} // namespace aloha


#if defined(__OBJC__) || defined(ANDROID)
  #define TRACE(...) aloha::Logger(__FILE__, __LINE__ ).Log(__VA_ARGS__)
  #define LOG(...) aloha::Logger().Log(__VA_ARGS__)
#else
  #define TRACE(...) aloha::Logger(__FILE__, __LINE__ ).Log(__VA_ARGS__)
  #define LOG(...) aloha::Logger().Log(__VA_ARGS__)
#endif