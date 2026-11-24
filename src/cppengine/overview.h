#ifndef LOGQ_OVERVIEW_H
#define LOGQ_OVERVIEW_H

#include <cstddef>
#include <fstream>
#include <string>

struct file_info {
    const std::string error_str = "ERROR";
    const std::string warning_str = "WARNING";
    const std::string warn_str = "WARN";
    std::size_t bytes = 0;
    std::size_t lines = 0;
    std::size_t error_counter = 0;
    std::size_t warn_counter = 0;
};

file_info analyze_file(std::ifstream& file);

// The stream must be opened in binary mode
// Count whole lines starting at or after begin and before end
// Skip a partial first line only up to end and read owned lines completely
// A single long line is still analyzed by one task
file_info analyze_file_range(std::ifstream& file, std::streamoff begin, std::streamoff end);

#endif
