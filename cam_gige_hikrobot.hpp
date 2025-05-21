
#pragma once

#include "global.hpp"

typedef enum {
    ugc_hikrobot = 0,
    ugc_brand_x,
    ugc_brand_y,
  } UsedGigeCam;

typedef enum {
    gaa_off = 0,
    gaa_once,
    gaa_continuous,
  } GstArvAuto;
  
extern double GigE_getExposure_hik();
extern void GigE_setExposure_hik(string exposureTimeS);
extern double GigE_getExposureAuto_hik();
extern void GigE_setExposureAuto_hik(GstArvAuto gaa);

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