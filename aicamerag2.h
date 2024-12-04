#pragma once

#include <string> 
#include <regex>

#include <mosquittopp.h>
#include <linux/videodev2.h>  // For V4L2 definitions
#include <sys/ioctl.h>        // For ioctl()
#include <fcntl.h>            // For open()


std::string AICamrea_getVideoDevice();
int AICamera_getBrightness();
void AICamera_setBrightness(int value);
void AICamera_setWhiteBalanceAutomatic(bool enable);
