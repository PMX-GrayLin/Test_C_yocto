#pragma once

#include "global.hpp"

#include <gst/gst.h>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

// void imgu_saveImage(GstCaps *caps, GstMapInfo map, string filePathName);
void imgu_saveImage(void* v_caps /* GstCaps */, void* v_map /* GstMapInfo */, const std::string& filePathName);
// void imgu_saveCropedImage(GstCaps *caps, GstMapInfo map, const std::string& filePathName, cv::Rect roi);
void imgu_saveCropedImage(GstCaps *v_caps /* GstCaps */, GstMapInfo v_map /* GstMapInfo */, const std::string& filePathName, cv::Rect roi);
void imgu_saveCropedImage(const std::string& inputFilePathName, const std::string& outputFilePathName, cv::Rect roi, bool isPadding = false);

// demo
void imgu_Thread_saveImage(GstCaps *caps, GstMapInfo map, string filePathName);