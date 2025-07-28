#pragma once

#include "global.hpp"

#include <vector>

// RESTful
namespace httplib {
    class Response;  // Forward declaration
}
void handle_RESTful(std::vector<std::string> segments, httplib::Response &res);

// #if defined(ENABLE_FTDI)
// void test_ftdi();
// #endif // ENABLE_FTDI




