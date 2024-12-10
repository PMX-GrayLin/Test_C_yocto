#pragma once

#include "global.hpp"

extern std::string AICamrea_getVideoDevice();

// IOCTLS
int ioctl_get_value(int control_ID);
int ioctl_set_value(int control_ID, int value);
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
extern int AICamera_getExposure();
extern void AICamera_setExposure(int value);
extern int AICamera_getWhiteBalanceTemperature();
extern void AICamera_setWhiteBalanceTemperature(int value);
extern int AICamera_getExposureAuto();
extern void AICamera_setExposureAuto(bool enable);
extern int AICamera_getFocusAbsolute();
extern void AICamera_setFocusAbsolute(int value);
extern int AICamera_getFocusAuto();
extern void AICamera_setFocusAuto(bool enable);  

// Streaming
GstPadProbeReturn cb_streaming_data(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
void ThreadAICameraStreaming(int param);
extern void AICamera_startStreaming();
extern void AICamera_stopStreaming();