#include "overview.h"

#include <stdexcept>
#include <string_view>

using std::getline;
using std::ifstream;
using std::invalid_argument;
using std::runtime_error;
using std::size_t;
using std::string;
using std::string_view;
using std::streamoff;

namespace {

bool is_token_char(char ch)
{
    return (ch >= 'A' && ch <= 'Z')
        || (ch >= 'a' && ch <= 'z')
        || (ch >= '0' && ch <= '9')
        || ch == '_';
}

void analyze_line(string_view text, bool has_newline, file_info& stats)
{
    ++stats.lines;
    stats.bytes += text.size();
    // getline removes LF so add it back unless EOF ended the last line
    if (has_newline)
    {
        ++stats.bytes;
    }
    bool has_error = false;
    bool has_warn = false;
    size_t position = 0;

    while (position < text.size())
    {
        while (position < text.size() && !is_token_char(text[position]))
        {
            ++position;
        }

        if (position == text.size())
        {
            break;
        }

        const size_t begin = position;

        while (position < text.size() && is_token_char(text[position]))
        {
            ++position;
        }

        const string_view token = text.substr(begin, position - begin);
        if (token == stats.error_str)
        {
            has_error = true;
        }
        else if (token == stats.warn_str || token == stats.warning_str)
        {
            has_warn = true;
        }
    }

    stats.error_counter += has_error;
    stats.warn_counter += has_warn;
}

}

file_info analyze_file(ifstream& file)
{
    file_info stats = {};
    string line;
    while (getline(file, line))
    {
        analyze_line(line, !file.eof(), stats);
    }
    if (file.bad() || !file.eof())
    {
        throw runtime_error("failed to read file");
    }
    return stats;
}

file_info analyze_file_range(ifstream& file, streamoff begin, streamoff end)
{
    if (begin < 0 || end < begin)
    {
        throw invalid_argument("invalid file range");
    }
    if (!file.is_open() || file.bad())
    {
        throw runtime_error("failed to read file");
    }

    file_info stats = {};
    if (begin == end)
    {
        return stats;
    }

    file.clear();
    file.seekg(begin == 0 ? 0 : begin - 1);
    if (!file)
    {
        throw runtime_error("failed to seek file");
    }

    // Skip the partial first line because it belongs to an earlier range
    if (begin > 0)
    {
        char previous = 0;
        if (file.get(previous) && previous != '\n')
        {
            // Stop at end when no new line can belong to this range
            file.ignore(end - begin, '\n');
        }
    }
    if (file.bad() || (file.fail() && !file.eof()))
    {
        throw runtime_error("failed to read file");
    }
    if (file.eof())
    {
        return stats;
    }

    const streamoff position = file.tellg();
    if (position < 0)
    {
        throw runtime_error("failed to seek file");
    }
    if (position >= end)
    {
        return stats;
    }

    const size_t bytes_to_cover = static_cast<size_t>(end - position);
    string line;
    while (stats.bytes < bytes_to_cover && getline(file, line))
    {
        // Read the last owned line completely even if it crosses end
        analyze_line(line, !file.eof(), stats);
    }
    if (file.bad() || (file.fail() && !file.eof()))
    {
        throw runtime_error("failed to read file");
    }
    return stats;
}
