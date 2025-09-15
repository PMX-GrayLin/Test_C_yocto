
#pragma once

#include "global.hpp"

#include <vector>

#include "MvCameraControl.h"

#define NUM_GigE 2

typedef enum {
  ugc_hikrobot = 0,
  ugc_basler,
  ugc_flir,
  ugc_ids,
  ugc_other_brand,
} UsedGigeCam;

typedef enum {
  gaa_off = 0,
  gaa_once,
  gaa_continuous,
  gaa_invalid,
} GstArvAuto;

typedef enum {
  tm_off = 0,
  tm_on = 1,
} TriggerMode;


struct GigeControlParams {
  int exposure_auto;  // Auto exposure mode: 0=off, 1=once, 2=continuous
  double exposure;    // Exposure time (in microseconds)

  int gain_auto;      // Auto gain mode: 0=off, 1=once, 2=continuous
  double gain;        // Gain value (in dB)
};

void Gige_handle_RESTful_hik(std::vector<std::string> segments);

void GigE_getSettings_hik(int index_cam);

double GigE_getExposure_hik(int index_cam);
void GigE_setExposure_hik(int index_cam, const string& exposureTimeS);
GstArvAuto GigE_getExposureAuto_hik(int index_cam);
void GigE_setExposureAuto_hik(int index_cam, const string& gstArvAutoS);

double GigE_getGain_hik(int index_cam);
void GigE_setGain_hik(int index_cam, const string& gainS);
GstArvAuto GigE_getGainAuto_hik(int index_cam);
void GigE_setGainAuto_hik(int index_cam, const string& gstArvAutoS);

void GigE_setImagePath_hik(int index_cam, const string& imagePath);
void GigE_captureImage_hik(int index_cam);

void GigE_ThreadStreaming_Hik(int index_cam);
void GigE_StreamingStart_Hik(int index_cam);
void GigE_StreamingStop_Hik(int index_cam);
void GigE_streamingLED(int index_cam);
void GigE_setResolution(int index_cam, const string& resolutionS);

// for trigger mode
bool GigE_PrintDeviceInfo(MV_CC_DEVICE_INFO *pstMVDevInfo);
void GigE_cameraOpen(int index_cam);
void GigE_cameraClose(int index_cam);
void __stdcall GigE_imageCallback(MV_FRAME_OUT *pstFrame, void *pUser, bool bAutoFree);
void GigE_isTriggerMode(int index_cam);
void GigE_setTriggerMode(int index_cam, const string& triggerModeS);
void GigE_triggerModeStart(int index_cam);
void GigE_triggerModeStop(int index_cam);
void GigE_sendTriggerSoftware(int index_cam);


/* 

# gst-inspect-1.0 aravissrc

exposure            : Exposure time (?s)
flags: readable, writable
Double. Range:              -1 -           1e+08 Default:              -1

exposure-auto       : Auto Exposure Mode
flags: readable, writable
Enum "GstArvAuto" Default: 0, "off"
   (0): off              - Off
   (1): once             - Once
   (2): on               - Continuous

gain                : Gain (dB)
flags: readable, writable
Double. Range:              -1 -             500 Default:              -1

gain-auto           : Auto Gain Mode
flags: readable, writable
Enum "GstArvAuto" Default: 0, "off"
   (0): off              - Off
   (1): once             - Once
   (2): on               - Continuous

 */

/* 

# arv-tool-0.8 features

         Enumeration  : [RW] 'TriggerSelector'
              * TriggerMode
              * TriggerSoftware
              * TriggerSource
              * TriggerActivation
              * TriggerDelay
            EnumEntry   : 'FrameBurstStart'
        Enumeration  : [RW] 'TriggerMode'
            EnumEntry   : 'On'
            EnumEntry   : 'Off'
        Command      : [WO] 'TriggerSoftware'
        Enumeration  : [RW] 'TriggerSource'
            EnumEntry   : 'Anyway'
            EnumEntry   : 'Counter0'
            EnumEntry   : 'Line2'
            EnumEntry   : 'Line0'
            EnumEntry   : 'Software'
        Enumeration : 'TriggerActivation' (Not available)
            EnumEntry   : 'AnyEdge'
            EnumEntry   : 'LevelLow' (Not available)
            EnumEntry   : 'LevelHigh' (Not available)
            EnumEntry   : 'FallingEdge'
            EnumEntry   : 'RisingEdge'

*/