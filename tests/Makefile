.PHONY: run all indent clean

CPP=g++
CPPFLAGS=-std=c++11 -Wall -W
LDFLAGS=-lz -pthread
MORE_SOURCES=../src/posix/file_manager_posix_impl.cc

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=build/%)
OS := $(shell uname)

run: all
	./build/test_location
	./build/test_gzip
	./build/test_messages_queue
	./build/test_file_manager

all: build ${BIN}

indent:
	(find . -name "*.cc" -o -name "*.h") | xargs clang-format -i

clean:
	rm -rf build

build:
	mkdir -p $@

build/%: %.cc
	${CPP} ${CPPFLAGS} -o $@ $< ${MORE_SOURCES} ${LDFLAGS}
