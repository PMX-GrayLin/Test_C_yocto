#pragma once

#include "global.hpp"

#include <gst/gst.h>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

void saveImage(GstCaps *caps, GstMapInfo map, string filePathName);
void saveCropedImage(GstCaps *caps, GstMapInfo map, string filePathName, cv::Rect roi);
void saveCropedImage(std::string inputFilePathName, string outputFilePathName, cv::Rect roi, bool isPadding = false);

// demo
void Thread_saveImage(GstCaps *caps, GstMapInfo map, string filePathName);