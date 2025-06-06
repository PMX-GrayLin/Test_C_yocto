// AI Camera Plus embeded CIS ( Camera Image Sensor )
// OG05b10

#pragma once

#include "global.hpp"

#include <vector>

#define AICamreaCISPath "/dev/csi_cam_preview"
#define AICamreaUSBPath "/dev/video137"

typedef enum {
    spf_BMP,
    spf_JPEG,
    spf_PNG,
} SavedPhotoFormat;

extern void CIS_handle_RESTful(std::vector<std::string> segments);

extern bool AICamrea_isUseCISCamera();
extern std::string AICamrea_getVideoDevice();

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
extern int AICamera_getSharpness();
extern void AICamera_setSharpness(int value);
extern int AICamera_getISO();
extern void AICamera_setISO(int value);
extern int AICamera_getExposure();
extern void AICamera_setExposure(int value);
extern int AICamera_getWhiteBalanceTemperature();
extern void AICamera_setWhiteBalanceTemperature(int value);
extern int AICamera_getExposureAuto();
extern void AICamera_setExposureAuto(bool enable);
extern int AICamera_getExposureTimeAbsolute();
extern void AICamera_setExposureTimeAbsolute(double sec);
extern int AICamera_getFocusAbsolute();
extern void AICamera_setFocusAbsolute(int value);
extern int AICamera_getFocusAuto();
extern void AICamera_setFocusAuto(bool enable);  
// IOCTLS ===========

extern void AICamera_setImagePath(const string& imagePath);
extern void AICamera_setCropImagePath(const string& imagePath);
extern void AICamera_setInputImagePath(const string& imagePath);
extern bool AICamera_isCropImage();
extern void AICamera_captureImage();
extern void AICamera_enableCrop(bool enable);
extern void AICamera_enablePadding(bool enable);

// Streaming
void ThreadAICameraStreaming();
void ThreadAICameraStreaming_usb();
extern void AICamera_streamingStart();
extern void AICamera_streamingStop();

// image processing
extern void AICAMERA_load_crop_saveImage();

