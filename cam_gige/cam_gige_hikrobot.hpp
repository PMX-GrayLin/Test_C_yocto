
#pragma once

#include "global.hpp"

#include <vector>

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
} GstArvAuto;

struct GigeControlParams {
  int exposure_auto;  // Auto exposure mode: 0=off, 1=once, 2=continuous
  double exposure;    // Exposure time (in microseconds)

  int gain_auto;      // Auto gain mode: 0=off, 1=once, 2=continuous
  double gain;        // Gain value (in dB)
};

void Gige_handle_RESTful_hik(std::vector<std::string> segments);

void GigE_getSettings_hik();

double GigE_getExposure_hik();
void GigE_setExposure_hik(string exposureTimeS);
GstArvAuto GigE_getExposureAuto_hik();
void GigE_setExposureAuto_hik(string gstArvAutoS);

double GigE_getGain_hik();
void GigE_setGain_hik(string gainS);
GstArvAuto GigE_getGainAuto_hik();
void GigE_setGainAuto_hik(string gstArvAutoS);

void GigE_setImagePath_hik(const string& imagePath);
void GigE_captureImage_hik();

void GigE_ThreadStreaming_Hik();
void GigE_StreamingStart_Hik();
void GigE_StreamingStop_Hik();
void GigE_streamingLED();
void GigE_setResolution(const string& indexS, const string& resolutionS);

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