#pragma once

#include <string> 
#include <regex>

#include <mosquittopp.h>
#include <linux/videodev2.h>  // For V4L2 definitions
#include <sys/ioctl.h>        // For ioctl()
#include <fcntl.h>            // For open()

extern std::string AICamrea_getVideoDevice();
extern int AICamera_getBrightness();
extern void AICamera_setBrightness(int value);
extern void AICamera_setWhiteBalanceAutomatic(bool enable);
