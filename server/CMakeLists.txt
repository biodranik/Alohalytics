project(alohaserver)

find_package(FCGI REQUIRED)

function(add_alohaserver_executable)
  foreach(CC_FILE ${ARGN})
    set(
      SRC
      ${ALOHA_ROOT}/src/posix/file_manager_posix_impl.cc
      ${CC_FILE}
    )
    get_filename_component(EXEC_NAME ${CC_FILE} NAME_WE)
    add_executable(${EXEC_NAME} ${SRC})
    target_link_libraries(
      ${EXEC_NAME}
      ZLIB::ZLIB
      Threads::Threads
      FCGI::FCGI
      FCGI::FCGI++
      )
  endforeach()
endfunction()

add_alohaserver_executable(
  date_splitter.cc
  logs_processor.cc
  fcgi_server.cc
)
