// AI Camera Plus embeded CIS ( Camera Image Sensor )
// OG05b10
// AICP for AI Camera Plus

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

extern bool AICP_isUseCISCamera();
extern std::string AICP_getVideoDevice();

int ioctl_get_value(int control_ID);
int ioctl_set_value(int control_ID, int value);
extern int AICP_getBrightness();
extern void AICP_setBrightness(int value);
extern int AICP_getContrast();
extern void AICP_setContrast(int value);
extern int AICP_getSaturation();
extern void AICP_setSaturation(int value);
extern int AICP_getHue();
extern void AICP_setHue(int value);
extern int AICP_getWhiteBalanceAutomatic();
extern void AICP_setWhiteBalanceAutomatic(bool enable);
extern int AICP_getSharpness();
extern void AICP_setSharpness(int value);
extern int AICP_getISO();
extern void AICP_setISO(int value);
extern int AICP_getExposure();
extern void AICP_setExposure(int value);
extern int AICP_getWhiteBalanceTemperature();
extern void AICP_setWhiteBalanceTemperature(int value);
extern int AICP_getExposureAuto();
extern void AICP_setExposureAuto(bool enable);
extern int AICP_getExposureTimeAbsolute();
extern void AICP_setExposureTimeAbsolute(double sec);
extern int AICP_getFocusAbsolute();
extern void AICP_setFocusAbsolute(int value);
extern int AICP_getFocusAuto();
extern void AICP_setFocusAuto(bool enable);  
// IOCTLS ===========

extern void AICP_setImagePath(const string& imagePath);
extern void AICP_setCropImagePath(const string& imagePath);
extern void AICP_setInputImagePath(const string& imagePath);
extern bool AICP_isCropImage();
extern void AICP_captureImage();
extern void AICP_enableCrop(bool enable);
extern void AICP_enablePadding(bool enable);

// Streaming
void ThreadAICameraStreaming();
void ThreadAICameraStreaming_usb();
extern void AICP_streamingStart();
extern void AICP_streamingStop();

// image processing
extern void AICP_load_crop_saveImage();

