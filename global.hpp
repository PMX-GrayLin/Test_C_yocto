#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <string> 
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>

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
