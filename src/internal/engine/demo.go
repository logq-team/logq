package engine

/*
#cgo CXXFLAGS: -std=c++20 -Wall -Wextra -Wpedantic
#cgo LDFLAGS: -lstdc++
#include <stdlib.h>
#include "demo.h"
*/
import "C"

import (
	"errors"
	"fmt"
	"unsafe"
)

var ErrOverflow = errors.New("C++ demo: doubling the number would overflow int64")

// Go specific structure (mirror for lq_demo_result in demo.h)
type DemoResult struct {
	Bytes       uint64
	Newlines    uint64
	Doubled     int64
	CPPStandard int64
}

// Demo demonstrates the Go/C++ boundary, it is not the final scanning API.
// It is called from main.go
func Demo(text string, number int64) (DemoResult, error) {
	input := C.CString(text)
	defer C.free(unsafe.Pointer(input))

	var result C.lq_demo_result // this is C structure from demo.h
	// go passes this structure addres to C++
	status := C.lq_demo_inspect(input, C.size_t(len(text)), C.int64_t(number), &result)
	// Now we have result (C structure), and we can build DemoResult for Go
	// based on result data
	switch status {
	case C.LQ_DEMO_OK:
		return DemoResult{
			Bytes:       uint64(result.bytes),
			Newlines:    uint64(result.newlines),
			Doubled:     int64(result.doubled),
			CPPStandard: int64(result.cpp_standard),
		}, nil
	case C.LQ_DEMO_OVERFLOW:
		return DemoResult{}, ErrOverflow
	default:
		return DemoResult{}, fmt.Errorf("C++ demo failed with status %d", int(status))
	}
}
