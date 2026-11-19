package cppengine

/*
#cgo CXXFLAGS: -std=c++23 -fopenmp  -Wall -Wextra -Wpedantic
#cgo LDFLAGS: -lstdc++ -lgomp
#include "lq_overview.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"runtime"
	"unsafe"
)

type FileStatus int32

const (
	FileOK            FileStatus = C.LQ_FILE_OK
	FileOpenError     FileStatus = C.LQ_FILE_OPEN_ERROR
	FileReadError     FileStatus = C.LQ_FILE_READ_ERROR
	FileInternalError FileStatus = C.LQ_FILE_INTERNAL_ERROR
)

type FileStats struct {
	Bytes      uint64
	Lines      uint64
	ErrorLines uint64
	WarnLines  uint64
	Status     FileStatus
}

func OverviewCall(fileNames []string, workerThreadNum int) ([]FileStats, error) {
	if len(fileNames) == 0 {
		return []FileStats{}, nil
	}

	cPaths := make([]*C.char, len(fileNames))
	for i, p := range fileNames {
		cPaths[i] = C.CString(p)
	}
	defer func() {
		for _, p := range cPaths {
			C.free(unsafe.Pointer(p))
		}
	}()

	req := C.lq_overview_request{
		paths:      &cPaths[0],
		file_count: C.size_t(len(fileNames)),
		workers:    C.uint32_t(workerThreadNum),
	}

	results := make([]C.lq_file_stats, len(fileNames))

	var pinner runtime.Pinner
	defer pinner.Unpin()

	pinner.Pin(&results[0])
	pinner.Pin(&req)
	pinner.Pin(&cPaths[0])

	status := C.lq_overview(&req, &results[0], C.size_t(len(fileNames)))

	if status != C.LQ_CALL_OK {
		var outErr error
		switch status {
		case C.LQ_CALL_INVALID_ARGUMENT:
			outErr = errors.New("Processing failed: invalid argument")
			// TODO ?????? Unsupported
		case C.LQ_CALL_UNSUPPORTED:
			outErr = errors.New("Processing failed: unsupported")
		case C.LQ_CALL_INTERNAL_ERROR:
			outErr = errors.New("Processing failed: internal error")
		}
		return nil, outErr
	}

	out := make([]FileStats, len(fileNames))
	for i, r := range results {
		out[i] = FileStats{
			Bytes:      uint64(r.bytes),
			Lines:      uint64(r.lines),
			ErrorLines: uint64(r.error_lines),
			WarnLines:  uint64(r.warn_lines),
			Status:     FileStatus(r.status),
		}
	}

	return out, nil
}
