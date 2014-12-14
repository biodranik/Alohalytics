.PHONY: test all indent clean

CPP=g++
CPPFLAGS=-std=c++11 -Wall -W
LDFLAGS=-pthread

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=build/%)
OS := $(shell uname)

ifeq ($(OS),Darwin)
  CPPFLAGS += -x objective-c++ -fobjc-arc
  LDFLAGS += -framework Foundation
endif

test: all
	./build/aloha_stats_test

all: build ${BIN}

indent:
	(find . ../../src -name "*.cc" ; find . ../../src -name "*.h") | xargs clang-format-3.5 -i

clean:
	rm -rf build

build:
	mkdir -p $@

build/%: %.cc
	${CPP} ${CPPFLAGS} -o $@ $< ${LDFLAGS}
