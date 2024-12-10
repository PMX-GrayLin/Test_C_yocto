#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <linux/videodev2.h>  // For V4L2 definitions
#include <sys/ioctl.h>        // For ioctl()
#include <fcntl.h>            // For open()

#include <string> 
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <regex>

using namespace std;

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

extern bool isSave2Jpeg;

extern int testCounter;

extern void startTimer(int ms);
extern void stopTimer();

