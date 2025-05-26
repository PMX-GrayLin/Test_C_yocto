#pragma once

#include "global.hpp"

// Forward declarations to avoid including heavy headers
struct _GstCaps;
typedef _GstCaps GstCaps;

struct _GstMapInfo;
typedef _GstMapInfo GstMapInfo;

namespace cv {
    class Rect;
}

extern void saveImage(GstCaps *caps, GstMapInfo map, string filePathName);
extern void saveCropedImage(GstCaps *caps, GstMapInfo map, string filePathName, cv::Rect roi);