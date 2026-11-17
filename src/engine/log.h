#ifndef LOGQ_LOG_H
#define LOGQ_LOG_H

#include <cstdio>
#include <print>

#define INFO(...) std::println("[CPP][INFO] " __VA_ARGS__)
#define DEBUG(...) std::println("[CPP][DEBUG] " __VA_ARGS__)
#define ERROR(...) std::println(stderr, "[CPP][ERROR] " __VA_ARGS__)

#endif
