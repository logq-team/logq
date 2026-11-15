GO ?= go
CXX ?= g++
OVERVIEW_FILE ?= res/cpp/test.log

.PHONY: all build debug test hooks overview-demo

all: build

build:
	mkdir -p build
	CGO_ENABLED=1 CXX="$(CXX)" $(GO) build -o build/logq ./src/cli

debug:
	mkdir -p build
	CGO_ENABLED=1 CXX="$(CXX)" CGO_CFLAGS="-O0 -g3" CGO_CXXFLAGS="-O0 -g3 -fno-omit-frame-pointer" \
		$(GO) build -gcflags='all=-N -l' -ldflags=-compressdwarf=false -o build/logq-debug ./src/cli

test:
	CGO_ENABLED=1 CXX="$(CXX)" $(GO) test ./...

overview-demo: build/overview-demo
	./build/overview-demo "$(OVERVIEW_FILE)"

build/overview-demo: examples/overview/main.cpp src/engine/overview.cpp src/engine/overview.h
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Wpedantic -I src/engine \
		examples/overview/main.cpp src/engine/overview.cpp -o $@

hooks:
	git config core.hooksPath .githooks
