#include "lq_overview.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

static constexpr std::string info_prefix{"[CPP][INFO] "};
static constexpr std::string error_prefix{"[CPP][ERROR] "};

int main(int argc, char* argv[])
{
    std::cout << info_prefix << "Compare repeated overview timings against sequential results\n";

    if (argc < 3)
    {
        std::cerr << error_prefix << "Usage: overview-benchmark WORKERS FILE [FILE ...]\n";
        return 2;
    }

    std::vector<uint32_t> worker_counts{1};
    std::string_view remaining{argv[1]};
    // This is needed for "BENCH_WORKERS=1,2,3,4" feature to run different workers
    while (true)
    {
        const size_t comma = remaining.find(',');
        const std::string_view argument = remaining.substr(0, comma);
        uint32_t workers{};
        const auto parsed = std::from_chars(argument.data(), argument.data() + argument.size(), workers);
        if (parsed.ec != std::errc{} || parsed.ptr != argument.data() + argument.size() || workers == 0)
        {
            std::cerr << error_prefix << "WORKERS must be comma-separated positive uint32 values\n";
            return 2;
        }

        if (std::find(worker_counts.begin(), worker_counts.end(), workers) == worker_counts.end())
        {
            worker_counts.push_back(workers);
        }
        if (comma == std::string_view::npos)
        {
            break;
        }
        remaining.remove_prefix(comma + 1);
    }

    lq_overview_request request{
        .paths = argv + 2,
        .file_count = static_cast<size_t>(argc - 2),
        .workers = 1
    };
    std::vector<lq_file_stats> results(request.file_count);
    std::vector<lq_file_stats> sequential(request.file_count);

    lq_overview_request sequential_request = request;
    sequential_request.workers = 1;
    const int32_t sequential_status = lq_overview(&sequential_request, sequential.data(), sequential.size());
    if (sequential_status != LQ_CALL_OK)
    {
        std::cerr << error_prefix << "Sequential reference returned call_status = " << sequential_status << '\n';
        return 1;
    }

    uint64_t bytes{};
    uint64_t lines{};
    uint64_t error_lines{};
    uint64_t warn_lines{};
    for (size_t i{}; i < sequential.size(); ++i)
    {
        const lq_file_stats& result = sequential[i];
        if (result.status != LQ_FILE_OK)
        {
            std::cerr << error_prefix << request.paths[i] << ": file_status = " << result.status << '\n';
            return 1;
        }

        bytes += result.bytes;
        lines += result.lines;
        error_lines += result.error_lines;
        warn_lines += result.warn_lines;
    }

    constexpr size_t repeat_count{7};
    std::vector<std::vector<double>> timings(worker_counts.size());
    for (auto& samples : timings)
    {
        samples.reserve(repeat_count);
    }

    for (size_t round{}; round <= repeat_count; ++round)
    {
        for (size_t offset{}; offset < worker_counts.size(); ++offset)
        {
            const size_t mode = (offset + round) % worker_counts.size();
            request.workers = worker_counts[mode];
            std::fill(results.begin(), results.end(), lq_file_stats{0, 0, 0, 0, LQ_FILE_INTERNAL_ERROR});

            const auto start = std::chrono::steady_clock::now();
            const int32_t status = lq_overview(&request, results.data(), results.size());
            const auto finish = std::chrono::steady_clock::now();

            if (status != LQ_CALL_OK)
            {
                std::cerr << error_prefix << "workers = " << request.workers
                          << " lq_overview returned call_status = " << status << '\n';
                return 1;
            }

            for (size_t i{}; i < results.size(); ++i)
            {
                const lq_file_stats& result = results[i];
                const lq_file_stats& expected = sequential[i];
                if (result.bytes != expected.bytes || result.lines != expected.lines
                    || result.error_lines != expected.error_lines || result.warn_lines != expected.warn_lines
                    || result.status != expected.status)
                {
                    std::cerr << error_prefix << "workers = " << request.workers
                              << " sequential result mismatch at index " << i << ": " << request.paths[i] << '\n';
                    return 1;
                }
            }

            if (round > 0)
            {
                const std::chrono::duration<double, std::milli> elapsed = finish - start;
                timings[mode].push_back(elapsed.count());
            }
        }
    }

    for (auto& samples : timings)
    {
        std::sort(samples.begin(), samples.end());
    }
    const double sequential_ms = timings[0][repeat_count / 2];

    std::cout << info_prefix << "Per-file results match the sequential reference\n"
              << info_prefix << "files = " << request.file_count << '\n'
              << info_prefix << "bytes = " << bytes << '\n'
              << info_prefix << "lines = " << lines << '\n'
              << info_prefix << "error_lines = " << error_lines << '\n'
              << info_prefix << "warn_lines = " << warn_lines << '\n'
              << info_prefix << "warmups_per_mode = 1\n"
              << info_prefix << "repeats_per_mode = " << repeat_count << '\n'
              << info_prefix << "Warm file cache with rotating mode order\n"
              << info_prefix << "WORKERS   MEDIAN_MS      MIN_MS      MAX_MS   SPEEDUP\n";

    for (size_t mode{}; mode < worker_counts.size(); ++mode)
    {
        const auto& samples = timings[mode];
        const double median_ms = samples[repeat_count / 2];
        std::cout << info_prefix << std::fixed << std::setprecision(3)
                  << std::setw(7) << worker_counts[mode]
                  << std::setw(12) << median_ms
                  << std::setw(12) << samples.front()
                  << std::setw(12) << samples.back()
                  << std::setw(10) << std::setprecision(2) << sequential_ms / median_ms << "x\n";
    }

    return 0;
}
