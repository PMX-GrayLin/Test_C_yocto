#pragma once

#include "global.hpp"

typedef enum {
    spf_BMP,
    spf_JPEG,
    spf_PNG,
} SavedPhotoFormat;

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

extern void AICamera_setImagePath(string imagePath);
extern void AICamera_setCropImagePath(string imagePath);
extern void AICamera_setInputImagePath(string imagePath);
extern void AICamera_setCropROI(cv::Rect roi);
extern bool AICamera_isCropImage();
extern void AICamera_captureImage();
extern void AICamera_enableCrop(bool enable);
extern void AICamera_enablePadding(bool enable);

// Streaming
GstPadProbeReturn AICAMERA_streamingDataCallback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
void ThreadAICameraStreaming(int param);
extern void AICamera_startStreaming();
extern void AICamera_stopStreaming();

// image processing
extern void AICAMERA_load_crop_saveImage();
