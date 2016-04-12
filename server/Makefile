.PHONY: run all indent clean

CPP=g++
CPPFLAGS=-std=c++11 -Wall -W -I${INCLUDES} -O3
LDFLAGS=-L${LIBS} -lz -pthread -lfcgi -lfcgi++
MORE_SOURCES=../src/posix/file_manager_posix_impl.cc

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=build/%)
OBJ=build/file_manager_posix_impl.obj
OS := $(shell uname)
ifeq ($(OS),Darwin)
  INCLUDES=/usr/local/Cellar/fcgi/2.4.0/include
  LIBS=/usr/local/Cellar/fcgi/2.4.0/lib
else
  ifeq ($(OS),Linux)
    INCLUDES=/usr/include
    LIBS=/usr/lib
  endif
endif

run: all
	spawn-fcgi -n -p 8888 -- build/fcgi_server /tmp/ /monitoring

all: build ${BIN}

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
