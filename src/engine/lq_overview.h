#ifndef LOGQ_LQ_OVERVIEW_H
#define LOGQ_LQ_OVERVIEW_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum lq_call_status {
    LQ_CALL_OK = 0,
    LQ_CALL_INVALID_ARGUMENT = 1,
    LQ_CALL_UNSUPPORTED = 2,
    LQ_CALL_INTERNAL_ERROR = 3
};

enum lq_file_status {
    LQ_FILE_OK = 0,
    LQ_FILE_OPEN_ERROR = 1,
    LQ_FILE_READ_ERROR = 2,
    LQ_FILE_INTERNAL_ERROR = 3
};

typedef struct {
    const char* const* paths;
    size_t file_count;
    uint32_t workers;
} lq_overview_request;

typedef struct {
    uint64_t bytes;
    uint64_t lines;
    uint64_t error_lines;
    uint64_t warn_lines;
    int32_t status;
} lq_file_stats;

// Each result corresponds to the input path at the same index
// All pointers remain owned by the caller and are used only during the call
int32_t lq_overview(
    const lq_overview_request* request,
    lq_file_stats* results,
    size_t result_capacity
);

#ifdef __cplusplus
}
#endif

#endif
