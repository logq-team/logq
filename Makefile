GO ?= go
CXX ?= g++

.PHONY: all build debug test hooks

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

hooks:
	git config core.hooksPath .githooks
