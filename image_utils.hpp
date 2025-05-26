#pragma once

#include "global.hpp"

#include <gst/gst.h>
#include <opencv2/core.hpp>

extern void saveImage(GstCaps *caps, GstMapInfo map, string filePathName);
extern void saveCropedImage(GstCaps *caps, GstMapInfo map, string filePathName, cv::Rect roi);