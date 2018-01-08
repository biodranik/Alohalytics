cmake_minimum_required(VERSION 3.2)

project(alohalytics C CXX)

get_filename_component(ALOHA_ROOT ../../ ABSOLUTE)

include_directories(${ALOHA_ROOT})

set(
  SRC
  ${ALOHA_ROOT}/src/cpp/alohalytics.cc
  jni/file_manager_posix_impl.cc
  jni/jni_alohalytics.cc
  jni/jni_main.cc
)

add_library(${PROJECT_NAME} SHARED ${SRC})

target_link_libraries(
  ${PROJECT_NAME}
  # Android libs
  log
  atomic
  z
)
