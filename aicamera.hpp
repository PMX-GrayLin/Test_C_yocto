#pragma once

#include "global.hpp"

#define AICamreaCSIPath "/dev/csi_cam_preview"
#define AICamreaUSBPath "/dev/video137"

#define GPIO_CHIP       "/dev/gpiochip0"
#define NUM_DI          2                 // Number of DI
#define NUM_Triger      2                 // Number of Triger, treat as DI
#define NUM_DO          2                 // Number of DO

// ? NUM_DIOm aicamera = 2, visionhb = 4
#define NUM_DIO         4                 // Number of DIO

typedef enum {
  lc_red,
  lc_green,
  lc_orange,
  lc_off,
} LEDColor;

typedef enum {
    spf_BMP,
    spf_JPEG,
    spf_PNG,
} SavedPhotoFormat;

typedef enum {
  diod_in,
  diod_out,
} DIO_Direction;


extern bool AICamrea_isUseCSICamera();
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
extern void AICamera_setCropROI(cv::Rect roi);
extern bool AICamera_isCropImage();
extern void AICamera_captureImage();
extern void AICamera_enableCrop(bool enable);
extern void AICamera_enablePadding(bool enable);

// Streaming
GstPadProbeReturn AICAMERA_streamingDataCallback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
void ThreadAICameraStreaming();
void ThreadAICameraStreaming_usb();
extern void AICamera_streamingStart();
extern void AICamera_streamingStop();

// image processing
extern void AICAMERA_load_crop_saveImage();

// led
void AICamera_setGPIO(int gpio_num, int value);
void AICamera_setLED(string led_index, string led_color);

// DI
void ThreadAICameraMonitorDI();
extern void AICamera_MonitorDIStart();
extern void AICamera_MonitorDIStop();

// Triger
void ThreadAICameraMonitorTriger();
extern void AICamera_MonitorTrigerStart();
extern void AICamera_MonitorTrigerStop();

// DO
extern void AICamera_setDO(string index_do, string on_off);

// DIO
void ThreadAICameraMonitorDIOIn(int index_dio);
extern void AICamera_MonitorDIOInStart(int index_dio);
extern void AICamera_MonitorDIOInStop(int index_dio);
extern void AICamera_setDIODirection(string index_dio, string di_do);
extern void AICamera_setDIOOut(string index_dio, string on_off);

// PWM 
void AICamera_writePWMFile(const std::string &path, const std::string &value);
extern void AICamera_setPWM(string sPercent);
