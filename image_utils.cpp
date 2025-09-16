#include "image_utils.hpp"

#include <vector>

#include <gst/gst.h>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

SyncSignal syncSignal_save;
SyncSignal syncSignal_crop;

void imgu_resetSignal(SyncSignal *sync) {
  if (sync) {
    std::lock_guard<std::mutex> lock(sync->mtx);
    sync->done = false;
  }
}
void imgu_waitSignal(SyncSignal *sync) {
  if (sync) {
    std::unique_lock<std::mutex> lock(sync->mtx);
    sync->cv.wait(lock, [&sync] { return sync->done; });
  }
}
void imgu_trigerSignal(SyncSignal *sync) {
  if (sync) {
    std::lock_guard<std::mutex> lock(sync->mtx);
    sync->done = true;
    sync->cv.notify_one();
  }
}

void imgu_saveImage_mat(
    cv::Mat &frame,
    string filePathName) 
{
  if (frame.empty()) {
    xlog("frame is empty. Cannot save image to %s", filePathName.c_str());
  } else {
    // save full image
    std::vector<int> params;

    // Lowercase file extension check (C++17 compatible)
    std::string lower_path = filePathName;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);

    if (lower_path.size() >= 4 && lower_path.substr(lower_path.size() - 4) == ".png") {
      params = {cv::IMWRITE_PNG_COMPRESSION, 0};  // Fastest (no compression)
    } else if (
        (lower_path.size() >= 4 && lower_path.substr(lower_path.size() - 4) == ".jpg") ||
        (lower_path.size() >= 5 && lower_path.substr(lower_path.size() - 5) == ".jpeg")) {
      params = {cv::IMWRITE_JPEG_QUALITY, 95};  // Quality: 0–100 (default is 95)
    }

    try {
      bool isSaveOK;
      if (!params.empty()) {
        isSaveOK = cv::imwrite(filePathName, frame, params);
      } else {
        isSaveOK = cv::imwrite(filePathName, frame);
      }
      xlog("%s frame to %s", isSaveOK ? "Saved" : "Failed to save", filePathName.c_str());
    } catch (const cv::Exception &e) {
      xlog("cv::imwrite exception: %s", e.what());
    }
  }
}

void imgu_saveImage(
  void *v_pad /* GstPad* */,
  void *v_info /* GstPadProbeInfo */,
  const std::string &filePathName) 
{
  auto start = std::chrono::high_resolution_clock::now();

  GstPad *pad = static_cast<GstPad *>(v_pad);
  GstPadProbeInfo *info = static_cast<GstPadProbeInfo *>(v_info);

  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
  if (buffer == nullptr) {
    xlog("Failed to get buffer");
    return;
  }

  // Get the capabilities of the pad to understand the format
  GstCaps *caps = gst_pad_get_current_caps(pad);
  if (!caps) {
    xlog("Failed to get caps");
    return;
  }
  // Print the entire caps for debugging
  // xlog("caps: %s", gst_caps_to_string(caps));

  // Get the structure of the first capability (format)
  GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *format = gst_structure_get_string(str, "format");
  if (!format) {
    xlog("Failed to get format from caps");
    gst_caps_unref(caps);
    return;
  }
  // xlog("format:%s", format);

  int width = 0, height = 0;
  if (!gst_structure_get_int(str, "width", &width) ||
      !gst_structure_get_int(str, "height", &height)) {
    xlog("Failed to get video dimensions");
    gst_caps_unref(caps);
    return;
  }

  // Map the buffer to access its data
  GstMapInfo map;
  if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    xlog("Failed to map buffer");
    gst_caps_unref(caps);
    return;
  }

  // Prepare cv::Mat
  cv::Mat bgr_frame;
  if (g_strcmp0(format, "NV12") == 0) {
    xlog("NV12");
    cv::Mat nv12_frame(height + height / 2, width, CV_8UC1, map.data);
    bgr_frame.create(height, width, CV_8UC3);
    cv::cvtColor(nv12_frame, bgr_frame, cv::COLOR_YUV2BGR_NV12);
  } else if (g_strcmp0(format, "I420") == 0) {
    xlog("I420");
    cv::Mat i420_frame(height + height / 2, width, CV_8UC1, map.data);
    bgr_frame.create(height, width, CV_8UC3);
    cv::cvtColor(i420_frame, bgr_frame, cv::COLOR_YUV2BGR_I420);
  } else {
    xlog("Unsupported format: %s", format);
    gst_buffer_unmap(buffer, &map);
    gst_caps_unref(caps);
    return;
  }

  imgu_saveImage_mat(bgr_frame, filePathName);

  // if (bgr_frame.empty()) {
  //   xlog("bgr_frame is empty. Cannot save image to %s", filePathName.c_str());
  // } else {
  //   // save full image
  //   std::vector<int> params;

  //   // Lowercase file extension check (C++17 compatible)
  //   std::string lower_path = filePathName;
  //   std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);

  //   if (lower_path.size() >= 4 && lower_path.substr(lower_path.size() - 4) == ".png") {
  //     params = {cv::IMWRITE_PNG_COMPRESSION, 0};  // Fastest (no compression)
  //   } else if (
  //       (lower_path.size() >= 4 && lower_path.substr(lower_path.size() - 4) == ".jpg") ||
  //       (lower_path.size() >= 5 && lower_path.substr(lower_path.size() - 5) == ".jpeg")) {
  //     params = {cv::IMWRITE_JPEG_QUALITY, 95};  // Quality: 0–100 (default is 95)
  //   }

  //   try {
  //     bool isSaveOK;
  //     if (!params.empty()) {
  //       isSaveOK = cv::imwrite(filePathName, bgr_frame, params);
  //     } else {
  //       isSaveOK = cv::imwrite(filePathName, bgr_frame);
  //     }
  //     xlog("%s frame to %s", isSaveOK ? "Saved" : "Failed to save", filePathName.c_str());
  //   } catch (const cv::Exception &e) {
  //     xlog("cv::imwrite exception: %s", e.what());
  //   }
  // }

  auto end = std::chrono::high_resolution_clock::now();
  xlog("Elapsed time: %ld ms", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

  // Cleanup
  gst_buffer_unmap(buffer, &map);
  gst_caps_unref(caps);
}

void imgu_saveImage_thread(
  void *v_pad, 
  void *v_info, 
  const std::string &filePathName,
  SyncSignal *sync) 
{
  GstPad *pad = static_cast<GstPad *>(v_pad);
  GstPadProbeInfo *info = static_cast<GstPadProbeInfo *>(v_info);

  // Get the buffer from the probe info
  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
  if (!buffer) {
    xlog("No buffer in probe info");
    return;
  }

  // Deep copy the buffer so it can be safely used in a detached thread
  GstBuffer *copied_buffer = gst_buffer_copy_deep(buffer);
  if (!copied_buffer) {
    xlog("Failed to copy buffer");
    return;
  }

  // Launch thread using copied buffer and pad
  std::thread([pad, copied_buffer, filePathName, sync]() {
    // Create a temporary GstPadProbeInfo-like struct
    GstPadProbeInfo fake_info = {};
    GST_PAD_PROBE_INFO_DATA(&fake_info) = copied_buffer;

    // Use fake_info to call the image save function
    imgu_saveImage((void *)pad, (void *)&fake_info, filePathName);

    // Cleanup
    gst_buffer_unref(copied_buffer);

    // Signal if sync object provided & job done
    imgu_trigerSignal(sync);
  }).detach();
}

void imgu_cropImage(
    void *v_pad, void *v_info,
    const std::string &filePathName,
    const SimpleRect roi) 
{
  auto start = std::chrono::high_resolution_clock::now();

  GstPad *pad = static_cast<GstPad *>(v_pad);
  GstPadProbeInfo *info = static_cast<GstPadProbeInfo *>(v_info);
  cv::Rect cv_roi(roi.x, roi.y, roi.width, roi.height);

  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
  if (buffer == nullptr) {
    xlog("Failed to get buffer");
    return;
  }

  // Get the capabilities of the pad to understand the format
  GstCaps *caps = gst_pad_get_current_caps(pad);
  if (!caps) {
    xlog("Failed to get caps");
    return;
  }
  // Print the entire caps for debugging
  // gchar *caps_str = gst_caps_to_string(caps);
  // xlog("caps: %s", caps_str);
  // g_free(caps_str);

  // Get the structure of the first capability (format)
  GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *format = gst_structure_get_string(str, "format");
  if (!format) {
    xlog("Failed to get format from caps");
    gst_caps_unref(caps);
    return;
  }
  // xlog("format:%s", format);

  int width = 0, height = 0;
  if (!gst_structure_get_int(str, "width", &width) ||
      !gst_structure_get_int(str, "height", &height)) {
    xlog("Failed to get video dimensions");
    gst_caps_unref(caps);
    return;
  }

  // Map the buffer to access its data
  GstMapInfo map;
  if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    xlog("Failed to map buffer");
    gst_caps_unref(caps);
    return;
  }

  // Prepare cv::Mat
  cv::Mat bgr_frame;
  if (g_strcmp0(format, "NV12") == 0) {
    cv::Mat nv12_frame(height + height / 2, width, CV_8UC1, map.data);
    bgr_frame.create(height, width, CV_8UC3);
    cv::cvtColor(nv12_frame, bgr_frame, cv::COLOR_YUV2BGR_NV12);
  } else if (g_strcmp0(format, "I420") == 0) {
    cv::Mat i420_frame(height + height / 2, width, CV_8UC1, map.data);
    bgr_frame.create(height, width, CV_8UC3);
    cv::cvtColor(i420_frame, bgr_frame, cv::COLOR_YUV2BGR_I420);
  } else {
    xlog("Unsupported format: %s", format);
    gst_buffer_unmap(buffer, &map);
    gst_caps_unref(caps);
    return;
  }

  if (bgr_frame.empty()) {
    xlog("bgr_frame is empty. Cannot save image to %s", filePathName.c_str());
  } else {

    // save crop image
    std::vector<int> params;

    // Lowercase file extension check (C++17 compatible)
    std::string lower_path = filePathName;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);

    if (lower_path.size() >= 4 && lower_path.substr(lower_path.size() - 4) == ".png") {
      params = {cv::IMWRITE_PNG_COMPRESSION, 0};  // Fastest (no compression)
    } else if (
        (lower_path.size() >= 4 && lower_path.substr(lower_path.size() - 4) == ".jpg") ||
        (lower_path.size() >= 5 && lower_path.substr(lower_path.size() - 5) == ".jpeg")) {
      params = {cv::IMWRITE_JPEG_QUALITY, 95};  // Quality: 0–100 (default is 95)
    }
    
    // Validate ROI dimensions
    bool isCrop = true;
    if (cv_roi.x < 0 || cv_roi.y < 0 || cv_roi.width == 0 || cv_roi.height == 0 ||
        cv_roi.x + cv_roi.width > bgr_frame.cols ||
        cv_roi.y + cv_roi.height > bgr_frame.rows) {
      xlog("ROI not correct... not crop");
      isCrop = false;
    }

    if (isCrop) {
      // Crop the region of interest (ROI)
      cv::Mat croppedImage = bgr_frame(cv_roi);

      // Create a black canvas of the target size
      int squqareSize = (croppedImage.cols > croppedImage.rows) ? croppedImage.cols : croppedImage.rows;
      cv::Size paddingSize(squqareSize, squqareSize);
      cv::Mat paddedImage = cv::Mat::zeros(paddingSize, croppedImage.type());

      // Calculate offsets to center the cropped image
      int offsetX = (paddedImage.cols - croppedImage.cols) / 2;
      int offsetY = (paddedImage.rows - croppedImage.rows) / 2;
      // Check if offsets are valid
      if (offsetX < 0 || offsetY < 0) {
        xlog("Error: Cropped image is larger than the padding canvas!");
        return;
      }

      // Define the ROI on the black canvas where the cropped image will be placed
      cv::Rect roi_padding(offsetX, offsetY, croppedImage.cols, croppedImage.rows);

      // Copy the cropped image onto the black canvas
      croppedImage.copyTo(paddedImage(roi_padding));

      // Attempt to save the image
      try {
        bool isSaveOK;
        if (!params.empty()) {
          isSaveOK = cv::imwrite(filePathName, paddedImage, params);
        } else {
          isSaveOK = cv::imwrite(filePathName, paddedImage);
        }
        xlog("%s frame to %s", isSaveOK ? "Saved" : "Failed to save", filePathName.c_str());
      } catch (const cv::Exception &e) {
        xlog("cv::imwrite exception: %s", e.what());
      }

    } else {
        // save the image
        try {
          bool isSaveOK;
          if (!params.empty()) {
            isSaveOK = cv::imwrite(filePathName, bgr_frame, params);
          } else {
            isSaveOK = cv::imwrite(filePathName, bgr_frame);
          }
          xlog("%s frame to %s", isSaveOK ? "Saved" : "Failed to save", filePathName.c_str());
        } catch (const cv::Exception &e) {
          xlog("cv::imwrite exception: %s", e.what());
        }
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  xlog("Elapsed time: %ld ms", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

  // Cleanup
  gst_buffer_unmap(buffer, &map);
  gst_caps_unref(caps);
    
}

void imgu_cropImage_thread(
  void *v_pad, void *v_info, 
  const std::string &filePathName, 
  const SimpleRect roi, 
  SyncSignal *sync) 
{
  GstPad *pad = static_cast<GstPad *>(v_pad);
  GstPadProbeInfo *info = static_cast<GstPadProbeInfo *>(v_info);

  // Get the buffer from the probe info
  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
  if (!buffer) {
    xlog("No buffer in probe info");
    return;
  }

  // Deep copy the buffer so it can be safely used in a detached thread
  GstBuffer *copied_buffer = gst_buffer_copy_deep(buffer);
  if (!copied_buffer) {
    xlog("Failed to copy buffer");
    return;
  }

  // Launch thread using copied buffer and pad
  std::thread([pad, copied_buffer, filePathName, roi, sync]() {
    // Create a temporary GstPadProbeInfo-like struct
    GstPadProbeInfo fake_info = {};
    GST_PAD_PROBE_INFO_DATA(&fake_info) = copied_buffer;

    // Use fake_info to call the image save function
    imgu_cropImage((void *)pad, (void *)&fake_info, filePathName, roi);

    // Cleanup
    gst_buffer_unref(copied_buffer);

    // Signal if sync object provided & job done
    imgu_trigerSignal(sync);
  }).detach();
}

void imgu_saveCropedImage(const std::string &inputFilePathName, const std::string &outputFilePathName, SimpleRect roi, bool isPadding) {

  cv::Rect cv_roi(roi.x, roi.y, roi.width, roi.height);

  // Load the image
  cv::Mat image = cv::imread(inputFilePathName);
  if (image.empty()) {
    xlog("Failed to load image from %s", inputFilePathName.c_str());
    return;
  }

  // Validate ROI dimensions
  bool isCrop = true;
  if (cv_roi.x < 0 || cv_roi.y < 0 || cv_roi.width == 0 || cv_roi.height == 0 ||
      cv_roi.x + cv_roi.width > image.cols ||
      cv_roi.y + cv_roi.height > image.rows) {
    xlog("ROI not correct... not crop");
    isCrop = false;
  }

  if (isCrop) {
    // Crop the region of interest (ROI)
    cv::Mat croppedImage = image(cv_roi);

    if (isPadding) {
      // Create a black canvas of the target size
      int squqareSize = (croppedImage.cols > croppedImage.rows) ? croppedImage.cols : croppedImage.rows;
      cv::Size paddingSize(squqareSize, squqareSize);
      cv::Mat paddedImage = cv::Mat::zeros(paddingSize, croppedImage.type());

      // Calculate offsets to center the cropped image
      int offsetX = (paddedImage.cols - croppedImage.cols) / 2;
      int offsetY = (paddedImage.rows - croppedImage.rows) / 2;
      // Check if offsets are valid
      if (offsetX < 0 || offsetY < 0) {
        xlog("Error: Cropped image is larger than the padding canvas!");
        return;
      }

      // Define the ROI on the black canvas where the cropped image will be placed
      cv::Rect roi_padding(offsetX, offsetY, croppedImage.cols, croppedImage.rows);

      // Copy the cropped image onto the black canvas
      croppedImage.copyTo(paddedImage(roi_padding));

      // Attempt to save the image
      if (cv::imwrite(outputFilePathName, paddedImage)) {
        xlog("Saved crop frame to %s", outputFilePathName.c_str());
      } else {
        xlog("Failed crop to save frame to %s", outputFilePathName.c_str());
      }

    } else {
      // Attempt to save the image
      if (cv::imwrite(outputFilePathName, croppedImage)) {
        xlog("Saved crop frame to %s", outputFilePathName.c_str());
      } else {
        xlog("Failed crop to save frame to %s", outputFilePathName.c_str());
      }
    }

  } else {
    // save the image
    if (cv::imwrite(outputFilePathName, image)) {
      xlog("Saved crop frame to %s", outputFilePathName.c_str());
    } else {
      xlog("Failed crop to save frame to %s", outputFilePathName.c_str());
    }
  }
}

