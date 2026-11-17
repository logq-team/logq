#include "overview.h"
#include "log.h"

#include <exception>
#include <fstream>

using std::ios;
using std::exception;
using std::ifstream;

int main(int argc, char* argv[])
{
    INFO("Sequential overview of one file without the C API or Go");

    if (argc != 2)
    {
        ERROR("Usage: overview-demo FILE");
        return 2;
    }

    ifstream file(argv[1], ios::binary);
    if (!file.is_open())
    {
        ERROR("can't open file: {}", argv[1]);
        return 1;
    }

    try
    {
        const file_info stats = analyze_file(file);
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
