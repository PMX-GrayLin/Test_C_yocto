#pragma once

#include "global.hpp"

#include <gst/gst.h>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

void imgu_saveImage(GstCaps *caps, GstMapInfo map, string filePathName);
void imgu_saveImage(void* v_caps /* GstCaps */, void* v_map /* GstMapInfo */, const std::string& filePathName);
void imgu_saveCropedImage(GstCaps *caps, GstMapInfo map, string filePathName, cv::Rect roi);
void imgu_saveCropedImage(std::string inputFilePathName, string outputFilePathName, cv::Rect roi, bool isPadding = false);

// demo
void imgu_Thread_saveImage(GstCaps *caps, GstMapInfo map, string filePathName);