#include "overview.h"
#include "lq_overview.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <unistd.h>

namespace {

class temporary_file {
public:
    temporary_file()
    {
        char name[]{"/tmp/logq-overview-test-XXXXXX"};
        const int fd = mkstemp(name);
        if (fd == -1)
        {
            throw std::runtime_error("can't create temporary file");
        }
        close(fd);
        path = name;
    }

    ~temporary_file()
    {
        std::remove(path.c_str());
    }

    void write(const std::string& content)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(content.data(), content.size());
        file.close();
        if (!file)
        {
            throw std::runtime_error("can't write temporary file");
        }
    }

    std::string path;
};

struct test_case {
    const char* name;
    std::string content;
    std::size_t lines;
    std::size_t errors;
    std::size_t warnings;
};

void PrintTo(const test_case& test, std::ostream* output)
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
    std::ifstream file(input.path, std::ios::binary);
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
        test_case{"EmbeddedNul", std::string{"ERROR\0WARN", 10}, 1, 1, 1},
        test_case{"LongLine", std::string(20000, 'x') + " ERROR WARNING\n", 1, 1, 1},
        test_case{"TokenAtOldBufferBoundary", std::string(9996, 'x') + " ERROR\n", 1, 1, 0}
    ),
    [](const ::testing::TestParamInfo<test_case>& info)
    {
        return info.param.name;
    }
);

TEST_F(overview_test, FileOpenFailure)
{
    std::ifstream file(input.path + ".missing", std::ios::binary);
    ASSERT_FALSE(file.is_open());
    EXPECT_THROW(analyze_file(file), std::runtime_error);
}

TEST_F(overview_test, ReadFailure)
{
    input.write("ERROR\n");
    std::ifstream file(input.path, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file.setstate(std::ios::badbit);
    EXPECT_THROW(analyze_file(file), std::runtime_error);
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
    for (std::size_t i{}; i < request.file_count; ++i)
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
    const std::string missing = input.path + ".missing";
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
    for (std::size_t i{}; i < request.file_count; ++i)
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
    const std::string missing = input.path + ".missing";
    const char* paths[]{
        second.path.c_str(), input.path.c_str(), missing.c_str(), ".", second.path.c_str()
    };
    lq_overview_request request{paths, 5, 1};
    lq_file_stats sequential[5]{};
    ASSERT_EQ(lq_overview(&request, sequential, 5), LQ_CALL_OK);

    for (uint32_t workers : {2u, 8u, std::numeric_limits<uint32_t>::max()})
    {
        SCOPED_TRACE(workers);
        request.workers = workers;
        lq_file_stats parallel[6];
        for (auto& result : parallel)
        {
            result = {11, 12, 13, 14, LQ_FILE_INTERNAL_ERROR};
        }

        ASSERT_EQ(lq_overview(&request, parallel, 6), LQ_CALL_OK);
        for (std::size_t i{}; i < request.file_count; ++i)
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

}
