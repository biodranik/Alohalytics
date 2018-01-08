# Created by Viacheslav Greshilov http://github.com/greshilov

# Uncomment when new versions of fcgi will be available on brew (09.01.2018)
#file(GLOB _BREW_FCGI "/usr/local/Cellar/fcgi/*")
#list(SORT _BREW_FCGI)
#list(REVERSE _BREW_FCGI)

set(
  _FCGI_SOURCE_PATHS
  /usr/
  /usr/local/
  #${_BREW_FCGI}
)

find_path(
  FCGI_INCLUDE_DIR
  NAMES fcgio.h fcgiapp.h
  PATHS ${_FCGI_SOURCE_PATHS}
  PATH_SUFFIXES include
)

find_library(
  FCGI_LIBRARY
  NAMES fcgi libfcgi
  PATHS ${_FCGI_SOURCE_PATHS}
  PATH_SUFFIXES lib
)

find_library(
  FCGI++_LIBRARY
  NAMES fcgi++ libfcgi++
  PATHS ${_FCGI_SOURCE_PATHS}
  PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  FCGI
  REQUIRED_VARS FCGI_LIBRARY
                FCGI_INCLUDE_DIR
  )

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  FCGI++
  REQUIRED_VARS FCGI++_LIBRARY
                FCGI_INCLUDE_DIR
  )

if(FCGI_FOUND)
  set(FCGI_INCLUDE_DIRS ${FCGI_INCLUDE_DIR})
  if(NOT FCGI_LIBRARIES)
    set(FCGI_LIBRARIES ${FCGI_LIBRARY})
  endif()
  if(NOT TARGET FCGI::FCGI)
    add_library(FCGI::FCGI UNKNOWN IMPORTED)
    set_target_properties(FCGI::FCGI
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FCGI_INCLUDE_DIRS}")
    set_property(TARGET FCGI::FCGI APPEND
    PROPERTY IMPORTED_LOCATION "${FCGI_LIBRARY}")
  endif()
endif()

if(FCGI++_FOUND)
  if(NOT FCGI_LIBRARIES)
    set(FCGI_LIBRARIES ${FCGI_LIBRARY})
  else()
    set(FCGI_LIBRARIES ${FCGI_LIBRARIES} ${FCGI++_LIBRARY})
  endif()
  if(NOT TARGET FCGI::FCGI++)
    add_library(FCGI::FCGI++ UNKNOWN IMPORTED)
    set_target_properties(FCGI::FCGI++
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FCGI_INCLUDE_DIRS}")
    set_property(TARGET FCGI::FCGI++ APPEND
    PROPERTY IMPORTED_LOCATION "${FCGI++_LIBRARY}")
  endif()
endif()
