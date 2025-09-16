#pragma once

#include "global.hpp"

#include <mutex>
#include <condition_variable>

#include <opencv2/core.hpp>

typedef enum {
    spf_BMP,
    spf_JPEG,
    spf_PNG,
} SavedPhotoFormat;

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
extern SyncSignal syncSignal_crop;

void imgu_resetSignal(SyncSignal *sync);
void imgu_waitSignal(SyncSignal *sync);

bool imgu_saveImage_mat(
    cv::Mat &frame,
    const std::string &filePathName
);

// save whole image or the cropped image
void imgu_saveImage(
    void* v_pad /* GstPad* */,
    void* v_info /* GstPadProbeInfo */,
    const std::string& filePathName
);

void imgu_saveImage_thread(
    void* v_pad /* GstPad* */,
    void* v_info /* GstPadProbeInfo */,
    const std::string& filePathName,
    SyncSignal *sync = nullptr
);

// cropped image
void imgu_cropImage(
    void* v_pad /* GstPad* */,
    void* v_info /* GstPadProbeInfo */,
    const std::string& filePathName,
    const SimpleRect roi = {0, 0, 0, 0}
);

void imgu_cropImage_thread(
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


