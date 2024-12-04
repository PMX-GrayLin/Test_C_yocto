
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <vector>
#include <algorithm>
#include <thread>
#include <string> 
#include <regex>
#include <iostream>

#include <mosquittopp.h>
#include <linux/videodev2.h>  // For V4L2 definitions
#include <sys/ioctl.h>        // For ioctl()
#include <fcntl.h>            // For open()

using namespace std;

// yocto test
#include "json.h" 

#define DEBUGX

#ifndef DEBUGX
#define xlog(...) ((void)0)
#else
#define xlog(fmt, ...) printf("%s:%d, " fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#endif


std::string AICamrea_getVideoDevice();
int AICamera_getBrightness();
void AICamera_setBrightness(int value);
void AICamera_setWhiteBalanceAutomatic(bool enable);
