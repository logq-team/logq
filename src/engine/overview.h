#ifndef LOGQ_OVERVIEW_H
#define LOGQ_OVERVIEW_H

#include <cstddef>
#include <fstream>
#include <string>

struct file_info {
    const std::string error_str{"ERROR"};
    const std::string warning_str{"WARNING"};
    const std::string warn_str{"WARN"};
    std::size_t bytes;
    std::size_t lines;
    std::size_t error_counter;
    std::size_t warn_counter;
};

file_info analyze_file(std::ifstream& file);

#endif
