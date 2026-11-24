#include "lq_overview.h"
#include "overview.h"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <vector>

using std::error_code;
using std::filesystem::file_size;
using std::ios;
using std::ifstream;
using std::min;
using std::runtime_error;
using std::streamoff;
using std::vector;
using int_limits = std::numeric_limits<int>;
using stream_limits = std::numeric_limits<streamoff>;

namespace
{
constexpr streamoff range_size = 8 * 1024 * 1024;

struct file_task {
    size_t file_index;
    streamoff begin;
    streamoff end;
    bool whole_file;
};

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

    for (size_t i = 0; i < request->file_count; ++i)
    {
        if (request->paths[i] == nullptr)
        {
            return LQ_CALL_INVALID_ARGUMENT;
        }
    }

    return LQ_CALL_OK;
}

void process_file(const char* path, lq_file_stats& result, const file_task* task = nullptr)
{
    result = {};
    try
    {
        ifstream file(path, ios::binary);
        if (!file.is_open())
        {
            result.status = LQ_FILE_OPEN_ERROR;
            return;
        }

        const file_info stats = task == nullptr || task->whole_file
            ? analyze_file(file)
            : analyze_file_range(file, task->begin, task->end);
        result.bytes = stats.bytes;
        result.lines = stats.lines;
        result.error_lines = stats.error_counter;
        result.warn_lines = stats.warn_counter;
        result.status = LQ_FILE_OK;
    }
    catch (const runtime_error&)
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
    for (size_t i = 0; i < request->file_count; ++i)
    {
        process_file(request->paths[i], results[i]);
    }
    return LQ_CALL_OK;
}

vector<file_task> make_file_tasks(const lq_overview_request* request)
{
    vector<file_task> tasks;
    for (size_t i = 0; i < request->file_count; ++i)
    {
        error_code error;
        const auto size = file_size(request->paths[i], error);
        // Leave small files and paths with unknown sizes to the whole file reader
        if (error || size <= range_size || size > stream_limits::max())
        {
            tasks.push_back({i, 0, 0, true});
            continue;
        }

        const streamoff length = static_cast<streamoff>(size);
        for (streamoff begin = 0; begin < length;)
        {
            const streamoff end = begin + min(range_size, length - begin);
            tasks.push_back({i, begin, end, false});
            begin = end;
        }
    }
    return tasks;
}

void merge_file_stats(lq_file_stats& total, const lq_file_stats& part)
{
    if (total.status != LQ_FILE_OK)
    {
        return;
    }
    // A failed range invalidates statistics for its entire file
    if (part.status != LQ_FILE_OK)
    {
        total = {};
        total.status = part.status;
        return;
    }
    total.bytes += part.bytes;
    total.lines += part.lines;
    total.error_lines += part.error_lines;
    total.warn_lines += part.warn_lines;
}

int32_t process_files_parallel(const lq_overview_request* request, lq_file_stats* results)
{
    const vector<file_task> tasks = make_file_tasks(request);
    if (tasks.empty())
    {
        return LQ_CALL_OK;
    }

    vector<lq_file_stats> partial_results(tasks.size());
    // Limit requested threads to the task count and the OpenMP int range
    const int worker_count = static_cast<int>(min<size_t>({
        request->workers,
        tasks.size(),
        int_limits::max()
    }));

    // Each free thread takes one task which is a whole file or a byte range
    // Each task writes to its own result without shared file counters
    // The OS schedules these threads on CPU cores
    #pragma omp parallel for num_threads(worker_count) schedule(dynamic, 1)
    for (size_t i = 0; i < tasks.size(); ++i)
    {
        const file_task& task = tasks[i];
        process_file(request->paths[task.file_index], partial_results[i], &task);
    }

    // The parallel loop has finished before merging results in input file order
    for (size_t i = 0; i < request->file_count; ++i)
    {
        results[i] = {};
    }
    for (size_t i = 0; i < tasks.size(); ++i)
    {
        merge_file_stats(results[tasks[i].file_index], partial_results[i]);
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

    try
    {
        if (request->workers == 1)
        {
            return process_files_sequential(request, results);
        }

        return process_files_parallel(request, results);
    }
    catch (...)
    {
        // Task allocation failures must not cross the C ABI
        return LQ_CALL_INTERNAL_ERROR;
    }
}
