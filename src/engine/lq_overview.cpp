#include "lq_overview.h"
#include "overview.h"

#include <stdexcept>

int32_t lq_overview(
    const lq_overview_request* request,
    lq_file_stats* results,
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

    if (request->workers > 1)
    {
        return LQ_CALL_UNSUPPORTED;
    }

    if (request->file_count == 0)
    {
        return LQ_CALL_OK;
    }

    for (size_t i{}; i < request->file_count; ++i)
    {
        results[i] = {};
        try
        {
            std::ifstream file(request->paths[i], std::ios::binary);
            if (!file.is_open())
            {
                results[i].status = LQ_FILE_OPEN_ERROR;
                continue;
            }

            const file_info fi = analyze_file(file);
            results[i].bytes = fi.bytes;
            results[i].lines = fi.lines;
            results[i].error_lines = fi.error_counter;
            results[i].warn_lines = fi.warn_counter;
            results[i].status = LQ_FILE_OK;
        }
        catch (const std::runtime_error&)
        {
            results[i].status = LQ_FILE_READ_ERROR;
        }
        catch (...)
        {
            results[i].status = LQ_FILE_INTERNAL_ERROR;
        }
    }
    return LQ_CALL_OK;
}
