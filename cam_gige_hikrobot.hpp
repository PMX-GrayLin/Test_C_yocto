
#pragma once

#include "global.hpp"

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
  double exposure;    // Exposure time (e.g., in microseconds)
  double gain;        // Gain value (e.g., in dB)
  int exposure_auto;  // Auto exposure mode: 0=off, 1=once, 2=continuous
  int gain_auto;      // Auto gain mode: 0=off, 1=once, 2=continuous
};

extern void Gige_handle_RESTful(std::vector<std::string> segments);

extern void GigE_getSettings_hik();

extern double GigE_getExposure_hik();
extern void GigE_setExposure_hik(string exposureTimeS);
extern GstArvAuto GigE_getExposureAuto_hik();
extern void GigE_setExposureAuto_hik(string gstArvAutoS);

extern double GigE_getGain_hik();
extern void GigE_setGain_hik(string gainS);
extern GstArvAuto GigE_getGainAuto_hik();
extern void GigE_setGainAuto_hik(string gstArvAutoS);

void GigE_ThreadStreaming_Hik();
extern void GigE_StreamingStart_Hik();
extern void GigE_StreamingStop_Hik();

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