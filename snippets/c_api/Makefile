.PHONY: all indent clean

ALOHALYTICS_ROOT=../../Alohalytics

C=gcc
CPP=g++
CFLAGS=-Wall -Werror -O3
CPPFLAGS=-std=c++11 ${CFLAGS} -I${ALOHALYTICS_ROOT}/src -c
OBJ=build/*.o
LDFLAGS=-shared

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=build/%)
OS := $(shell uname)

all: build ${BIN}

indent:
	(find . -name "*.cc" -o -name "*.h") | xargs clang-format -i

clean:
	rm -rf build

build:
	mkdir -p $@

build/%: ${OBJ}
	${CPP} -shared ${OBJ} -o $@.so

${OBJ}: *.cc
	${CPP} ${CPPFLAGS} -fpic *.cc -o $@
