#include "lq_overview.h"
#include "overview.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace
{
int32_t validate_request(
    const lq_overview_request* request,
    const lq_file_stats* results,
    size_t result_capacity
)
{
    if (request == nullptr)
    {
        return LQ_CALL_INVALID_ARGUMENT;
    }

    if (request->workers == 0 || result_capacity < request->file_count)
    {
        return LQ_CALL_INVALID_ARGUMENT;
    }

    if (request->file_count > 0 && (request->paths == nullptr || results == nullptr))
    {
        return LQ_CALL_INVALID_ARGUMENT;
    }

    for (size_t i{}; i < request->file_count; ++i)
    {
        if (request->paths[i] == nullptr)
        {
            return LQ_CALL_INVALID_ARGUMENT;
        }
    }

    return LQ_CALL_OK;
}

void process_file(const char* path, lq_file_stats& result)
{
    result = {};
    try
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            result.status = LQ_FILE_OPEN_ERROR;
            return;
        }

        const file_info fi = analyze_file(file);
        result.bytes = fi.bytes;
        result.lines = fi.lines;
        result.error_lines = fi.error_counter;
        result.warn_lines = fi.warn_counter;
        result.status = LQ_FILE_OK;
    }
    catch (const std::runtime_error&)
    {
        result.status = LQ_FILE_READ_ERROR;
    }
    catch (...)
    {
        result.status = LQ_FILE_INTERNAL_ERROR;
    }
}

int32_t process_files_sequential(const lq_overview_request* request, lq_file_stats* results)
{
    for (size_t i{}; i < request->file_count; ++i)
    {
        process_file(request->paths[i], results[i]);
    }
    return LQ_CALL_OK;
}

int32_t process_files_parallel(const lq_overview_request* request, lq_file_stats* results)
{
    if (request->file_count == 0)
    {
        return LQ_CALL_OK;
    }

    // Limit requested threads to the file count and the OpenMP int range
    const int worker_count = static_cast<int>(std::min<size_t>({
        request->workers,
        request->file_count,
        std::numeric_limits<int>::max()
    }));

    // Assign files to threads round robin with one file per chunk
    // With 3 threads and 9 files
    // Thread 0 gets files 0 3 6
    // Thread 1 gets files 1 4 7
    // Thread 2 gets files 2 5 8
    // The OS schedules these threads on CPU cores
    #pragma omp parallel for num_threads(worker_count) schedule(static, 1)
    for (size_t i = 0; i < request->file_count; ++i)
    {
        process_file(request->paths[i], results[i]);
    }
    return LQ_CALL_OK;
}
}

int32_t lq_overview(
    const lq_overview_request* request,
    lq_file_stats* results,
    size_t result_capacity
)
{
    const int32_t status = validate_request(request, results, result_capacity);
    if (status != LQ_CALL_OK)
    {
        return status;
    }

    if (request->workers == 1)
    {
        return process_files_sequential(request, results);
    }

    return process_files_parallel(request, results);
}
