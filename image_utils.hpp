#pragma once

#include <gst/gst.h>

#include "global.hpp"

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

struct SimpleRect {
    int x;
    int y;
    int width;
    int height;
};

// void imgu_saveImage(GstCaps *caps, GstMapInfo map, string filePathName);
void imgu_saveImage(void* v_caps /* GstCaps* */, void* v_map /* GstMapInfo */, const string& filePathName);
// void imgu_saveCropedImage(GstCaps *caps, GstMapInfo map, const string& filePathName, cv::Rect roi);
void imgu_saveCropedImage(void* v_caps /* GstCaps* */, void* v_map /* GstMapInfo */, const string& filePathName, SimpleRect roi);
void imgu_saveCropedImage(const string& inputFilePathName, const string& outputFilePathName, SimpleRect roi, bool isPadding = false);

// demo
void imgu_Thread_saveImage(GstCaps* caps, GstMapInfo map, string filePathName);