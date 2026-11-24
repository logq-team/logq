#include "overview.h"
#include "log.h"

#include <charconv>
#include <exception>
#include <fstream>
#include <string_view>

using std::errc;
using std::from_chars;
using std::ios;
using std::exception;
using std::ifstream;
using std::streamoff;
using std::string_view;

namespace {

bool parse_offset(const char* argument, streamoff& offset)
{
    const string_view text = argument;
    const auto result = from_chars(text.data(), text.data() + text.size(), offset);
    return result.ec == errc{} && result.ptr == text.data() + text.size() && offset >= 0;
}

}

int main(int argc, char* argv[])
{
    INFO("Sequential overview of one file without the C API or Go");

    if (argc != 2 && argc != 4)
    {
        ERROR("Usage: overview-demo FILE [BEGIN END]");
        return 2;
    }

    streamoff begin = 0;
    streamoff end = 0;
    if (argc == 4)
    {
        if (!parse_offset(argv[2], begin) || !parse_offset(argv[3], end) || end < begin)
        {
            ERROR("BEGIN and END must be nonnegative byte offsets with BEGIN <= END");
            return 2;
        }
        INFO("Count whole lines starting in byte range [{}, {})", begin, end);
    }

    ifstream file(argv[1], ios::binary);
    if (!file.is_open())
    {
        ERROR("can't open file: {}", argv[1]);
        return 1;
    }

    try
    {
        const file_info stats = argc == 4
            ? analyze_file_range(file, begin, end)
            : analyze_file(file);
        INFO("bytes = {}", stats.bytes);
        INFO("lines = {}", stats.lines);
        INFO("error_counter = {}", stats.error_counter);
        INFO("warn_counter = {}", stats.warn_counter);
    }
    catch (const exception& error)
    {
        ERROR("{}: {}", argv[1], error.what());
        return 1;
    }
    return 0;
}
