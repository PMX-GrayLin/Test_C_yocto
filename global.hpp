#pragma once

// ========== enable / disable functions ==========
// #define ENABLE_FTDI

// ========== enable / disable functions ==========

// C headers
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <poll.h>

// C++ Standard Library headers 
#include <string>
#include <cstring>

// C++ Standard Library headers, heavy
#include <thread>
#include <chrono>

// #if defined(ENABLE_FTDI)
// #include "ftd2xx.h"
// #include "libft4222.h"
// #endif

#define DEBUGX
#ifndef DEBUGX
#define xlog(...) ((void)0)
#else
#define xlog(fmt, ...) printf("%s:%d, " fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#endif

using namespace std;

extern int testCounter;

extern void startTimer(int ms);
extern void stopTimer();

extern void printBuffer(const uint8_t* buffer, size_t len);
extern void printArray_float(const float* buffer, size_t len);
extern void printArray_forUI(const float* buffer, size_t len);

extern bool isSameString(const char* s1, const char* s2, bool isCaseSensitive = false);
extern bool isPathExist(const char* path);
double limitValueInRange(double input, double rangeMin, double rangeMax);

extern std::string getTimeString();

std::string get_parent_directory(const std::string& path);
extern std::string exec_command(const std::string& cmd);