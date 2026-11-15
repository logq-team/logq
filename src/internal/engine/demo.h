#ifndef LOGQ_DEMO_H
#define LOGQ_DEMO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum lq_demo_status {
    LQ_DEMO_OK = 0,
    LQ_DEMO_INVALID_ARGUMENT = 1,
    LQ_DEMO_OVERFLOW = 2
};

// Shared structure for C++ and Go (passed via cgo)
typedef struct {
    uint64_t bytes;
    uint64_t newlines;
    int64_t doubled;
    long cpp_standard;
} lq_demo_result;

/* Reads text[0..length) synchronously without retaining it, result belongs to the caller. 
 * lq_demo_result* result - is output parameter
 */
int lq_demo_inspect(const char* text, size_t length, int64_t number, lq_demo_result* result);

#ifdef __cplusplus
}
#endif

#endif
