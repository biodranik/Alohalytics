project(alohatests)

add_subdirectory(googletest)

include_directories(googletest googletest/include)

set(
  SRC
  generate_temporary_file_name.h
  test_file_manager.cc
  test_gzip.cc
  test_location.cc
  test_messages_queue.cc
  test_statistics_receiver.cc

  ${ALOHA_ROOT}/src/posix/file_manager_posix_impl.cc

  googletest/src/gtest-all.cc
  googletest/src/gtest_main.cc
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB Threads::Threads)

add_test(tests ${PROJECT_NAME})
