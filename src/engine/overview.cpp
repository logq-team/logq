#include "overview.h"

#include <stdexcept>
#include <string_view>

namespace {

bool is_token_char(char ch)
{
    return (ch >= 'A' && ch <= 'Z')
        || (ch >= 'a' && ch <= 'z')
        || (ch >= '0' && ch <= '9')
        || ch == '_';
}

}

file_info analyze_file(std::ifstream& file)
{
    file_info fi{};
    std::string line;
    while (std::getline(file, line))
    {
        ++fi.lines;
        fi.bytes += line.size();
        // getline removes LF so add it back unless EOF ended the last line
        if (!file.eof())
        {
            ++fi.bytes;
        }
        bool has_error{};
        bool has_warn{};
        std::string_view text{line};
        std::size_t pos{};

        while (pos < text.size())
        {
            while (pos < text.size() && !is_token_char(text[pos]))
            {
                ++pos;
            }

            if (pos == text.size())
            {
                break;
            }

            const std::size_t begin = pos;

            while (pos < text.size() && is_token_char(text[pos]))
            {
                ++pos;
            }

            const std::string_view token = text.substr(begin, pos - begin);
            if (token == fi.error_str)
            {
                has_error = true;
            }
            else if (token == fi.warn_str || token == fi.warning_str)
            {
                has_warn = true;
            }
        }

        fi.error_counter += has_error;
        fi.warn_counter += has_warn;
    }
    if (file.bad() || !file.eof())
    {
        throw std::runtime_error("failed to read file");
    }
    return fi;
}
