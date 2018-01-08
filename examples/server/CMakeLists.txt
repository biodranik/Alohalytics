project(demo)

function(add_alohaexampleserver_executable)
  foreach(CC_FILE ${ARGN})
    set(
      SRC
      ${ALOHA_ROOT}/src/posix/file_manager_posix_impl.cc
      processor.h
      ${CC_FILE}
    )
    get_filename_component(EXEC_NAME ${CC_FILE} NAME_WE)
    add_executable(${EXEC_NAME} ${SRC})
    target_link_libraries(
      ${EXEC_NAME}
      ZLIB::ZLIB
      Threads::Threads
      )
  endforeach()
endfunction()

add_alohaexampleserver_executable(
  demo.cc
  demo2.cc
  gunzip.cc
)
