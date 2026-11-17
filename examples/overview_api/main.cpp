#include "lq_overview.h"
#include "log.h"

#include <iterator>

using std::size;

int main()
{
    INFO("Batch overview through the C API without Go including a missing file");

    const char* paths[] = {
        "res/cpp/test.log",
        "res/cpp/nonexistent-api-demo.log",
        "res/cpp/test.log"
    };

    lq_overview_request request = {
        .paths = paths,
        .file_count = size(paths),
        .workers = 1
    };
    lq_file_stats results[size(paths)] = {};

    INFO("file_count = {}", request.file_count);
    INFO("workers = {}", request.workers);
    INFO("result_capacity = {}", size(results));
    for (size_t i = 0; i < request.file_count; ++i)
    {
        INFO("paths[{}] = {}", i, request.paths[i]);
    }

    const int32_t status = lq_overview(&request, results, size(results));
    if (status == LQ_CALL_OK)
    {
        int exit_code = 0;
        for (size_t i = 0; i < request.file_count; ++i)
        {
            if (results[i].status != LQ_FILE_OK)
            {
                ERROR("results[{}] {}: file_status = {}",
                        i, request.paths[i], results[i].status);
                exit_code = 1;
                continue;
            }

            INFO("results[{}] {}", i, request.paths[i]);
            INFO("bytes = {}", results[i].bytes);
            INFO("lines = {}", results[i].lines);
            INFO("error_lines = {}", results[i].error_lines);
            INFO("warn_lines = {}", results[i].warn_lines);
        }
        return exit_code;
    }

    if (status == LQ_CALL_UNSUPPORTED)
    {
        INFO("lq_overview returned LQ_CALL_UNSUPPORTED");
        INFO("Requested processing mode is unsupported");
        return 0;
    }

    ERROR("Unexpected call status = {}", status);
    return 1;
}
