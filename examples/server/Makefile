.PHONY: run all indent clean

CPP=g++
CPPFLAGS=-std=c++11 -Wall -W -I../..
LDFLAGS=-lz -pthread
MORE_SOURCES=../../src/posix/file_manager_posix_impl.cc

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=build/%)
OS := $(shell uname)

run: all
	./build/demo

all: build ${BIN}

indent:
	(find . -name "*.cc") | xargs clang-format -i

clean:
	rm -rf build

build:
	mkdir -p $@

build/%: %.cc
	${CPP} ${CPPFLAGS} -o $@ $< ${MORE_SOURCES} ${LDFLAGS}
