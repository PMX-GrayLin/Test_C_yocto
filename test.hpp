#pragma once

#include "global.hpp"

#include <vector>

// RESTful
namespace httplib {
    class Response;  // Forward declaration
}
void handle_RESTful(std::vector<std::string> segments);

// #if defined(ENABLE_FTDI)
// void test_ftdi();
// #endif // ENABLE_FTDI




