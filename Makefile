GO ?= go
CXX ?= g++
GDB ?= gdb
PKG_CONFIG ?= pkg-config
OVERVIEW_FILE ?= res/cpp/test.log
CXX_DEBUG_FLAGS = -O0 -g3 -fno-omit-frame-pointer

.PHONY: all build debug test hooks overview-demo cpp-test overview-api-demo
.PHONY: debug-demo overview-demo-debug overview-api-demo-debug

all: build

build:
	mkdir -p build
	CGO_ENABLED=1 CXX="$(CXX)" $(GO) build -o build/logq ./src/cli

debug:
	mkdir -p build
	CGO_ENABLED=1 CXX="$(CXX)" CGO_CFLAGS="-O0 -g3" CGO_CXXFLAGS="-O0 -g3 -fno-omit-frame-pointer" \
		$(GO) build -gcflags='all=-N -l' -ldflags=-compressdwarf=false -o build/logq-debug ./src/cli

debug-demo: debug
	$(GDB) -q -x scripts/debug-demo.gdb

test: cpp-test
	CGO_ENABLED=1 CXX="$(CXX)" $(GO) test ./...

cpp-test: build/overview-test
	./build/overview-test

build/overview-test: tests/cpp/overview_test.cpp src/engine/overview.cpp src/engine/overview.h src/engine/lq_overview.cpp src/engine/lq_overview.h
	@$(PKG_CONFIG) --exists gtest_main || { echo "[CPP][ERROR] Install libgtest-dev and pkg-config to run C++ tests" >&2; exit 1; }
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -fopenmp -Wall -Wextra -Wpedantic -I src/engine \
		$(shell $(PKG_CONFIG) --cflags gtest_main) tests/cpp/overview_test.cpp src/engine/overview.cpp src/engine/lq_overview.cpp \
		-o $@ $(shell $(PKG_CONFIG) --libs gtest_main)

overview-demo: build/overview-demo
	./build/overview-demo "$(OVERVIEW_FILE)"

build/overview-demo: examples/overview/main.cpp src/engine/overview.cpp src/engine/overview.h
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Wpedantic -I src/engine \
		examples/overview/main.cpp src/engine/overview.cpp -o $@

overview-demo-debug: build/overview-demo-debug
	$(GDB) -q -ex 'break main' -ex 'break analyze_file' -ex run \
		--args ./build/overview-demo-debug "$(OVERVIEW_FILE)"

build/overview-demo-debug: examples/overview/main.cpp src/engine/overview.cpp src/engine/overview.h
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CXX_DEBUG_FLAGS) -std=c++20 -Wall -Wextra -Wpedantic -I src/engine \
		examples/overview/main.cpp src/engine/overview.cpp -o $@

overview-api-demo: build/overview-api-demo
	./build/overview-api-demo

build/overview-api-demo: examples/overview_api/main.cpp src/engine/lq_overview.cpp src/engine/lq_overview.h src/engine/overview.cpp src/engine/overview.h
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -fopenmp -Wall -Wextra -Wpedantic -I src/engine \
		examples/overview_api/main.cpp src/engine/lq_overview.cpp src/engine/overview.cpp -o $@

overview-api-demo-debug: build/overview-api-demo-debug
	$(GDB) -q -ex 'break main' -ex 'break lq_overview' -ex run \
		--args ./build/overview-api-demo-debug

build/overview-api-demo-debug: examples/overview_api/main.cpp src/engine/lq_overview.cpp src/engine/lq_overview.h src/engine/overview.cpp src/engine/overview.h
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CXX_DEBUG_FLAGS) -std=c++20 -fopenmp -Wall -Wextra -Wpedantic -I src/engine \
		examples/overview_api/main.cpp src/engine/lq_overview.cpp src/engine/overview.cpp -o $@

hooks:
	git config core.hooksPath .githooks
