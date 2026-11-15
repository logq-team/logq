GO ?= go
CXX ?= g++
PKG_CONFIG ?= pkg-config
OVERVIEW_FILE ?= res/cpp/test.log

.PHONY: all build debug test hooks overview-demo cpp-test

all: build

build:
	mkdir -p build
	CGO_ENABLED=1 CXX="$(CXX)" $(GO) build -o build/logq ./src/cli

debug:
	mkdir -p build
	CGO_ENABLED=1 CXX="$(CXX)" CGO_CFLAGS="-O0 -g3" CGO_CXXFLAGS="-O0 -g3 -fno-omit-frame-pointer" \
		$(GO) build -gcflags='all=-N -l' -ldflags=-compressdwarf=false -o build/logq-debug ./src/cli

test: cpp-test
	CGO_ENABLED=1 CXX="$(CXX)" $(GO) test ./...

cpp-test: build/overview-test
	./build/overview-test

build/overview-test: tests/cpp/overview_test.cpp src/engine/overview.cpp src/engine/overview.h
	@$(PKG_CONFIG) --exists gtest_main || { echo "[CPP][ERROR] Install libgtest-dev and pkg-config to run C++ tests" >&2; exit 1; }
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Wpedantic -I src/engine \
		$(shell $(PKG_CONFIG) --cflags gtest_main) tests/cpp/overview_test.cpp src/engine/overview.cpp \
		-o $@ $(shell $(PKG_CONFIG) --libs gtest_main)

overview-demo: build/overview-demo
	./build/overview-demo "$(OVERVIEW_FILE)"

build/overview-demo: examples/overview/main.cpp src/engine/overview.cpp src/engine/overview.h
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Wpedantic -I src/engine \
		examples/overview/main.cpp src/engine/overview.cpp -o $@

hooks:
	git config core.hooksPath .githooks
