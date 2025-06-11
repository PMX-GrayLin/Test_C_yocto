#include "cam_omnivision.hpp"

#include <linux/videodev2.h>  // For V4L2 definitions
#include <sys/ioctl.h>        // For ioctl()
#include <fcntl.h>            // For open()

#include <iomanip>

#include <gst/gst.h>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

std::thread t_aicamera_streaming;
bool isStreaming = false;

bool isCapturePhoto = false;
bool isCropPhoto = false;
bool isPaddingPhoto = false;
SavedPhotoFormat savedPhotoFormat = spf_PNG;
std::string pathName_savedImage = "";
std::string pathName_croppedImage = "";
std::string pathName_inputImage = "";
cv::Rect crop_roi(0, 0, 0, 0);

static volatile int counterFrame = 0;
static int counterImg = 0;

static GstElement *gst_pipeline = nullptr;
static GMainLoop *gst_loop = nullptr;

namespace fs = std::filesystem;

void CIS_handle_RESTful(std::vector<std::string> segments) {
  if (isSameString(segments[1], "start")) {
    AICamera_streamingStart();
  } else if (isSameString(segments[1], "stop")) {
    AICamera_streamingStop();

  } else if (isSameString(segments[1], "tp")) {
    xlog("take picture");
    std::string path = "";
    if (segments.size() > 2 && !segments[2].empty()) {
      //ex: curl http://localhost:8765/fw/gige/tp/%252Fhome%252Froot%252Fprimax%252F12345.png
      path = segments[2];
      const std::string from = "%2F";
      const std::string to = "/";
      size_t start_pos = 0;
      while ((start_pos = path.find(from, start_pos)) != std::string::npos) {
        path.replace(start_pos, from.length(), to);
        start_pos += to.length();
      }

    } else {
      path = "/home/root/primax/fw_" + getTimeString() + ".png";
    }
    AICamera_setImagePath(path.c_str());
    AICamera_captureImage();
  }
}

bool AICamrea_isUseCISCamera() {
  if (isPathExist(AICamreaCISPath)) {
    xlog("path /dev/csi_cam_preview exist");
    return true;
  } else {
    xlog("path /dev/csi_cam_preview not exist");
    return false;
  }
}

std::string AICamrea_getVideoDevice() {
  std::string videoPath;

  bool isUseCSICamera = AICamrea_isUseCISCamera();

  if (isUseCSICamera) {
    videoPath = AICamreaCISPath;
  } else {
    videoPath = AICamreaUSBPath;
  }

  // #if defined(USE_USB_CAM)

  //   videoPath = "/dev/video137";

  // #else
  //   FILE *pipe = popen("v4l2-ctl --list-devices | grep mtk-v4l2-camera -A 3", "r");
  //   if (pipe) {
  //     std::string output;
  //     char buffer[256];
  //     while (fgets(buffer, sizeof(buffer), pipe)) {
  //       output += buffer;
  //     }
  //     pclose(pipe);

  //     std::regex device_regex("/dev/video\\d+");
  //     std::smatch match;
  //     if (std::regex_search(output, match, device_regex)) {
  //       videoPath = match[0];
  //     }
  //   }

  // #endif

  xlog("videoPath:%s", videoPath.c_str());
  return videoPath;
}

int ioctl_get_value(int control_ID) {
  int fd = open(AICamrea_getVideoDevice().c_str(), O_RDWR);
  if (fd == -1) {
    xlog("Failed to open video device:%s", strerror(errno));
    return -1;
  }

  struct v4l2_queryctrl queryctrl = {};
  // memset(&queryctrl, 0, sizeof(queryctrl));
  queryctrl.id = control_ID;
  if (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
    xlog("queryctrl.minimum:%d", queryctrl.minimum);
    xlog("queryctrl.maximum:%d", queryctrl.maximum);
  } else {
    xlog("ioctl fail, VIDIOC_QUERYCTRL... error:%s", strerror(errno));
  }

  struct v4l2_control ctrl = {};
  // memset(&ctrl, 0, sizeof(ctrl));
  ctrl.id = control_ID;
  if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) == 0) {
    xlog("ctrl.id:0x%x, ctrl.value:%d", ctrl.id, ctrl.value);
  } else {
    xlog("ioctl fail, VIDIOC_G_CTRL... error:%s", strerror(errno));
  }

  return ctrl.value;
}

int ioctl_set_value(int control_ID, int value) {
  int fd = open(AICamrea_getVideoDevice().c_str(), O_RDWR);
  if (fd == -1) {
    xlog("Failed to open video device:%s", strerror(errno));
    return -1;
  }

  struct v4l2_control ctrl = {};
  // memset(&ctrl, 0, sizeof(ctrl));
  ctrl.id = control_ID;
  ctrl.value = value;

  if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) == 0) {
    xlog("ctrl.id:0x%x, set to:%d", ctrl.id, ctrl.value);
  } else {
    xlog("fail to set value, error:%s", strerror(errno));
  }
  close(fd);

  return 0;
}

int AICamera_getBrightness() {
  return ioctl_get_value(V4L2_CID_BRIGHTNESS);
}

void AICamera_setBrightness(int value) {
  xlog("value:%d", value);
  ioctl_set_value(V4L2_CID_BRIGHTNESS, value);
}

int AICamera_getContrast() {
  return ioctl_get_value(V4L2_CID_CONTRAST);
}

void AICamera_setContrast(int value) {
  xlog("value:%d", value);
  ioctl_set_value(V4L2_CID_CONTRAST, value);
}

int AICamera_getSaturation() {
  return ioctl_get_value(V4L2_CID_SATURATION);
}

void AICamera_setSaturation(int value) {
  xlog("value:%d", value);
  ioctl_set_value(V4L2_CID_SATURATION, value);
}

int AICamera_getHue() {
  return ioctl_get_value(V4L2_CID_HUE);
}

void AICamera_setHue(int value) {
  xlog("value:%d", value);
  ioctl_set_value(V4L2_CID_HUE, value);
}

// command example
// v4l2-ctl -d 78 --get-ctrl=white_balance_automatic
int AICamera_getWhiteBalanceAutomatic() {
  return ioctl_get_value(V4L2_CID_AUTO_WHITE_BALANCE);
}

// command example
// v4l2-ctl -d 78 --set-ctrl=white_balance_automatic=0
// v4l2-ctl -d 78 --set-ctrl=white_balance_automatic=1
void AICamera_setWhiteBalanceAutomatic(bool enable) {
  xlog("enable:%d", enable);
  ioctl_set_value(V4L2_CID_AUTO_WHITE_BALANCE, enable ? 1 : 0);
}

int AICamera_getSharpness() {
  return ioctl_get_value(V4L2_CID_SHARPNESS);
}

void AICamera_setSharpness(int value) {
  xlog("value:%d", value);
  ioctl_set_value(V4L2_CID_SHARPNESS, value);
}

int AICamera_getISO() {
  return ioctl_get_value(0x009819a9);
}

void AICamera_setISO(int value) {
  xlog("value:%d", value);
  ioctl_set_value(0x009819a9, value);
}

int AICamera_getExposure() {
  return ioctl_get_value(V4L2_CID_EXPOSURE);
}

void AICamera_setExposure(int value) {
  xlog("value:%d", value);
  ioctl_set_value(V4L2_CID_EXPOSURE, value);
}

int AICamera_getWhiteBalanceTemperature() {
  return ioctl_get_value(V4L2_CID_WHITE_BALANCE_TEMPERATURE);
}

void AICamera_setWhiteBalanceTemperature(int value) {
  xlog("value:%d", value);
  ioctl_set_value(V4L2_CID_WHITE_BALANCE_TEMPERATURE, value);
}

int AICamera_getExposureAuto() {
  return ioctl_get_value(V4L2_CID_EXPOSURE_AUTO);
}

void AICamera_setExposureAuto(bool enable) {
  xlog("enable:%d", enable);
  ioctl_set_value(V4L2_CID_EXPOSURE_AUTO, enable ? 0 : 1);
}

int AICamera_getExposureTimeAbsolute() {
  xlog("");
  return ioctl_get_value(V4L2_CID_EXPOSURE_ABSOLUTE);
}

void AICamera_setExposureTimeAbsolute(double sec) {
  xlog("tmp sec:%f", sec);
  int value = (int)(sec * 1000000.0);
  xlog("value:%d", value);
  ioctl_set_value(V4L2_CID_EXPOSURE_ABSOLUTE, value);
}

int AICamera_getFocusAbsolute() {
  return ioctl_get_value(V4L2_CID_FOCUS_ABSOLUTE);
}

void AICamera_setFocusAbsolute(int value) {
  xlog("value:%d", value);
  ioctl_set_value(V4L2_CID_FOCUS_ABSOLUTE, value);
}

int AICamera_getFocusAuto() {
  return ioctl_get_value(V4L2_CID_FOCUS_AUTO);
}

void AICamera_setFocusAuto(bool enable) {
  xlog("enable:%d", enable);
  ioctl_set_value(V4L2_CID_FOCUS_AUTO, enable ? 1 : 0);
}

void AICamera_setImagePath(const string& imagePath) {
  pathName_savedImage = imagePath;
  xlog("pathName_savedImage:%s", pathName_savedImage.c_str());
}

void AICamera_setCropImagePath(const string& imagePath) {
  pathName_croppedImage = imagePath;
  xlog("pathName_croppedImage:%s", pathName_croppedImage.c_str());
}

void AICamera_setInputImagePath(const string& imagePath) {
  pathName_inputImage = imagePath;
  xlog("pathName_inputImage:%s", pathName_inputImage.c_str());
}

void AICamera_setCropROI(cv::Rect roi) {
  crop_roi = roi;
  xlog("ROI x:%d, y:%d, w:%d, h:%d", crop_roi.x, crop_roi.y, crop_roi.width, crop_roi.height);
}

bool AICamera_isCropImage() {
  return (crop_roi != cv::Rect(0, 0, 0, 0));
}

void AICamera_captureImage() {
  if (!isStreaming) {
    xlog("do nothing...camera is not streaming");
    return;
  }
  xlog("");
  isCapturePhoto = true;
}

void AICamera_enableCrop(bool enable) {
  isCropPhoto = enable;
  xlog("isCropPhoto:%d", isCropPhoto);
}

void AICamera_enablePadding(bool enable) {
  isPaddingPhoto = enable;
  xlog("isPaddingPhoto:%d", isPaddingPhoto);
}

void AICAMERA_load_crop_saveImage() {
  // not use thread here
  // std::thread([]() {
    try {
      xlog("---- AICAMERA_load_crop_saveImage start ----");

      // Load the image
      cv::Mat image = cv::imread(pathName_inputImage);
      if (image.empty()) {
        xlog("Failed to load image from %s", pathName_inputImage.c_str());
        return;
      }

      if (isCropPhoto) {
        // Crop the region of interest (ROI)
        cv::Mat croppedImage = image(crop_roi);

        if (isPaddingPhoto) {
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
          if (cv::imwrite(pathName_savedImage, paddedImage)) {
            xlog("Saved crop frame to %s", pathName_savedImage.c_str());
          } else {
            xlog("Failed crop to save frame to %s", pathName_savedImage.c_str());
          }

        } else {
          
          // Attempt to save the image
          if (cv::imwrite(pathName_savedImage, croppedImage)) {
            xlog("Saved crop frame to %s", pathName_savedImage.c_str());
          } else {
            xlog("Failed crop to save frame to %s", pathName_savedImage.c_str());
          }
        }

        cv::Rect reset_roi(0, 0, 0, 0);
        AICamera_setCropROI(reset_roi);

      } else {
        // save the image
        if (cv::imwrite(pathName_savedImage, image)) {
          xlog("Saved crop frame to %s", pathName_savedImage.c_str());
        } else {
          xlog("Failed crop to save frame to %s", pathName_savedImage.c_str());
        }
      }

    } catch (const std::exception &e) {
      xlog("Exception during image save: %s", e.what());
    }
    xlog("---- AICAMERA_load_crop_saveImage stop ----");
  // }).detach();  // Detach to run in the background
}

void AICAMERA_threadSaveImage(const std::string path, const cv::Mat &frameBuffer) {
  std::thread([path, frameBuffer]() {
    try {
      xlog("---- AICAMERA_threadSaveImage start ----");
      // Extract directory from the full path

      // ??
      // std::string directory = fs::path(path).parent_path().string();
      std::string directory = get_parent_directory(path);
      xlog("Raw path: [%s]", path.c_str());
      xlog("Parent directory: [%s]", directory.c_str());
      if (!isPathExist(directory.c_str())) {
        xlog("Directory does not exist, creating: %s", directory.c_str());
        // ??
        // create_directories(directory);
      }

      // Check if frameBuffer is valid
      if (frameBuffer.empty()) {
        xlog("Frame buffer is empty. Cannot save image to %s", path.c_str());
        return;
      }

      // save the image
      if (cv::imwrite(path, frameBuffer)) {
        xlog("Saved frame to %s", path.c_str());
      } else {
        xlog("Failed to save frame to %s", path.c_str());
      }

    } catch (const std::exception &e) {
      xlog("Exception during image save: %s", e.what());
    }

    xlog("---- AICAMERA_threadSaveImage stop ----");
  }).detach();  // Detach to run in the background
}

void AICAMERA_threadSaveCropImage(const std::string path, const cv::Mat &frameBuffer, cv::Rect roi) {
  std::thread([path, frameBuffer, roi]() {
    try {
      xlog("---- AICAMERA_threadSaveCropImage start ----");
      bool isCrop = true;
      // bool isPadding = true;

      // Extract directory from the full path

      // ??
      // std::string directory = fs::path(path).parent_path().string();
      std::string directory = get_parent_directory(path);
      xlog("Raw path: [%s]", path.c_str());
      xlog("Parent directory: [%s]", directory.c_str());
      if (!isPathExist(directory.c_str())) {
        xlog("Directory does not exist, creating: %s", directory.c_str());
        // ??
        // create_directories(directory);
      }

      // Check if frameBuffer is valid
      if (frameBuffer.empty()) {
        xlog("Frame buffer is empty. Cannot save image to %s", path.c_str());
        return;
      }

      // Validate ROI dimensions
      if (roi.x < 0 || roi.y < 0 || roi.width == 0 || roi.height == 0 ||
          roi.x + roi.width > frameBuffer.cols ||
          roi.y + roi.height > frameBuffer.rows) {
        xlog("ROI not correct... not crop");
        isCrop = false;
      }

      if (isCrop) {
        // Crop the region of interest (ROI)
        cv::Mat croppedImage = frameBuffer(roi);

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
        if (cv::imwrite(path, paddedImage)) {
          xlog("Saved crop frame to %s", path.c_str());
        } else {
          xlog("Failed crop to save frame to %s", path.c_str());
        }

        cv::Rect reset_roi(0, 0, 0, 0);
        AICamera_setCropROI(reset_roi);

      } else {
        // save the image
        if (cv::imwrite(path, frameBuffer)) {
          xlog("Saved crop frame to %s", path.c_str());
        } else {
          xlog("Failed crop to save frame to %s", path.c_str());
        }
      }

    } catch (const std::exception &e) {
      xlog("Exception during image save: %s", e.what());
    }

    xlog("---- AICAMERA_threadSaveCropImage stop ----");
  }).detach();  // Detach to run in the background
}

void AICAMERA_saveImage(GstPad *pad, GstPadProbeInfo *info) {
  if (isCapturePhoto) {

    xlog("");
    isCapturePhoto = false;
    
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

    // Map the buffer to access its data
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
      xlog("Failed to map buffer");
      gst_caps_unref(caps);
      return;
    }

    // Get the structure of the first capability (format)
    GstStructure *str = gst_caps_get_structure(caps, 0);
    const gchar *format = gst_structure_get_string(str, "format");
    xlog("format:%s", format);

    // Only proceed if the format is NV12
    if (format && g_strcmp0(format, "NV12") == 0) {
      int width = 0, height = 0;
      if (!gst_structure_get_int(str, "width", &width) ||
          !gst_structure_get_int(str, "height", &height)) {
        xlog("Failed to get video dimensions");
      }
      // xlog("Video dimensions: %dx%d", width, height);

      // Create a cv::Mat to store the frame in NV12 format
      cv::Mat nv12_frame(height + height / 2, width, CV_8UC1, map.data);
      // Create a cv::Mat to store the frame in BGR format
      cv::Mat bgr_frame(height, width, CV_8UC3);
      // Convert NV12 to BGR using OpenCV
      cv::cvtColor(nv12_frame, bgr_frame, cv::COLOR_YUV2BGR_NV12);

      // Save the frame to a picture
      counterImg++;
      std::ostringstream oss;
      std::string filename = "";

      if (pathName_savedImage == "") {
        if (savedPhotoFormat == spf_BMP) {
          oss << "frame_" << std::setw(5) << std::setfill('0') << counterImg << ".bmp";
        } else if (savedPhotoFormat == spf_JPEG) {
          oss << "frame_" << std::setw(5) << std::setfill('0') << counterImg << ".jpg";
        } else {
          oss << "frame_" << std::setw(5) << std::setfill('0') << counterImg << ".png";
        }
        filename = oss.str();
      } else {
        filename = pathName_savedImage;
      }

      AICAMERA_threadSaveImage(filename, bgr_frame);
      AICAMERA_threadSaveCropImage(pathName_croppedImage, bgr_frame, crop_roi);
      
    } else if (format && g_strcmp0(format, "I420") == 0) {
      int width = 0, height = 0;
      if (!gst_structure_get_int(str, "width", &width) ||
          !gst_structure_get_int(str, "height", &height)) {
        xlog("Failed to get video dimensions");
      }

      // Convert I420 to BGR using OpenCV
      cv::Mat i420_frame(height + height / 2, width, CV_8UC1, map.data);
      cv::Mat bgr_frame(height, width, CV_8UC3);
      cv::cvtColor(i420_frame, bgr_frame, cv::COLOR_YUV2BGR_I420);

      // Save the frame
      counterImg++;
      std::ostringstream oss;
      std::string filename = "";

      if (pathName_savedImage == "") {
        if (savedPhotoFormat == spf_BMP) {
          oss << "frame_" << std::setw(5) << std::setfill('0') << counterImg << ".bmp";
        } else if (savedPhotoFormat == spf_JPEG) {
          oss << "frame_" << std::setw(5) << std::setfill('0') << counterImg << ".jpg";
        } else {
          oss << "frame_" << std::setw(5) << std::setfill('0') << counterImg << ".png";
        }
        filename = oss.str();
      } else {
        filename = pathName_savedImage;
      }

      AICAMERA_threadSaveImage(filename, bgr_frame);
      AICAMERA_threadSaveCropImage(pathName_croppedImage, bgr_frame, crop_roi);
    }

    // Cleanup
    gst_buffer_unmap(buffer, &map);
    gst_caps_unref(caps);
  }
}

// Callback to handle incoming buffer data
GstPadProbeReturn AICAMERA_streamingDataCallback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  AICAMERA_saveImage(pad, info);
  return GST_PAD_PROBE_OK;
}

void ThreadAICameraStreaming() {
  xlog("++++ start ++++");
  counterFrame = 0;
  counterImg = 0;

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // final gst pipeline
  // gst-launch-1.0 v4l2src device=${VIDEO_DEV[0]} ! video/x-raw,width=2048,height=1536 ! queue ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! h264parse config-interval=1 ! rtspclientsink location=rtsp://localhost:8554/mystream

  // Create the elements
  gst_pipeline = gst_pipeline_new("video-pipeline");
  GstElement *source = gst_element_factory_make("v4l2src", "source");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  GstElement *queue = gst_element_factory_make("queue", "queue");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *parser = gst_element_factory_make("h264parse", "parser");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!gst_pipeline || !source || !capsfilter || !queue || !encoder || !parser || !sink) {
    xlog("failed to create GStreamer elements");
    return;
  }

  // Set properties for the elements
  g_object_set(G_OBJECT(source), "device", AICamrea_getVideoDevice().c_str(), nullptr);

  // Define the capabilities for the capsfilter
  // 5M : 2592 * 1944
  // AD : 2048 * 1536
  // elic : 1920 * 1080
  GstCaps *caps = gst_caps_new_simple(
      "video/x-raw",
      "width", G_TYPE_INT, 2592,
      "height", G_TYPE_INT, 1944,
      "framerate", GST_TYPE_FRACTION, 30, 1,  // Add frame rate as 30/1
      nullptr);
  g_object_set(capsfilter, "caps", caps, nullptr);
  gst_caps_unref(caps);

  // Create a GstStructure for extra-controls
  GstStructure *controls = gst_structure_new(
      "extra-controls",                  // Name of the structure
      "video_gop_size", G_TYPE_INT, 30,  // Key-value pair
      // "h264_level", G_TYPE_INT, 13,      // Key-value pair
      nullptr  // End of key-value pairs
  );
  if (!controls) {
    xlog("Failed to create GstStructure");
    gst_object_unref(gst_pipeline);
    return;
  }
  g_object_set(G_OBJECT(encoder), "extra-controls", controls, nullptr);
  // Free the GstStructure after use
  gst_structure_free(controls);

  g_object_set(encoder, "capture-io-mode", 4, nullptr);  // dmabuf = 4
  g_object_set(parser, "config-interval", 1, nullptr); // send SPS/PPS every keyframe
  g_object_set(sink, "location", "rtsp://localhost:8554/mystream", nullptr);

  // Build the pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, capsfilter, queue, encoder, parser, sink, nullptr);
  if (!gst_element_link_many(source, capsfilter, queue, encoder, parser, sink, nullptr)) {
    xlog("failed to link elements in the pipeline");
    gst_object_unref(gst_pipeline);
    return;
  }

  // Attach pad probe to capture frames
  GstPad *pad = gst_element_get_static_pad(encoder, "sink");
  if (pad) {
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)AICAMERA_streamingDataCallback, nullptr, nullptr);
    gst_object_unref(pad);
  }

  // Add bus watch to handle errors and EOS
  GstBus *bus = gst_element_get_bus(gst_pipeline);
  gst_bus_add_watch(bus, [](GstBus *, GstMessage *msg, gpointer user_data) -> gboolean {
      switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
          GError *err;
          gchar *dbg;
          gst_message_parse_error(msg, &err, &dbg);
          xlog("Pipeline error: %s", err->message);
          g_error_free(err);
          g_free(dbg);
  
          // GMainLoop *loop = static_cast<GMainLoop *>(user_data);
          // g_main_loop_quit(loop);

          // ?? to stop streaming
          AICamera_streamingStop();
          break;
        }
  
        case GST_MESSAGE_EOS:
          xlog("Received EOS, stopping...");
          // g_main_loop_quit(static_cast<GMainLoop *>(user_data));

          // ?? to stop streaming
          AICamera_streamingStop();
          break;
  
        default:
          break;
      }
      return TRUE; }, nullptr);

  // Start the pipeline
  GstStateChangeReturn ret = gst_element_set_state(gst_pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("failed to start the pipeline");
    gst_element_set_state(gst_pipeline, GST_STATE_NULL);
    gst_object_unref(gst_pipeline);
    isStreaming = false;
    return;
  }

  xlog("pipeline is running...");
  isStreaming = true;

  // Run the main loop
  gst_loop = g_main_loop_new(nullptr, FALSE);
  gst_bus_set_sync_handler(bus, nullptr, nullptr, nullptr);
  gst_object_unref(bus);
  g_main_loop_run(gst_loop);

  // Stop the pipeline when finished or interrupted
  xlog("Stopping the pipeline...");
  gst_element_set_state(gst_pipeline, GST_STATE_NULL);

  // Clean up
  gst_object_unref(gst_pipeline);
  if (gst_loop) {
    g_main_loop_unref(gst_loop);
    gst_loop = nullptr;
  }

  isStreaming = false;
  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");

}

void ThreadAICameraStreaming_usb() {
  xlog("++++ start ++++");
  counterFrame = 0;
  counterImg = 0;

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // final gst pipeline
  // gst-launch-1.0 v4l2src device="/dev/video137" io-mode=2 ! image/jpeg,width=2048,height=1536,framerate=30/1 ! jpegdec ! videoconvert ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream
  
  // Create the elements
  gst_pipeline = gst_pipeline_new("video-pipeline");
  GstElement *source = gst_element_factory_make("v4l2src", "source");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  GstElement *jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
  GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!gst_pipeline || !source || !capsfilter || !jpegdec || !videoconvert || !encoder || !sink) {
    xlog("failed to create GStreamer elements");
    return;
  }

  // Set properties for the elements
  g_object_set(G_OBJECT(source), "device", AICamrea_getVideoDevice().c_str(), nullptr);

  // Define the capabilities for the capsfilter
  // AD : 2048 * 1536
  // elic : 1920 * 1080
  GstCaps *caps = gst_caps_new_simple(
      "image/jpeg",
      "width", G_TYPE_INT, 2048,
      "height", G_TYPE_INT, 1536,
      "framerate", GST_TYPE_FRACTION, 30, 1,  // Add frame rate as 30/1
      nullptr);
  g_object_set(capsfilter, "caps", caps, nullptr);
  gst_caps_unref(caps);

  // Create a GstStructure for extra-controls
  GstStructure *controls = gst_structure_new(
      "extra-controls",                  // Name of the structure
      "video_gop_size", G_TYPE_INT, 30,  // Key-value pair
      // "h264_level", G_TYPE_INT, 13,      // Key-value pair
      nullptr  // End of key-value pairs
  );
  if (!controls) {
    xlog("Failed to create GstStructure");
    gst_object_unref(gst_pipeline);
    return;
  }
  g_object_set(G_OBJECT(encoder), "extra-controls", controls, nullptr);
  // Free the GstStructure after use
  gst_structure_free(controls);

  g_object_set(encoder, "capture-io-mode", 4, nullptr);  // dmabuf = 4
  g_object_set(sink, "location", "rtsp://localhost:8554/mystream", nullptr);

  // // Build the pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, capsfilter, jpegdec, videoconvert, encoder, sink, nullptr);
  if (!gst_element_link_many(source, capsfilter, jpegdec, videoconvert, encoder, sink, nullptr)) {
    xlog("failed to link elements in the pipeline");
    gst_object_unref(gst_pipeline);
    return;
  }

  // Attach pad probe to capture frames
  GstPad *pad = gst_element_get_static_pad(encoder, "sink");
  if (pad) {
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)AICAMERA_streamingDataCallback, nullptr, nullptr);
    gst_object_unref(pad);
  }

  // Start the pipeline
  GstStateChangeReturn ret = gst_element_set_state(gst_pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("failed to start the pipeline");
    gst_object_unref(gst_pipeline);
    return;
  }

  xlog("pipeline is running...");
  isStreaming = true;

  // Run the main loop
  gst_loop = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(gst_loop);

  // Stop the pipeline when finished or interrupted
  xlog("Stopping the pipeline...");
  gst_element_set_state(gst_pipeline, GST_STATE_NULL);

  // Clean up
  gst_object_unref(gst_pipeline);
  isStreaming = false;
  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");

}

void AICamera_streamingStart() {
  xlog("");
  if (isStreaming) {
    xlog("thread already running");
    return;
  }
  isStreaming = true;

  if (AICamrea_isUseCISCamera())
  {
    t_aicamera_streaming = std::thread(ThreadAICameraStreaming);
  } else {
    t_aicamera_streaming = std::thread(ThreadAICameraStreaming_usb);  
  }
  
  t_aicamera_streaming.detach();
}

void AICamera_streamingStop() {
  xlog("");
  if (!isStreaming) {
    xlog("Streaming not running");
    return;
  }

  if (gst_loop) {
    xlog("g_main_loop_quit");
    g_main_loop_quit(gst_loop);
    // g_main_loop_unref(gst_loop);
    // gst_loop = nullptr;
  } else {
    xlog("gst_loop is invalid or already destroyed.");
  }

  // ??
  isStreaming = false;
}

