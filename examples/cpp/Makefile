.PHONY: run all indent clean

CPP=g++
CPPFLAGS=-std=c++11 -Wall -W
LDFLAGS=-pthread -lz

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=build/%)
OS := $(shell uname)

MORE_SOURCES = ../../src/cpp/alohalytics.cc
ifeq ($(OS),Darwin)
  CPPFLAGS += -x objective-c++ -fobjc-arc
  LDFLAGS += -framework Foundation
  MORE_SOURCES += ../../src/apple/http_client_apple.mm
endif

run: all
	./build/example --help
	echo "Or spawn a local server or set ALOHA_SERVER and then run ./test.sh"

all: build ${BIN}

indent:
	(find . ../../src/cpp ../../src/posix -name "*.cc" ; find ../../src -name "*.h" -maxdepth 0) | xargs clang-format-3.5 -i

clean:
	rm -rf build

build:
	mkdir -p $@

build/%: %.cc
	${CPP} ${CPPFLAGS} -o $@ $< ${MORE_SOURCES}  ${LDFLAGS}
