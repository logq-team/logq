#include "overview.h"

#include <exception>
#include <fstream>
#include <iostream>

static constexpr std::string info_prefix{"[CPP][INFO] "};
static constexpr std::string error_prefix{"[CPP][ERROR] "};

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << error_prefix << "Usage: overview-demo FILE\n";
        return 2;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << error_prefix << "can't open file: " << argv[1] << '\n';
        return 1;
    }

    try
    {
        const file_info fi = analyze_file(file);
        std::cout << info_prefix << "bytes = " << fi.bytes << '\n'
                  << info_prefix << "lines = " << fi.lines << '\n'
                  << info_prefix << "error_counter = " << fi.error_counter << '\n'
                  << info_prefix << "warn_counter = " << fi.warn_counter
                  << '\n';
    }
    catch (const std::exception& error)
    {
        std::cerr << error_prefix << argv[1] << ": " << error.what() << '\n';
        return 1;
    }
    return 0;
}
