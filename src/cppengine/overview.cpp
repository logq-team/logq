#include "overview.h"

#include <stdexcept>
#include <string_view>

using std::getline;
using std::ifstream;
using std::runtime_error;
using std::size_t;
using std::string;
using std::string_view;

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
