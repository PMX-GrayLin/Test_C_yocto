#include "image_utils.hpp"

#include <gst/gst.h>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

void imgu_saveImage(void *v_caps, void *v_map, const std::string &filePathName) {
  GstCaps *caps = static_cast<GstCaps *>(v_caps);
  GstMapInfo *map = static_cast<GstMapInfo *>(v_map);

  // Get the structure of the first capability (format)
  GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *format = gst_structure_get_string(str, "format");
  xlog("format:%s", format);

  int width = 0, height = 0;
  cv::Mat bgr_frame;

  // Only proceed if the format is NV12
  if (format && g_strcmp0(format, "NV12") == 0) {
    if (!gst_structure_get_int(str, "width", &width) ||
        !gst_structure_get_int(str, "height", &height)) {
      xlog("Failed to get video dimensions");
    }
    // xlog("Video dimensions: %dx%d", width, height);

    // Convert NV12 to BGR
    cv::Mat nv12_frame(height + height / 2, width, CV_8UC1, map->data);
    bgr_frame.create(height, width, CV_8UC3);
    cv::cvtColor(nv12_frame, bgr_frame, cv::COLOR_YUV2BGR_NV12);

  } else if (format && g_strcmp0(format, "I420") == 0) {
    if (!gst_structure_get_int(str, "width", &width) ||
        !gst_structure_get_int(str, "height", &height)) {
      xlog("Failed to get video dimensions");
    }

    // Convert I420 to BGR
    cv::Mat i420_frame(height + height / 2, width, CV_8UC1, map->data);
    bgr_frame.create(height, width, CV_8UC3);
    cv::cvtColor(i420_frame, bgr_frame, cv::COLOR_YUV2BGR_I420);

  } else {
    xlog("Unsupported format: %s", format ? format : "NULL");
    return;
  }

  // Check if bgr_frame is valid
  if (bgr_frame.empty()) {
    xlog("bgr_frame is empty. Cannot save image to %s", filePathName.c_str());
    return;
  }

  // save the image
  if (cv::imwrite(filePathName, bgr_frame)) {
    xlog("Saved frame to %s", filePathName.c_str());
  } else {
    xlog("Failed to save frame to %s", filePathName.c_str());
  }
}

void imgu_saveCropedImage(void *v_caps, void *v_map, const std::string &filePathName, SimpleRect roi) {
  GstCaps *caps = static_cast<GstCaps *>(v_caps);
  GstMapInfo *map = static_cast<GstMapInfo *>(v_map);
  cv::Rect cv_roi(roi.x, roi.y, roi.width, roi.height);

  // Get the structure of the first capability (format)
  GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *format = gst_structure_get_string(str, "format");
  xlog("format:%s", format);

  int width = 0, height = 0;
  cv::Mat bgr_frame;

  // Only proceed if the format is NV12
  if (format && g_strcmp0(format, "NV12") == 0) {
    if (!gst_structure_get_int(str, "width", &width) ||
        !gst_structure_get_int(str, "height", &height)) {
      xlog("Failed to get video dimensions");
    }
    // xlog("Video dimensions: %dx%d", width, height);

    // Convert NV12 to BGR
    cv::Mat nv12_frame(height + height / 2, width, CV_8UC1, map->data);
    bgr_frame.create(height, width, CV_8UC3);
    cv::cvtColor(nv12_frame, bgr_frame, cv::COLOR_YUV2BGR_NV12);

  } else if (format && g_strcmp0(format, "I420") == 0) {
    if (!gst_structure_get_int(str, "width", &width) ||
        !gst_structure_get_int(str, "height", &height)) {
      xlog("Failed to get video dimensions");
    }

    // Convert I420 to BGR
    cv::Mat i420_frame(height + height / 2, width, CV_8UC1, map->data);
    bgr_frame.create(height, width, CV_8UC3);
    cv::cvtColor(i420_frame, bgr_frame, cv::COLOR_YUV2BGR_I420);

  } else {
    xlog("Unsupported format: %s", format ? format : "NULL");
    return;
  }

  // Check if frameBuffer is valid
  if (bgr_frame.empty()) {
    xlog("Frame buffer is empty. Cannot save image to %s", filePathName.c_str());
    return;
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
    if (cv::imwrite(filePathName, paddedImage)) {
      xlog("Saved crop frame to %s", filePathName.c_str());
    } else {
      xlog("Failed crop to save frame to %s", filePathName.c_str());
    }

  } else {
    // save the image
    if (cv::imwrite(filePathName, bgr_frame)) {
      xlog("Saved crop frame to %s", filePathName.c_str());
    } else {
      xlog("Failed crop to save frame to %s", filePathName.c_str());
    }
  }
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

// void imgu_Thread_saveImage(GstCaps *caps, GstMapInfo map, string filePathName) {
//   // Copy map data
//   GstMapInfo copiedMap = map;
//   copiedMap.data = (guint8 *)malloc(map.size);
//   memcpy(copiedMap.data, map.data, map.size);

//   // Launch the thread
//   std::thread t([=]() {
//     imgu_saveImage((void *)caps, (void *)&copiedMap, filePathName);
//     free(copiedMap.data);  // Clean up manually
//   });
//   t.detach();  // Or .join() depending on use case
// }
