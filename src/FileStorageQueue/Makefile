.PHONY: test fulltest all indent clean check coverage

CPLUSPLUS=g++
CPPFLAGS=-std=c++11 -Wall -W
LDFLAGS=-pthread
CPPFLAGS_FOR_COVERAGE=${CPPFLAGS} -O0 -g -fprofile-arcs -ftest-coverage
LDFLAGS_FOR_COVERAGE=${LDFLAGS}

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=build/%)
BIN_FOR_COVERAGE=$(SRC:%.cc=build/coverage/%)

LATENCY_TEST=./build/test_retry_strategy --verbose=false --gtest_break_on_failure --gtest_repeat=50

test: all
	./build/test

fulltest: test
	${LATENCY_TEST} --number_of_failures=1 --mean_ms=250 --min_ms=0 --max_ms=1000 --p25_max=150 --p75_min=250 && \
	${LATENCY_TEST} --number_of_failures=3 --mean_ms=50 --min_ms=0 --max_ms=500 --p25_max=120 --p75_min=150 && \
	${LATENCY_TEST} --number_of_failures=10 --mean_ms=50 --min_ms=0 --max_ms=500 --p25_max=450 --p75_min=550 && \
	echo "ALL TESTS PASSED"

all: build ${BIN}

indent:
	(find . -name "*.cc" ; find . -name "*.h") | xargs clang-format-3.5 -i

clean:
	rm -rf build

check: build build/CHECK_OK

build/CHECK_OK: build *.h
	for i in *.h ; do \
		echo -n $(basename $$i)': ' ; \
		ln -sf ${PWD}/$$i ${PWD}/build/$$i.cc ; \
		if [ ! -f build/$$i.h.o -o build/$$i.h.cc -nt build/$$i.h.o ] ; then \
			${CPLUSPLUS} -I . ${CPPFLAGS} -c build/$$i.cc -o build/$$i.h.o ${LDFLAGS} || exit 1 ; echo 'OK' ; \
		else \
			echo 'Already OK' ; \
		fi \
	done && echo OK >$@

build:
	mkdir -p build

build/%: %.cc *.h
	${CPLUSPLUS} ${CPPFLAGS} -o $@ $< ${LDFLAGS}

build/coverage:
	mkdir -p $@

build/coverage/%: %.cc *.h
	${CPLUSPLUS} ${CPPFLAGS_FOR_COVERAGE} -o $@ $< ${LDFLAGS_FOR_COVERAGE}

coverage: build/coverage ${BIN_FOR_COVERAGE}
	./build/coverage/test
	gcov test.cc
	geninfo . --output-file coverage.info
	genhtml coverage.info --output-directory build/coverage | grep -A 2 "^Overall"
	rm -rf coverage.info *.gcov *.gcda *.gcno
	echo ${PWD}/build/coverage/index.html
