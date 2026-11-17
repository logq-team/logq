#include "lq_overview.h"
#include "log.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <string_view>
#include <vector>

using std::errc;
using std::fill;
using std::find;
using std::from_chars;
using std::sort;
using std::chrono::steady_clock;
using std::string_view;
using std::vector;
using elapsed_milliseconds = std::chrono::duration<double, std::milli>;

namespace
{
constexpr size_t repeat_count = 7;

bool parse_worker_counts(string_view remaining, vector<uint32_t>& worker_counts)
{
    // This is needed for "BENCH_WORKERS=1,2,3,4" feature to run different workers
    while (true)
    {
        const size_t comma = remaining.find(',');
        const string_view argument = remaining.substr(0, comma);
        uint32_t workers = 0;
        const auto parsed = from_chars(argument.data(), argument.data() + argument.size(), workers);
        if (parsed.ec != errc{} || parsed.ptr != argument.data() + argument.size() || workers == 0)
        {
            ERROR("WORKERS must be comma-separated positive uint32 values");
            return false;
        }

        if (find(worker_counts.begin(), worker_counts.end(), workers) == worker_counts.end())
        {
            worker_counts.push_back(workers);
        }
        if (comma == string_view::npos)
        {
            break;
        }
        remaining.remove_prefix(comma + 1);
    }
    return true;
}

bool collect_reference(
    const lq_overview_request& request,
    vector<lq_file_stats>& reference,
    lq_file_stats& totals
)
{
    lq_overview_request sequential_request = request;
    sequential_request.workers = 1;
    const int32_t status = lq_overview(&sequential_request, reference.data(), reference.size());
    if (status != LQ_CALL_OK)
    {
        ERROR("Sequential reference returned call_status = {}", status);
        return false;
    }

    for (size_t i = 0; i < reference.size(); ++i)
    {
        const lq_file_stats& result = reference[i];
        if (result.status != LQ_FILE_OK)
        {
            ERROR("{}: file_status = {}", request.paths[i], result.status);
            return false;
        }
        totals.bytes += result.bytes;
        totals.lines += result.lines;
        totals.error_lines += result.error_lines;
        totals.warn_lines += result.warn_lines;
    }
    return true;
}

bool results_match(
    const lq_overview_request& request,
    const vector<lq_file_stats>& results,
    const vector<lq_file_stats>& reference
)
{
    for (size_t i = 0; i < results.size(); ++i)
    {
        const lq_file_stats& result = results[i];
        const lq_file_stats& expected = reference[i];
        if (result.bytes != expected.bytes || result.lines != expected.lines
            || result.error_lines != expected.error_lines || result.warn_lines != expected.warn_lines
            || result.status != expected.status)
        {
            ERROR("workers = {} sequential result mismatch at index {}: {}",
                    request.workers, i, request.paths[i]);
            return false;
        }
    }
    return true;
}

bool run_benchmark(
    lq_overview_request request,
    const vector<uint32_t>& worker_counts,
    const vector<lq_file_stats>& reference,
    vector<vector<double>>& timings
)
{
    vector<lq_file_stats> results(request.file_count);
    for (vector<double>& samples : timings)
    {
        samples.reserve(repeat_count);
    }

    // Round zero warms every mode and does not contribute to the timings
    for (size_t round = 0; round <= repeat_count; ++round)
    {
        for (size_t offset = 0; offset < worker_counts.size(); ++offset)
        {
            const size_t mode = (offset + round) % worker_counts.size();
            request.workers = worker_counts[mode];
            fill(results.begin(), results.end(), lq_file_stats{0, 0, 0, 0, LQ_FILE_INTERNAL_ERROR});

            const auto start = steady_clock::now();
            const int32_t status = lq_overview(&request, results.data(), results.size());
            const auto finish = steady_clock::now();

            if (status != LQ_CALL_OK)
            {
                ERROR("workers = {} lq_overview returned call_status = {}",
                        request.workers, status);
                return false;
            }
            if (!results_match(request, results, reference))
            {
                return false;
            }

            if (round > 0)
            {
                const elapsed_milliseconds elapsed = finish - start;
                timings[mode].push_back(elapsed.count());
            }
        }
    }

    for (vector<double>& samples : timings)
    {
        sort(samples.begin(), samples.end());
    }
    return true;
}

void print_report(
    size_t file_count,
    const lq_file_stats& totals,
    const vector<uint32_t>& worker_counts,
    const vector<vector<double>>& timings
)
{
    INFO("Per-file results match the sequential reference");
    INFO("files = {}", file_count);
    INFO("bytes = {}", totals.bytes);
    INFO("lines = {}", totals.lines);
    INFO("error_lines = {}", totals.error_lines);
    INFO("warn_lines = {}", totals.warn_lines);
    INFO("warmups_per_mode = 1");
    INFO("repeats_per_mode = {}", repeat_count);
    INFO("Warm file cache with rotating mode order");
    INFO("WORKERS   MEDIAN_MS      MIN_MS      MAX_MS   SPEEDUP");

    const double sequential_ms = timings[0][repeat_count / 2];
    for (size_t mode = 0; mode < worker_counts.size(); ++mode)
    {
        const vector<double>& samples = timings[mode];
        const double median_ms = samples[repeat_count / 2];
        INFO("{:>7}{:>12.3f}{:>12.3f}{:>12.3f}{:>10.2f}x",
                worker_counts[mode], median_ms, samples.front(), samples.back(), sequential_ms / median_ms);
    }
}
}

int main(int argc, char* argv[])
{
    INFO("Compare repeated overview timings against sequential results");

    if (argc < 3)
    {
        ERROR("Usage: overview-benchmark WORKERS FILE [FILE ...]");
        return 2;
    }

    vector<uint32_t> worker_counts = {1};
    if (!parse_worker_counts(argv[1], worker_counts))
    {
        return 2;
    }

    lq_overview_request request = {
        .paths = argv + 2,
        .file_count = static_cast<size_t>(argc - 2),
        .workers = 1
    };
    vector<lq_file_stats> reference(request.file_count);
    lq_file_stats totals = {};
    if (!collect_reference(request, reference, totals))
    {
        return 1;
    }

    vector<vector<double>> timings(worker_counts.size());
    if (!run_benchmark(request, worker_counts, reference, timings))
    {
        return 1;
    }

    print_report(request.file_count, totals, worker_counts, timings);
    return 0;
}
