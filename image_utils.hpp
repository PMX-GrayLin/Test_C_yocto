#pragma once

#include "global.hpp"

#include <mutex>
#include <condition_variable>

struct SimpleRect {
    int x;
    int y;
    int width;
    int height;
};

struct SyncSignal {
  std::mutex mtx;
  std::condition_variable cv;
  bool done = false;
};

extern SyncSignal syncSignal_save;

void imgu_resetSignal();
void imgu_waitSignal();

// save whole image or the cropped image
void imgu_saveImage(
    void* v_pad /* GstPad* */,
    void* v_info /* GstPadProbeInfo */,
    const std::string& filePathName,
    const SimpleRect roi = {0, 0, 0, 0}
);

void imgu_saveImage_thread(
    void* v_pad /* GstPad* */,
    void* v_info /* GstPadProbeInfo */,
    const std::string& filePathName,
    const SimpleRect roi = {0, 0, 0, 0},
    SyncSignal *sync = nullptr
);

void imgu_saveCropedImage(
    const std::string& inputFilePathName,
    const std::string& outputFilePathName,
    SimpleRect roi,
    bool isPadding = false
);


