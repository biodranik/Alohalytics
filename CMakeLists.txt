cmake_minimum_required(VERSION 3.2)

project(alohalytics C CXX)

set(CMAKE_CXX_STANDART 11)

if(APPLE)
  set(CMAKE_CXX_FLAGS "-fobjc-arc -std=c++11")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Usage:
# make test ARGS="-V"
enable_testing()

get_filename_component(ALOHA_ROOT . ABSOLUTE)

include_directories(${ALOHA_ROOT})

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_subdirectory(examples/cpp)
add_subdirectory(examples/server)
add_subdirectory(server)
add_subdirectory(tests)

set(
  SRC
  src/alohalytics.h
  src/event_base.h
  src/file_manager.h
  src/gzip_wrapper.h
  src/http_client.h
  src/location.h
  src/logger.h
  src/messages_queue.h
  src/cpp/alohalytics.cc
  examples/cpp/example.cc
)

set(
  PLATFORM_SRC
  src/posix/file_manager_posix_impl.cc
)

if(UNIX AND NOT APPLE)
  add_executable(
    ${PROJECT_NAME}
    ${SRC}
    ${PLATFORM_SRC}
    src/posix/http_client_curl.cc
  )
endif()

if(APPLE)
  set(
    OBJECTIVE_SRC
    src/alohalytics_objc.h
    src/apple/http_client_apple.mm
    src/apple/alohalytics_objc.mm
  )

  add_executable(
    ${PROJECT_NAME}
    ${SRC}
    ${PLATFORM_SRC}
    ${OBJECTIVE_SRC}
  )

  target_link_libraries(
    ${PROJECT_NAME}
    "-framework Foundation"
    "-framework SystemConfiguration"
    )
endif()

target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB Threads::Threads)
