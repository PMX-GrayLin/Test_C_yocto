#pragma once

#include "global.hpp"

#include <gst/gst.h>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

extern void saveImage(GstCaps *caps, GstMapInfo map, string filePathName);
extern void saveCropedImage(GstCaps *caps, GstMapInfo map, string filePathName, cv::Rect roi);