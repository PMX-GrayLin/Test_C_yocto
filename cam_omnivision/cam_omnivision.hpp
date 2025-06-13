// AI Camera Plus embeded CIS ( Camera Image Sensor )
// OG05b10
// AICP for AI Camera Plus

#pragma once

#include "global.hpp"

#include <vector>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#define AICamreaCISPath "/dev/csi_cam_preview"
#define AICamreaUSBPath "/dev/video137"

typedef enum {
    spf_BMP,
    spf_JPEG,
    spf_PNG,
} SavedPhotoFormat;

void AICP_handle_RESTful(std::vector<std::string> segments);

bool AICP_isUseCISCamera();
std::string AICP_getVideoDevice();

// IOCTLS ===========
int ioctl_get_value_aic(int control_ID);
int ioctl_set_value_aic(int control_ID, int value);
int AICP_getBrightness();
void AICP_setBrightness(int value);
int AICP_getContrast();
void AICP_setContrast(int value);
int AICP_getSaturation();
void AICP_setSaturation(int value);
int AICP_getHue();
void AICP_setHue(int value);
int AICP_getWhiteBalanceAutomatic();
void AICP_setWhiteBalanceAutomatic(bool enable);
int AICP_getSharpness();
void AICP_setSharpness(int value);
int AICP_getISO();
void AICP_setISO(int value);
int AICP_getExposure();
void AICP_setExposure(int value);
int AICP_getWhiteBalanceTemperature();
void AICP_setWhiteBalanceTemperature(int value);
int AICP_getExposureAuto();
void AICP_setExposureAuto(bool enable);
int AICP_getExposureTimeAbsolute();
void AICP_setExposureTimeAbsolute(double sec);
int AICP_getFocusAbsolute();
void AICP_setFocusAbsolute(int value);
int AICP_getFocusAuto();
void AICP_setFocusAuto(bool enable);  
// IOCTLS ===========

void AICP_setImagePath(const string& imagePath);
void AICP_setCropImagePath(const string& imagePath);
void AICP_setInputImagePath(const string& imagePath);
void AICP_setCropROI(cv::Rect roi);
bool AICP_isCropImage();
void AICP_captureImage();
void AICP_enableCrop(bool enable);
void AICP_enablePadding(bool enable);

// Streaming
void Thread_AICPStreaming();
void Thread_AICPStreaming_usb();
void AICP_streamingStart();
void AICP_streamingStop();
void AICP_streamingLED();

// image processing
void AICP_load_crop_saveImage();

