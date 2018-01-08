project(example)

set(
  SRC
  ${ALOHA_ROOT}/src/cpp/alohalytics.cc
  ${ALOHA_ROOT}/src/posix/file_manager_posix_impl.cc
  dflags.h
  example.cc
)

if (UNIX)
  set(
    PLATFORM_SRC
    ${ALOHA_ROOT}/src/posix/http_client_curl.cc
  )
endif()

add_executable(${PROJECT_NAME} ${SRC} ${PLATFORM_SRC})

target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB Threads::Threads)
