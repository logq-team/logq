#ifndef LOGQ_LQ_GREP_H
#define LOGQ_LQ_GREP_H

#include "lq_overview.h"

#ifdef __cplusplus
extern "C" {
#endif

// A literal byte sequence which may contain NUL but not LF
// A zero length pattern matches every existing line and may have null data
typedef struct {
    const char* data;
    size_t length;
} lq_pattern;

typedef struct {
    const char* const* paths;
    size_t file_count;
    const lq_pattern* patterns;
    size_t pattern_count;
    uint32_t workers;
    uint64_t max_matches;
    int32_t count_only;
} lq_grep_request;

typedef struct {
    uint64_t lines;
    uint64_t matching_lines;
    int32_t status;
} lq_grep_file_stats;

typedef struct {
    size_t file_index;
    uint64_t line_number;
    const char* text;
    size_t length;
} lq_grep_match;

typedef struct {
    const lq_grep_file_stats* files;
    size_t file_count;
    const lq_grep_match* matches;
    size_t match_count;
    int32_t truncated;
    void* internal;
} lq_grep_result;

// Contract only with implementation provided by a later task
// Reuse lq_call_status and lq_file_status from lq_overview.h
// Require workers >= 1 and pattern_count >= 1
// count_only must be 0 or 1 and max_matches must be positive unless count_only is 1
// Match case sensitive substrings with OR semantics and count each matching line once
// The caller owns all inputs and keeps the inputs and files unchanged during the call
// Initialize result to zero and free it before reuse without copying its owning handle
// On LQ_CALL_OK files follow input order and matches follow file and line order
// Line numbers start at 1 and text excludes LF but preserves CR and embedded NUL
// Counts remain complete when max_matches limits text across all files
// Set truncated to 1 only when matching lines were omitted because of the limit
// Count only mode returns no matches and truncated is 0
// A failed file has zero counts and no matches while other files remain available
// A call error leaves result empty and C++ exceptions must not cross the C ABI
int32_t lq_grep(const lq_grep_request* request, lq_grep_result* result);

// Release all result storage using the opaque internal handle and reset every field to zero
// Null and zero initialized results are allowed and exceptions must not escape
// Call after success or failure and never free individual fields with C free
void lq_grep_free(lq_grep_result* result);

#ifdef __cplusplus
}
#endif

#endif
