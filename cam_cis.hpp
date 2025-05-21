// cis for camera image sensor

#pragma once

#include "global.hpp"


// ?? to update
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
