#include "demo.h"

#include "log.h" // for debug purposes
#include <algorithm>
#include <cstdio>
#include <limits>
#include <string>

static_assert(__cplusplus >= 202302L, "LogQ requires C++23 or newer");

using std::fflush;
using std::count;
using std::string;
using int64_limits = std::numeric_limits<int64_t>;

int lq_demo_inspect(const char* text, size_t length, int64_t number, lq_demo_result* result)
{
    DEBUG("Hello from C++:{}", __func__);
    fflush(stdout);
    if (result == nullptr || (text == nullptr && length != 0))
    {
        return LQ_DEMO_INVALID_ARGUMENT;
    }
    *result = {};

    if (number > int64_limits::max() / 2 ||
        number < int64_limits::min() / 2)
    {
        return LQ_DEMO_OVERFLOW;
    }

    const string input(text == nullptr ? "" : text, length);
    // C++ writes data to shared C structure, Go should parse it later
    result->bytes = input.size();
    result->newlines = count(input.begin(), input.end(), '\n');
    result->doubled = number * 2;
    result->cpp_standard = __cplusplus;
    return LQ_DEMO_OK;
}
