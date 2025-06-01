#pragma once

#include "global.hpp"

struct SimpleRect {
    int x;
    int y;
    int width;
    int height;
};

void imgu_saveImage(
    void* v_pad /* GstPad* */,
    void* v_info /* GstPadProbeInfo */,
    const std::string& filePathName,
    const SimpleRect roi = {0, 0, 0, 0}
);

void imgu_saveImage_thread(
    void* v_pad /* GstPad* */,
    void* v_info /* GstPadProbeInfo */,
    const std::string& filePathName
);

void imgu_saveCropedImage(void* v_caps /* GstCaps* */, void* v_map /* GstMapInfo */, const std::string& filePathName, SimpleRect roi);

void imgu_saveCropedImage(const std::string& inputFilePathName, const std::string& outputFilePathName, SimpleRect roi, bool isPadding = false);

