package engine

/*
#cgo CPPFLAGS: -I${SRCDIR}/../../engine
#include "lq_overview.h"
*/
import "C"

type OverviewOptions struct {
	Workers uint32
}

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
