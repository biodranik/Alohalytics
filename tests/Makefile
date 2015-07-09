.PHONY: run all indent clean

CPP=g++
CPPFLAGS=-std=c++11 -Wall -W -Igoogletest/include -Igoogletest -Wno-missing-field-initializers
LDFLAGS=-lz -pthread

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
OBJ=$(wildcard build/obj/*.obj)
MORE_SRC=../src/posix/file_manager_posix_impl.cc googletest/src/gtest-all.cc googletest/src/gtest_main.cc
BIN=build/tests
OS := $(shell uname)

run: all
	${BIN}

all: build build/obj ${BIN}

indent:
	(ls -1 *.cc *.h) | xargs clang-format -i

clean:
	rm -rf build

build:
	mkdir -p $@

build/obj:
	mkdir -p $@

${BIN}: ${SRC} ${OBJ}
	${CPP} ${CPPFLAGS} -o $@ $^ ${MORE_SRC} ${LDFLAGS}
