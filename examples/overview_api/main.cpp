#include "lq_overview.h"

#include <iostream>
#include <iterator>
#include <string>

static constexpr std::string info_prefix{"[CPP][INFO] "};
static constexpr std::string error_prefix{"[CPP][ERROR] "};

int main()
{
    const char* paths[]{
        "res/cpp/test.log",
        "res/cpp/nonexistent-api-demo.log",
        "res/cpp/test.log"
    };

    lq_overview_request request{
        .paths = paths,
        .file_count = std::size(paths),
        .workers = 1
    };
    lq_file_stats results[std::size(paths)]{};

    std::cout << info_prefix << "file_count = " << request.file_count << '\n'
              << info_prefix << "workers = " << request.workers << '\n'
              << info_prefix << "result_capacity = " << std::size(results) << '\n';
    for (std::size_t i{}; i < request.file_count; ++i)
    {
        std::cout << info_prefix << "paths[" << i << "] = " << request.paths[i] << '\n';
    }

    const int32_t status = lq_overview(&request, results, std::size(results));
    if (status == LQ_CALL_OK)
    {
        int exit_code{};
        for (std::size_t i{}; i < request.file_count; ++i)
        {
            if (results[i].status != LQ_FILE_OK)
            {
                std::cerr << error_prefix << "results[" << i << "] " << request.paths[i]
                          << ": file_status = " << results[i].status << '\n';
                exit_code = 1;
                continue;
            }

            std::cout << info_prefix << "results[" << i << "] " << request.paths[i] << '\n'
                      << info_prefix << "bytes = " << results[i].bytes << '\n'
                      << info_prefix << "lines = " << results[i].lines << '\n'
                      << info_prefix << "error_lines = " << results[i].error_lines << '\n'
                      << info_prefix << "warn_lines = " << results[i].warn_lines << '\n';
        }
        return exit_code;
    }

    if (status == LQ_CALL_UNSUPPORTED)
    {
        std::cout << info_prefix << "lq_overview returned LQ_CALL_UNSUPPORTED\n"
                  << info_prefix << "Only one worker is supported for now\n";
        return 0;
    }

    std::cerr << error_prefix << "Unexpected call status = " << status << '\n';
    return 1;
}
