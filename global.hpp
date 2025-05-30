#pragma once

// ========== enable / disable functions ==========
// #define ENABLE_FTDI

// ========== enable / disable functions ==========

// C headers
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// C++ Standard Library headers 
#include <string>
#include <cstring>

// C++ Standard Library headers, heavier
#include <thread>

using namespace std;

#define DEBUGX
#ifndef DEBUGX
#define xlog(...) ((void)0)
#else
#define xlog(fmt, ...) printf("%s:%d, " fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#endif

extern int testCounter;

extern void startTimer(int ms);
extern void stopTimer();

extern void printBuffer(const uint8_t* buffer, size_t len);

bool isSameString(const char* s1, const char* s2, bool isCaseSensitive = false);
bool isPathExist(const char* path);
double limitValueInRange(double input, double rangeMin, double rangeMax);

std::string getTimeString();

std::string get_parent_directory(const std::string& path);
std::string exec_command(const std::string& cmd);