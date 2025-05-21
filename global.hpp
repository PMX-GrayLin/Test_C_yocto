#pragma once

// #define ENABLE_ARAVIS
// #define ENABLE_FTDI

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <poll.h>

#include <linux/videodev2.h>  // For V4L2 definitions
#include <sys/ioctl.h>        // For ioctl()
#include <fcntl.h>            // For open()

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <regex>
#include <filesystem>
#include <iomanip>

#include "json.h"
#include <mosquittopp.h>

#include <gst/gst.h>
#include <opencv2/opencv.hpp>
#include <gpiod.h>

#include "httplib.hpp"

#if defined(ENABLE_ARAVIS)
#include <arv.h>
#endif

#if defined(ENABLE_FTDI)
#include "ftd2xx.h"
#include "libft4222.h"
#endif

#define DEBUGX
#ifndef DEBUGX
#define xlog(...) ((void)0)
#else
#define xlog(fmt, ...) printf("%s:%d, " fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#endif

#define NUM_CAM_USE 2

using namespace std;

extern bool isSave2Jpeg;

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
