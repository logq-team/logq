#include "overview.h"
#include "lq_overview.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <unistd.h>

using std::ifstream;
using std::invalid_argument;
using std::ios;
using std::min;
using std::ofstream;
using std::ostream;
using std::remove;
using std::runtime_error;
using std::size_t;
using std::string;
using std::streamoff;
using uint32_limits = std::numeric_limits<uint32_t>;

namespace {

class temporary_file {
public:
    temporary_file()
    {
        char name[]{"/tmp/logq-overview-test-XXXXXX"};
        const int fd = mkstemp(name);
        if (fd == -1)
        {
            throw runtime_error("can't create temporary file");
        }
        close(fd);
        path = name;
    }

    ~temporary_file()
    {
        remove(path.c_str());
    }

    void write(const string& content)
    {
        ofstream file(path, ios::binary | ios::trunc);
        file.write(content.data(), content.size());
        file.close();
        if (!file)
        {
            throw runtime_error("can't write temporary file");
        }
    }

    string path;
};

struct test_case {
    const char* name;
    string content;
    size_t lines;
    size_t errors;
    size_t warnings;
};

void PrintTo(const test_case& test, ostream* output)
{
    *output << test.name;
}

class overview_test : public ::testing::Test {
protected:
    temporary_file input;
};

class overview_content_test : public overview_test,
                              public ::testing::WithParamInterface<test_case> {
};

TEST_P(overview_content_test, CollectsStatistics)
{
    const test_case& test = GetParam();
    input.write(test.content);
    ifstream file(input.path, ios::binary);
    ASSERT_TRUE(file.is_open());

    const file_info fi = analyze_file(file);
    EXPECT_EQ(fi.bytes, test.content.size());
    EXPECT_EQ(fi.lines, test.lines);
    EXPECT_EQ(fi.error_counter, test.errors);
    EXPECT_EQ(fi.warn_counter, test.warnings);
}

INSTANTIATE_TEST_SUITE_P(
    Overview,
    overview_content_test,
    ::testing::Values(
        test_case{"EmptyFile", "", 0, 0, 0},
        test_case{"OneEmptyLine", "\n", 1, 0, 0},
        test_case{"TwoEmptyLines", "\n\n", 2, 0, 0},
        test_case{"Lf", "ERROR\nWARN\n", 2, 1, 1},
        test_case{"Crlf", "ERROR\r\nWARNING\r\n", 2, 1, 1},
        test_case{"LastLineWithoutLf", "INFO\nERROR", 2, 1, 0},
        test_case{"SingleLineWithoutLf", "WARNING", 1, 0, 1},
        test_case{"CrlfWithUnfinishedLastLine", "ERROR\r\nWARN", 2, 1, 1},
        test_case{"MixedNewlines", "ERROR\r\nINFO\nWARN", 3, 1, 1},
        test_case{"Utf8Bytes", "Привет\nмир", 2, 0, 0},
        test_case{"RepeatedError", "ERROR ERROR ERROR\n", 1, 1, 0},
        test_case{"WarnAndWarningShareACounter", "WARN WARNING WARN\n", 1, 0, 1},
        test_case{"BothSeverities", "ERROR WARNING WARN ERROR", 1, 1, 1},
        test_case{"IndependentLines", "ERROR ERROR\nWARN WARNING\nINFO\n", 3, 1, 1},
        test_case{"TokenBoundaries", "_ERROR ERROR_ XERROR ERROR2 2ERROR ERROR_CODE WARN_CODE XWARNING", 1, 0, 0},
        test_case{"CaseSensitive", "error Error warn Warn warning Warning", 1, 0, 0},
        test_case{"Punctuation", "[ERROR]: (WARNING), WARN.", 1, 1, 1},
        test_case{"Whitespace", " \t\r\n \t", 2, 0, 0},
        test_case{"EmbeddedNul", string{"ERROR\0WARN", 10}, 1, 1, 1},
        test_case{"LongLine", string(20000, 'x') + " ERROR WARNING\n", 1, 1, 1},
        test_case{"TokenAtOldBufferBoundary", string(9996, 'x') + " ERROR\n", 1, 1, 0}
    ),
    [](const ::testing::TestParamInfo<test_case>& info)
    {
        return info.param.name;
    }
);

TEST_F(overview_test, FileOpenFailure)
{
    ifstream file(input.path + ".missing", ios::binary);
    ASSERT_FALSE(file.is_open());
    EXPECT_THROW(analyze_file(file), runtime_error);
}

TEST_F(overview_test, ReadFailure)
{
    input.write("ERROR\n");
    ifstream file(input.path, ios::binary);
    ASSERT_TRUE(file.is_open());
    file.setstate(ios::badbit);
    EXPECT_THROW(analyze_file(file), runtime_error);
}

class overview_range_test : public overview_test {
};

TEST_F(overview_range_test, OwnsLinesByStartingOffset)
{
    input.write("ERROR\r\nWARN\nWARNING");
    ifstream file(input.path, ios::binary);
    ASSERT_TRUE(file.is_open());

    const struct {
        streamoff begin;
        streamoff end;
        size_t bytes;
        size_t lines;
        size_t errors;
        size_t warnings;
    } cases[] = {
        {0, 3, 7, 1, 1, 0},
        {3, 8, 5, 1, 0, 1},
        {7, 12, 5, 1, 0, 1},
        {8, 19, 7, 1, 0, 1},
        {0, 19, 19, 3, 1, 2}
    };

    for (const auto& test : cases)
    {
        SCOPED_TRACE(test.begin);
        SCOPED_TRACE(test.end);
        const file_info stats = analyze_file_range(file, test.begin, test.end);
        EXPECT_EQ(stats.bytes, test.bytes);
        EXPECT_EQ(stats.lines, test.lines);
        EXPECT_EQ(stats.error_counter, test.errors);
        EXPECT_EQ(stats.warn_counter, test.warnings);
    }
}

TEST_F(overview_range_test, AdjacentRangesMatchWholeFileAtEverySplit)
{
    const string contents[] = {
        "",
        "\nERROR\r\nWARN\nWARNING",
        string("ERROR\0WARN\n", 11),
        "Привет ERROR\nWARN"
    };

    for (const string& content : contents)
    {
        SCOPED_TRACE(content);
        input.write(content);
        ifstream file(input.path, ios::binary);
        ASSERT_TRUE(file.is_open());
        const file_info whole = analyze_file(file);
        const streamoff length = static_cast<streamoff>(content.size());

        for (streamoff split = 0; split <= length; ++split)
        {
            SCOPED_TRACE(split);
            const file_info left = analyze_file_range(file, 0, split);
            const file_info right = analyze_file_range(file, split, length);
            EXPECT_EQ(left.bytes + right.bytes, whole.bytes);
            EXPECT_EQ(left.lines + right.lines, whole.lines);
            EXPECT_EQ(left.error_counter + right.error_counter, whole.error_counter);
            EXPECT_EQ(left.warn_counter + right.warn_counter, whole.warn_counter);
        }
    }
}

TEST_F(overview_range_test, LongLineCrossesSeveralRanges)
{
    const string content = string(20000, 'x') + " ERROR\r\nWARN";
    input.write(content);
    ifstream file(input.path, ios::binary);
    ASSERT_TRUE(file.is_open());
    const streamoff length = static_cast<streamoff>(content.size());
    file_info total = {};

    for (streamoff begin = 0; begin < length; begin += 1000)
    {
        const file_info part = analyze_file_range(file, begin, min(begin + 1000, length));
        total.bytes += part.bytes;
        total.lines += part.lines;
        total.error_counter += part.error_counter;
        total.warn_counter += part.warn_counter;
    }

    EXPECT_EQ(total.bytes, content.size());
    EXPECT_EQ(total.lines, 2);
    EXPECT_EQ(total.error_counter, 1);
    EXPECT_EQ(total.warn_counter, 1);
}

TEST_F(overview_range_test, PartialFirstLineSkipStopsAtRangeEnd)
{
    const struct {
        string content;
        streamoff begin;
        streamoff end;
    } cases[] = {
        {"ERROR\r\nWARN", 2, 4},
        {"ERROR\r\nWARN", 2, 6},
        {"ERROR\r\nWARN", 2, 7},
        {"ERROR\r\nWARN", 6, 7},
        {string(64, 'x'), 8, 16},
        {string(64, 'x') + "\nWARN", 8, 16}
    };

    for (const auto& test : cases)
    {
        SCOPED_TRACE(test.begin);
        SCOPED_TRACE(test.end);
        SCOPED_TRACE(test.content.size());
        input.write(test.content);
        ifstream file(input.path, ios::binary);
        ASSERT_TRUE(file.is_open());

        const file_info stats = analyze_file_range(file, test.begin, test.end);
        EXPECT_EQ(stats.bytes, 0);
        EXPECT_EQ(stats.lines, 0);
        EXPECT_EQ(stats.error_counter, 0);
        EXPECT_EQ(stats.warn_counter, 0);
        EXPECT_TRUE(file.good());
        EXPECT_EQ(file.tellg(), test.end);
    }
}

TEST_F(overview_range_test, EmptyAndPastEndRangesHaveZeroStatistics)
{
    const string contents[] = {"", "ERROR\n"};
    const streamoff ranges[][2] = {{0, 0}, {1, 1}, {6, 10}, {10, 20}};
    for (const string& content : contents)
    {
        SCOPED_TRACE(content);
        input.write(content);
        ifstream file(input.path, ios::binary);
        ASSERT_TRUE(file.is_open());

        for (const auto& range : ranges)
        {
            SCOPED_TRACE(range[0]);
            SCOPED_TRACE(range[1]);
            const file_info stats = analyze_file_range(file, range[0], range[1]);
            EXPECT_EQ(stats.bytes, 0);
            EXPECT_EQ(stats.lines, 0);
            EXPECT_EQ(stats.error_counter, 0);
            EXPECT_EQ(stats.warn_counter, 0);
        }
    }
}

TEST_F(overview_range_test, RejectsInvalidRangesAndUnreadableStreams)
{
    input.write("ERROR\n");
    ifstream file(input.path, ios::binary);
    ASSERT_TRUE(file.is_open());
    EXPECT_THROW(analyze_file_range(file, -1, 1), invalid_argument);
    EXPECT_THROW(analyze_file_range(file, 0, -1), invalid_argument);
    EXPECT_THROW(analyze_file_range(file, 2, 1), invalid_argument);

    file.setstate(ios::badbit);
    EXPECT_THROW(analyze_file_range(file, 0, 6), runtime_error);
    ifstream closed;
    EXPECT_THROW(analyze_file_range(closed, 0, 6), runtime_error);
}

class overview_api_test : public overview_test {
};

TEST_F(overview_api_test, PreservesOrderAndExtraStorage)
{
    input.write("ERROR\n");
    temporary_file second;
    second.write("WARN\nWARN\n");
    const char* paths[]{second.path.c_str(), input.path.c_str(), second.path.c_str()};
    lq_overview_request request{paths, 3, 1};
    lq_file_stats results[4]{};
    results[3] = {11, 12, 13, 14, LQ_FILE_INTERNAL_ERROR};

    ASSERT_EQ(lq_overview(&request, results, 4), LQ_CALL_OK);
    const lq_file_stats expected[]{
        {10, 2, 0, 2, LQ_FILE_OK},
        {6, 1, 1, 0, LQ_FILE_OK},
        {10, 2, 0, 2, LQ_FILE_OK}
    };
    for (size_t i{}; i < request.file_count; ++i)
    {
        SCOPED_TRACE(i);
        EXPECT_EQ(results[i].bytes, expected[i].bytes);
        EXPECT_EQ(results[i].lines, expected[i].lines);
        EXPECT_EQ(results[i].error_lines, expected[i].error_lines);
        EXPECT_EQ(results[i].warn_lines, expected[i].warn_lines);
        EXPECT_EQ(results[i].status, expected[i].status);
    }
    EXPECT_EQ(results[3].bytes, 11);
    EXPECT_EQ(results[3].lines, 12);
    EXPECT_EQ(results[3].error_lines, 13);
    EXPECT_EQ(results[3].warn_lines, 14);
    EXPECT_EQ(results[3].status, LQ_FILE_INTERNAL_ERROR);
}

TEST_F(overview_api_test, ContinuesAfterFileErrors)
{
    input.write("ERROR\n");
    const string missing = input.path + ".missing";
    // A directory opens on Linux but fails when read
    const char* paths[]{input.path.c_str(), missing.c_str(), ".", input.path.c_str()};
    lq_overview_request request{paths, 4, 1};
    lq_file_stats results[4];
    for (auto& result : results)
    {
        result = {99, 99, 99, 99, LQ_FILE_INTERNAL_ERROR};
    }

    ASSERT_EQ(lq_overview(&request, results, 4), LQ_CALL_OK);
    const lq_file_stats expected[]{
        {6, 1, 1, 0, LQ_FILE_OK},
        {0, 0, 0, 0, LQ_FILE_OPEN_ERROR},
        {0, 0, 0, 0, LQ_FILE_READ_ERROR},
        {6, 1, 1, 0, LQ_FILE_OK}
    };
    for (size_t i{}; i < request.file_count; ++i)
    {
        SCOPED_TRACE(i);
        EXPECT_EQ(results[i].bytes, expected[i].bytes);
        EXPECT_EQ(results[i].lines, expected[i].lines);
        EXPECT_EQ(results[i].error_lines, expected[i].error_lines);
        EXPECT_EQ(results[i].warn_lines, expected[i].warn_lines);
        EXPECT_EQ(results[i].status, expected[i].status);
    }
}

TEST_F(overview_api_test, AcceptsEmptyRequest)
{
    lq_overview_request request{nullptr, 0, 1};
    EXPECT_EQ(lq_overview(&request, nullptr, 0), LQ_CALL_OK);
    request.workers = 2;
    EXPECT_EQ(lq_overview(&request, nullptr, 0), LQ_CALL_OK);
}

TEST_F(overview_api_test, RejectsInvalidRequestsWithoutWritingResults)
{
    const char* paths[]{input.path.c_str(), input.path.c_str()};
    lq_overview_request request{paths, 2, 1};
    lq_file_stats results[2];
    for (auto& result : results)
    {
        result = {11, 12, 13, 14, LQ_FILE_INTERNAL_ERROR};
    }

    EXPECT_EQ(lq_overview(nullptr, results, 2), LQ_CALL_INVALID_ARGUMENT);
    EXPECT_EQ(lq_overview(&request, nullptr, 2), LQ_CALL_INVALID_ARGUMENT);
    EXPECT_EQ(lq_overview(&request, results, 1), LQ_CALL_INVALID_ARGUMENT);
    paths[1] = nullptr;
    EXPECT_EQ(lq_overview(&request, results, 2), LQ_CALL_INVALID_ARGUMENT);
    request.paths = nullptr;
    EXPECT_EQ(lq_overview(&request, results, 2), LQ_CALL_INVALID_ARGUMENT);
    request.paths = paths;
    paths[1] = input.path.c_str();
    request.workers = 0;
    EXPECT_EQ(lq_overview(&request, results, 2), LQ_CALL_INVALID_ARGUMENT);

    for (const auto& result : results)
    {
        EXPECT_EQ(result.bytes, 11);
        EXPECT_EQ(result.lines, 12);
        EXPECT_EQ(result.error_lines, 13);
        EXPECT_EQ(result.warn_lines, 14);
        EXPECT_EQ(result.status, LQ_FILE_INTERNAL_ERROR);
    }
}

TEST_F(overview_api_test, ParallelMatchesSequential)
{
    input.write("ERROR\r\nINFO\nWARN");
    temporary_file second;
    second.write("WARNING\nWARN\n");
    const string missing = input.path + ".missing";
    const char* paths[]{
        second.path.c_str(), input.path.c_str(), missing.c_str(), ".", second.path.c_str()
    };
    lq_overview_request request{paths, 5, 1};
    lq_file_stats sequential[5]{};
    ASSERT_EQ(lq_overview(&request, sequential, 5), LQ_CALL_OK);

    for (uint32_t workers : {2u, 8u, uint32_limits::max()})
    {
        SCOPED_TRACE(workers);
        request.workers = workers;
        lq_file_stats parallel[6];
        for (auto& result : parallel)
        {
            result = {11, 12, 13, 14, LQ_FILE_INTERNAL_ERROR};
        }

        ASSERT_EQ(lq_overview(&request, parallel, 6), LQ_CALL_OK);
        for (size_t i{}; i < request.file_count; ++i)
        {
            SCOPED_TRACE(i);
            EXPECT_EQ(parallel[i].bytes, sequential[i].bytes);
            EXPECT_EQ(parallel[i].lines, sequential[i].lines);
            EXPECT_EQ(parallel[i].error_lines, sequential[i].error_lines);
            EXPECT_EQ(parallel[i].warn_lines, sequential[i].warn_lines);
            EXPECT_EQ(parallel[i].status, sequential[i].status);
        }
        EXPECT_EQ(parallel[5].bytes, 11);
        EXPECT_EQ(parallel[5].lines, 12);
        EXPECT_EQ(parallel[5].error_lines, 13);
        EXPECT_EQ(parallel[5].warn_lines, 14);
        EXPECT_EQ(parallel[5].status, LQ_FILE_INTERNAL_ERROR);
    }
}

TEST_F(overview_api_test, ParallelHandlesFilesAroundRangeSize)
{
    const size_t range_bytes = 8 * 1024 * 1024;
    const char* paths[] = {input.path.c_str()};
    lq_overview_request request{paths, 1, 1};

    for (size_t size : {range_bytes - 1, range_bytes, range_bytes + 1})
    {
        SCOPED_TRACE(size);
        input.write(string(size - 6, 'x') + " ERROR");

        for (uint32_t workers : {1u, 2u, 8u})
        {
            SCOPED_TRACE(workers);
            request.workers = workers;
            lq_file_stats result{};
            ASSERT_EQ(lq_overview(&request, &result, 1), LQ_CALL_OK);
            EXPECT_EQ(result.bytes, size);
            EXPECT_EQ(result.lines, 1);
            EXPECT_EQ(result.error_lines, 1);
            EXPECT_EQ(result.warn_lines, 0);
            EXPECT_EQ(result.status, LQ_FILE_OK);
        }
    }
}

TEST_F(overview_api_test, ParallelRangesPreserveResultsWithFileErrors)
{
    const size_t range_bytes = 8 * 1024 * 1024;
    // The first CRLF crosses the range boundary and the final line has no LF
    const string content = string(range_bytes - 6, 'x') + " ERROR\r\nWARN\n"
        + string(range_bytes, 'x') + "\nERROR";
    input.write(content);
    temporary_file small;
    small.write("WARNING\n");
    const string missing = input.path + ".missing";
    const char* paths[] = {
        small.path.c_str(), input.path.c_str(), missing.c_str(), ".", input.path.c_str()
    };
    lq_overview_request request{paths, 5, 1};
    const lq_file_stats expected[] = {
        {8, 1, 0, 1, LQ_FILE_OK},
        {content.size(), 4, 2, 1, LQ_FILE_OK},
        {0, 0, 0, 0, LQ_FILE_OPEN_ERROR},
        {0, 0, 0, 0, LQ_FILE_READ_ERROR},
        {content.size(), 4, 2, 1, LQ_FILE_OK},
        {11, 12, 13, 14, LQ_FILE_INTERNAL_ERROR}
    };

    for (uint32_t workers : {1u, 2u, 4u, 8u})
    {
        SCOPED_TRACE(workers);
        request.workers = workers;
        lq_file_stats results[6];
        for (auto& result : results)
        {
            result = {11, 12, 13, 14, LQ_FILE_INTERNAL_ERROR};
        }

        ASSERT_EQ(lq_overview(&request, results, 6), LQ_CALL_OK);
        for (size_t i = 0; i < 6; ++i)
        {
            SCOPED_TRACE(i);
            EXPECT_EQ(results[i].bytes, expected[i].bytes);
            EXPECT_EQ(results[i].lines, expected[i].lines);
            EXPECT_EQ(results[i].error_lines, expected[i].error_lines);
            EXPECT_EQ(results[i].warn_lines, expected[i].warn_lines);
            EXPECT_EQ(results[i].status, expected[i].status);
        }
    }
}

}
