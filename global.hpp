#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <linux/videodev2.h>  // For V4L2 definitions
#include <sys/ioctl.h>        // For ioctl()
#include <fcntl.h>            // For open()
#include <gpiod.h>

#include <string> 
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <regex>
#include <filesystem>

// yocto test
#include "json.h"
#include <mosquittopp.h>

#include <gst/gst.h>
#include <opencv2/opencv.hpp>

#define DEBUGX
#ifndef DEBUGX
#define xlog(...) ((void)0)
#else
#define xlog(fmt, ...) printf("%s:%d, " fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#endif

#define NUM_CAM_USE 2
#define USE_TOF
#define DEBUG_TOF
// #define DEBUG_SPI

using namespace std;

extern bool isSave2Jpeg;

extern int testCounter;

extern void startTimer(int ms);
extern void stopTimer();

extern void printBuffer(const uint8_t* buffer, size_t len);
extern void printArray_float(const float* buffer, size_t len);
extern void printArray_forUI(const float* buffer, size_t len);

extern bool isSameString(char* s1, char* s2, bool isCaseSensitive = false);