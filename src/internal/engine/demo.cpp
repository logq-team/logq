#include "demo.h"

#include <iostream> // for debug purposes
#include <algorithm>
#include <limits>
#include <string_view>

static_assert(__cplusplus == 202002L, "LogQ requires C++20");

int lq_demo_inspect(const char* text, size_t length, int64_t number, lq_demo_result* result) {
    std::cout << "[CPP][DEBUG] " << "Hello from C++:" << __func__ << std::endl;
    if (result == nullptr || (text == nullptr && length != 0)) {
        return LQ_DEMO_INVALID_ARGUMENT;
    }
    *result = {};

    if (number > std::numeric_limits<int64_t>::max() / 2 ||
        number < std::numeric_limits<int64_t>::min() / 2) {
        return LQ_DEMO_OVERFLOW;
    }

    const std::string_view input(text == nullptr ? "" : text, length);
    // C++ writes data to shared C structure, Go should parse it later
    result->bytes = input.size();
    result->newlines = std::count(input.begin(), input.end(), '\n');
    result->doubled = number * 2;
    result->cpp_standard = __cplusplus;
    return LQ_DEMO_OK;
}
