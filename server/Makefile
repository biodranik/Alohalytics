.PHONY: run all indent clean

CPP=g++
CPPFLAGS=-std=c++11 -Wall -W -I${INCLUDES_PARAMS} -O3
LDFLAGS=-L${LIBS} -lz -pthread -lfcgi -lfcgi++
MORE_SOURCES=../src/posix/file_manager_posix_impl.cc

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=build/%)
OBJ=build/file_manager_posix_impl.obj
OS := $(shell uname)
ifeq ($(OS),Darwin)
  INCLUDES=/usr/local/Cellar/fcgi/2.4.0/include ../
  LIBS=/usr/local/Cellar/fcgi/2.4.0/lib
else
  ifeq ($(OS),Linux)
    INCLUDES=/usr/include
    LIBS=/usr/lib
  endif
endif
# Expand directories separated with spaces into a list of compiler parameters -Idir1 -Idir2 etc.
INCLUDES_PARAMS=$(foreach d, $(INCLUDES), -I$d)

all: build ${BIN}

run: all
	spawn-fcgi -n -p 8888 -- build/fcgi_server /tmp/ /monitoring

indent:
	(find . -name "*.cc" -o -name "*.h") | xargs clang-format -i

clean:
	rm -rf build

build:
	mkdir -p $@

build/%: %.cc ${OBJ}
	${CPP} ${CPPFLAGS} ${OBJ} -o $@ $< ${LDFLAGS}

${OBJ}: ${MORE_SOURCES}
	${CPP} ${CPPFLAGS} -c ${MORE_SOURCES} -o $@
