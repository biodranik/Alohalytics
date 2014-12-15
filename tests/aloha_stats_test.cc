#include "gtest/gtest.h"
#include "gtest/gtest-main.h"
// Uncomment and use if needed.
//#include "../examples/cpp/dflags.h"

#include "../src/aloha_stats.h"

const char * TEST_SERVER = "http://httpbin.org/post";
const char * TEST_PATH = "build/";

TEST(AlohaStats, UniversalDebugAnswer) {
  aloha::Stats stats(TEST_SERVER, TEST_PATH);
  stats.LogEvent("Aloha!\n");
  EXPECT_EQ(42, stats.UniversalDebugAnswer());
  // TODO: aloha::Stats now for Android/iOS UI purposes posts async in detached thread.
  // It is temporary implementation and should be rewritten.
  std::chrono::milliseconds dura( 3000 );
  std::this_thread::sleep_for( dura );
}
