#include "overview.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
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

}
