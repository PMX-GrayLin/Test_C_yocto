#pragma once

#include "global.hpp"

#define AICamreaCSIPath "/dev/csi_cam_preview"
#define AICamreaUSBPath "/dev/video137"

#define GPIO_CHIP "/dev/gpiochip0"
#define NUM_DI    2                 // Number of DI
#define NUM_DO    2                 // Number of DO
#define NUM_DIO   2                 // Number of DIO

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

// IOCTLS ===========
/* 

User Controls

                     brightness 0x00980900 (int)    : min=0 max=100 step=1 default=0 value=0 flags=slider
                       contrast 0x00980901 (int)    : min=0 max=10 step=1 default=0 value=0 flags=slider
                     saturation 0x00980902 (int)    : min=0 max=10 step=1 default=0 value=0 flags=slider
                            hue 0x00980903 (int)    : min=0 max=100 step=1 default=0 value=0 flags=slider
        white_balance_automatic 0x0098090c (bool)   : default=1 value=1
                       exposure 0x00980911 (int)    : min=-40 max=40 step=1 default=0 value=0
           power_line_frequency 0x00980918 (menu)   : min=0 max=3 default=3 value=3 (Auto)
				0: Disabled
				1: 50 Hz
				2: 60 Hz
				3: Auto
      white_balance_temperature 0x0098091a (int)    : min=2700 max=6500 step=1 default=6500 value=6500
                      sharpness 0x0098091b (int)    : min=0 max=10 step=1 default=0 value=0 flags=slider
  min_number_of_capture_buffers 0x00980927 (int)    : min=1 max=32 step=1 default=1 value=8 flags=read-only, volatile
                            iso 0x009819a9 (int)    : min=100 max=6400 step=100 default=100 value=100

Camera Controls

                  auto_exposure 0x009a0901 (menu)   : min=0 max=1 default=0 value=0 (Auto Mode)
				0: Auto Mode
				1: Manual Mode
         exposure_time_absolute 0x009a0902 (int)    : min=100000 max=100000000 step=1 default=33000000 value=33000000
                 focus_absolute 0x009a090a (int)    : min=0 max=255 step=1 default=0 value=0
     focus_automatic_continuous 0x009a090c (bool)   : default=0 value=0

V4L2_CID_EXPOSURE_ABSOLUTE (integer)
Determines the exposure time of the camera sensor. The exposure time is limited by the frame interval. 
Drivers should interpret the values as 100 µs units, where the value 1 stands for 1/10000th of a second, 10000 for 1 second and 100000 for 10 seconds.

*/

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
