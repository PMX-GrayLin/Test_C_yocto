#pragma once

#include <string> 
#include <regex>

#include <mosquittopp.h>
#include <linux/videodev2.h>  // For V4L2 definitions
#include <sys/ioctl.h>        // For ioctl()
#include <fcntl.h>            // For open()

extern std::string AICamrea_getVideoDevice();

// IOCTLS
int ioctl_get_value(int control_ID);
extern int AICamera_getBrightness();
extern void AICamera_setBrightness(int value);
extern int AICamera_getContrast();
extern void AICamera_setContrast(int value);
extern int AICamera_getSaturation();
extern void AICamera_setSaturation(int value);
extern int AICamera_getHue();
extern void AICamera_setHue(int value);
extern int AICamera_getWhiteBalanceAutomatic();
extern void AICamera_setWhiteBalanceAutomatic(bool enable);

// Streaming