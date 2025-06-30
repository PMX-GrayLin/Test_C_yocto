#include "cam_omnivision.hpp"

#include <linux/videodev2.h>  // For V4L2 definitions
#include <sys/ioctl.h>        // For ioctl()
#include <fcntl.h>            // For open()
#include <iomanip>

#include <gst/gst.h>
#include "json.hpp"
// #include "mqttx.hpp"

#include "device.hpp"
#include "image_utils.hpp"

std::thread t_streaming_aic;
bool isStreaming_aic = false;

bool isCapturePhoto_aic = false;
bool isCropPhoto_aic = false;
bool isPaddingPhoto_aic = false;
SavedPhotoFormat savedPhotoFormat_aic = spf_PNG;
std::string pathName_savedImage_aic = "";
std::string pathName_croppedImage_aic = "";
std::string pathName_inputImage_aic = "";
cv::Rect crop_roi_aic(0, 0, 0, 0);

static volatile int counterFrame_aic = 0;

static GstElement *gst_pipeline_aic = nullptr;
static GstElement *gst_flip = nullptr;
static GMainLoop *gst_loop_aic = nullptr;

namespace fs = std::filesystem;

void AICP_handle_RESTful(std::vector<std::string> segments) {
  if (isSameString(segments[1], "start")) {
    AICP_streamingStart();

  } else if (isSameString(segments[1], "stop")) {
    AICP_streamingStop();

  } else if (isSameString(segments[1], "flip")) {
    AICP_setFlip(segments[2]);

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
    AICP_setImagePath(path.c_str());
    AICP_captureImage();
  }
}

bool AICP_isUseCISCamera() {
  if (isPathExist(AICamreaCISPath)) {
    // xlog("path /dev/csi_cam_preview exist");
    return true;
  } else {
    xlog("path /dev/csi_cam_preview not exist");
    return false;
  }
}

std::string AICP_getVideoDevice() {
  
  std::string videoPath;
  if (AICP_isUseCISCamera()) {
    videoPath = AICamreaCISPath;
  } else {
    videoPath = AICamreaUSBPath;
  }

  // xlog("videoPath:%s", videoPath.c_str());
  return videoPath;
}

int ioctl_get_value_aic(int control_ID) {
  int fd = open(AICP_getVideoDevice().c_str(), O_RDWR);
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

int ioctl_set_value_aic(int control_ID, int value) {
  int fd = open(AICP_getVideoDevice().c_str(), O_RDWR);
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

int AICP_getBrightness() {
  return ioctl_get_value_aic(V4L2_CID_BRIGHTNESS);
}

void AICP_setBrightness(int value) {
  xlog("value:%d", value);
  ioctl_set_value_aic(V4L2_CID_BRIGHTNESS, value);
}

int AICP_getContrast() {
  return ioctl_get_value_aic(V4L2_CID_CONTRAST);
}

void AICP_setContrast(int value) {
  xlog("value:%d", value);
  ioctl_set_value_aic(V4L2_CID_CONTRAST, value);
}

int AICP_getSaturation() {
  return ioctl_get_value_aic(V4L2_CID_SATURATION);
}

void AICP_setSaturation(int value) {
  xlog("value:%d", value);
  ioctl_set_value_aic(V4L2_CID_SATURATION, value);
}

int AICP_getHue() {
  return ioctl_get_value_aic(V4L2_CID_HUE);
}

void AICP_setHue(int value) {
  xlog("value:%d", value);
  ioctl_set_value_aic(V4L2_CID_HUE, value);
}

// command example
// v4l2-ctl -d 78 --get-ctrl=white_balance_automatic
int AICP_getWhiteBalanceAutomatic() {
  return ioctl_get_value_aic(V4L2_CID_AUTO_WHITE_BALANCE);
}

// command example
// v4l2-ctl -d 78 --set-ctrl=white_balance_automatic=0
// v4l2-ctl -d 78 --set-ctrl=white_balance_automatic=1
void AICP_setWhiteBalanceAutomatic(bool enable) {
  xlog("enable:%d", enable);
  ioctl_set_value_aic(V4L2_CID_AUTO_WHITE_BALANCE, enable ? 1 : 0);
}

int AICP_getSharpness() {
  return ioctl_get_value_aic(V4L2_CID_SHARPNESS);
}

void AICP_setSharpness(int value) {
  xlog("value:%d", value);
  ioctl_set_value_aic(V4L2_CID_SHARPNESS, value);
}

int AICP_getISO() {
  return ioctl_get_value_aic(0x009819a9);
}

void AICP_setISO(int value) {
  xlog("value:%d", value);
  ioctl_set_value_aic(0x009819a9, value);
}

int AICP_getExposure() {
  return ioctl_get_value_aic(V4L2_CID_EXPOSURE);
}

void AICP_setExposure(int value) {
  xlog("value:%d", value);
  ioctl_set_value_aic(V4L2_CID_EXPOSURE, value);
}

int AICP_getWhiteBalanceTemperature() {
  return ioctl_get_value_aic(V4L2_CID_WHITE_BALANCE_TEMPERATURE);
}

void AICP_setWhiteBalanceTemperature(int value) {
  xlog("value:%d", value);
  ioctl_set_value_aic(V4L2_CID_WHITE_BALANCE_TEMPERATURE, value);
}

int AICP_getExposureAuto() {
  return ioctl_get_value_aic(V4L2_CID_EXPOSURE_AUTO);
}

void AICP_setExposureAuto(bool enable) {
  xlog("enable:%d", enable);
  ioctl_set_value_aic(V4L2_CID_EXPOSURE_AUTO, enable ? 0 : 1);
}

int AICP_getExposureTimeAbsolute() {
  xlog("");
  return ioctl_get_value_aic(V4L2_CID_EXPOSURE_ABSOLUTE);
}

void AICP_setExposureTimeAbsolute(double sec) {
  xlog("sec:%f", sec);
  int value = (int)(sec * 1000000.0);
  xlog("value:%d", value);
  ioctl_set_value_aic(V4L2_CID_EXPOSURE_ABSOLUTE, value);
}

int AICP_getFocusAbsolute() {
  return ioctl_get_value_aic(V4L2_CID_FOCUS_ABSOLUTE);
}

void AICP_setFocusAbsolute(int value) {
  xlog("value:%d", value);
  ioctl_set_value_aic(V4L2_CID_FOCUS_ABSOLUTE, value);
}

int AICP_getFocusAuto() {
  return ioctl_get_value_aic(V4L2_CID_FOCUS_AUTO);
}

void AICP_setFocusAuto(bool enable) {
  xlog("enable:%d", enable);
  ioctl_set_value_aic(V4L2_CID_FOCUS_AUTO, enable ? 1 : 0);
}

void AICP_setImagePath(const string& imagePath) {
  pathName_savedImage_aic = imagePath;
  xlog("pathName_savedImage_aic:%s", pathName_savedImage_aic.c_str());
}

void AICP_setCropImagePath(const string& imagePath) {
  pathName_croppedImage_aic = imagePath;
  xlog("pathName_croppedImage_aic:%s", pathName_croppedImage_aic.c_str());
}

void AICP_setInputImagePath(const string& imagePath) {
  pathName_inputImage_aic = imagePath;
  xlog("pathName_inputImage_aic:%s", pathName_inputImage_aic.c_str());
}

void AICP_setCropROI(cv::Rect roi) {
  crop_roi_aic = roi;
  xlog("ROI x:%d, y:%d, w:%d, h:%d", crop_roi_aic.x, crop_roi_aic.y, crop_roi_aic.width, crop_roi_aic.height);
}

bool AICP_isCropImage() {
  return (crop_roi_aic != cv::Rect(0, 0, 0, 0));
}

void AICP_captureImage() {
  if (!isStreaming_aic) {
    xlog("do nothing...camera is not streaming");
    return;
  }

  xlog("");
  isCapturePhoto_aic = true;
}

void AICP_enableCrop(bool enable) {
  isCropPhoto_aic = enable;
  xlog("isCropPhoto_aic:%d", isCropPhoto_aic);
}

void AICP_enablePadding(bool enable) {
  isPaddingPhoto_aic = enable;
  xlog("isPaddingPhoto_aic:%d", isPaddingPhoto_aic);
}

void AICP_threadSaveImage(const std::string path, const cv::Mat &frameBuffer) {
  std::thread([path, frameBuffer]() {
    try {
      xlog("---- AICP_threadSaveImage start ----");
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

      auto start = std::chrono::high_resolution_clock::now();

      // save the image
      if (cv::imwrite(path, frameBuffer)) {
        xlog("Saved frame to %s", path.c_str());
      } else {
        xlog("Failed to save frame to %s", path.c_str());
      }

      auto end = std::chrono::high_resolution_clock::now();
      xlog("Elapsed time: %ld ms", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

    } catch (const std::exception &e) {
      xlog("Exception during image save: %s", e.what());
    }

    xlog("---- AICP_threadSaveImage stop ----");
  }).detach();  // Detach to run in the background
}

void AICP_threadSaveCropImage(const std::string path, const cv::Mat &frameBuffer, cv::Rect roi) {
  std::thread([path, frameBuffer, roi]() {
    try {
      xlog("---- AICP_threadSaveCropImage start ----");
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
        AICP_setCropROI(reset_roi);

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

    xlog("---- AICP_threadSaveCropImage stop ----");
  }).detach();  // Detach to run in the background
}

void AICP_saveImage(GstPad *pad, GstPadProbeInfo *info) {
  if (isCapturePhoto_aic) {
    isCapturePhoto_aic = false;

    SimpleRect roi = {crop_roi_aic.x, crop_roi_aic.y, crop_roi_aic.width, crop_roi_aic.height};
    imgu_saveImage_thread((void *)pad, (void *)info, pathName_savedImage_aic, &syncSignal_save);
    imgu_cropImage_thread((void *)pad, (void *)info, pathName_croppedImage_aic, roi, &syncSignal_crop);
  }
}

// Callback to handle incoming buffer data
GstPadProbeReturn AICP_streamingDataCallback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  AICP_streamingLED();
  AICP_saveImage(pad, info);
  return GST_PAD_PROBE_OK;
}

void Thread_AICPStreaming() {
  xlog("++++ start ++++");
  counterFrame_aic = 0;

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // final gst pipeline
  // gst-launch-1.0 v4l2src device=${VIDEO_DEV[0]} ! video/x-raw,width=2048,height=1536 ! queue ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! h264parse config-interval=1 ! rtspclientsink location=rtsp://localhost:8554/mystream

  // Create the elements
  gst_pipeline_aic = gst_pipeline_new("video-pipeline");
  GstElement *source = gst_element_factory_make("v4l2src", "source");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  gst_flip = gst_element_factory_make("videoflip", "flip"); 
  GstElement *queue = gst_element_factory_make("queue", "queue");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *parser = gst_element_factory_make("h264parse", "parser");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!gst_pipeline_aic || !source || !capsfilter || !gst_flip || !queue || !encoder || !parser || !sink) {
    xlog("failed to create GStreamer elements");
    return;
  }

  // Set properties for the elements
  g_object_set(G_OBJECT(source), "device", AICP_getVideoDevice().c_str(), nullptr);

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

  // Set videoflip mode: 0 = none, 1 = clockwise, 2 = rotate-180, 3 = counter-clockwise, etc.
  g_object_set(gst_flip, "method", 0, nullptr);  // no flip initially

  // Create a GstStructure for extra-controls
  GstStructure *controls = gst_structure_new(
      "extra-controls",                  // Name of the structure
      "video_gop_size", G_TYPE_INT, 30,  // Key-value pair
      // "h264_level", G_TYPE_INT, 13,      // Key-value pair
      nullptr  // End of key-value pairs
  );
  if (!controls) {
    xlog("Failed to create GstStructure");
    gst_object_unref(gst_pipeline_aic);
    return;
  }
  g_object_set(G_OBJECT(encoder), "extra-controls", controls, nullptr);
  // Free the GstStructure after use
  gst_structure_free(controls);

  g_object_set(encoder, "capture-io-mode", 4, nullptr);  // dmabuf = 4
  g_object_set(parser, "config-interval", 1, nullptr); // send SPS/PPS every keyframe
  g_object_set(sink, "location", "rtsp://localhost:8554/mystream", nullptr);

  // Build the pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline_aic), source, capsfilter, gst_flip, queue, encoder, parser, sink, nullptr);
  if (!gst_element_link_many(source, capsfilter, gst_flip, queue, encoder, parser, sink, nullptr)) {
    xlog("failed to link elements in the pipeline");
    gst_object_unref(gst_pipeline_aic);
    return;
  }

  // Attach pad probe to capture frames
  GstPad *pad = gst_element_get_static_pad(encoder, "sink");
  if (pad) {
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)AICP_streamingDataCallback, nullptr, nullptr);
    gst_object_unref(pad);
  }

  // Add bus watch to handle errors and EOS
  GstBus *bus = gst_element_get_bus(gst_pipeline_aic);
  gst_bus_add_watch(bus, [](GstBus *, GstMessage *msg, gpointer user_data) -> gboolean {
      switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
          GError *err;
          gchar *dbg;
          gst_message_parse_error(msg, &err, &dbg);
          xlog("Pipeline error: %s", err->message);
          g_error_free(err);
          g_free(dbg);
  
          AICP_streamingStop();
          FW_setLED("2","red");
          break;
        }
  
        case GST_MESSAGE_EOS:
          xlog("Received EOS, stopping...");

          AICP_streamingStop();
          FW_setLED("2","red");
          break;
  
        default:
          break;
      }
      return TRUE; }, nullptr);

  // Start the pipeline
  GstStateChangeReturn ret = gst_element_set_state(gst_pipeline_aic, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("failed to start the pipeline");
    gst_element_set_state(gst_pipeline_aic, GST_STATE_NULL);
    gst_object_unref(gst_pipeline_aic);
    isStreaming_aic = false;
    return;
  }

  xlog("pipeline is running...");
  isStreaming_aic = true;

  // Run the main loop
  gst_loop_aic = g_main_loop_new(nullptr, FALSE);
  gst_bus_set_sync_handler(bus, nullptr, nullptr, nullptr);
  gst_object_unref(bus);
  g_main_loop_run(gst_loop_aic);

  // Stop the pipeline when finished or interrupted
  xlog("Stopping the pipeline...");
  gst_element_set_state(gst_pipeline_aic, GST_STATE_NULL);

  // Clean up
  gst_object_unref(gst_pipeline_aic);
  if (gst_loop_aic) {
    g_main_loop_unref(gst_loop_aic);
    gst_loop_aic = nullptr;
  }

  FW_setLED("2","green");
  isStreaming_aic = false;
  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");
}

void Thread_AICPStreaming_usb() {
  xlog("++++ start ++++");
  counterFrame_aic = 0;

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // final gst pipeline
  // gst-launch-1.0 v4l2src device="/dev/video137" io-mode=2 ! image/jpeg,width=2048,height=1536,framerate=30/1 ! jpegdec ! videoconvert ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream
  
  // Create the elements
  gst_pipeline_aic = gst_pipeline_new("video-pipeline");
  GstElement *source = gst_element_factory_make("v4l2src", "source");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  GstElement *jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
  GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!gst_pipeline_aic || !source || !capsfilter || !jpegdec || !videoconvert || !encoder || !sink) {
    xlog("failed to create GStreamer elements");
    return;
  }

  // Set properties for the elements
  g_object_set(G_OBJECT(source), "device", AICP_getVideoDevice().c_str(), nullptr);

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
    gst_object_unref(gst_pipeline_aic);
    return;
  }
  g_object_set(G_OBJECT(encoder), "extra-controls", controls, nullptr);
  // Free the GstStructure after use
  gst_structure_free(controls);

  g_object_set(encoder, "capture-io-mode", 4, nullptr);  // dmabuf = 4
  g_object_set(sink, "location", "rtsp://localhost:8554/mystream", nullptr);

  // // Build the pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline_aic), source, capsfilter, jpegdec, videoconvert, encoder, sink, nullptr);
  if (!gst_element_link_many(source, capsfilter, jpegdec, videoconvert, encoder, sink, nullptr)) {
    xlog("failed to link elements in the pipeline");
    gst_object_unref(gst_pipeline_aic);
    return;
  }

  // Attach pad probe to capture frames
  GstPad *pad = gst_element_get_static_pad(encoder, "sink");
  if (pad) {
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)AICP_streamingDataCallback, nullptr, nullptr);
    gst_object_unref(pad);
  }

  // Start the pipeline
  GstStateChangeReturn ret = gst_element_set_state(gst_pipeline_aic, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("failed to start the pipeline");
    gst_object_unref(gst_pipeline_aic);
    return;
  }

  xlog("pipeline is running...");
  isStreaming_aic = true;

  // Run the main loop
  gst_loop_aic = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(gst_loop_aic);

  // Stop the pipeline when finished or interrupted
  xlog("Stopping the pipeline...");
  gst_element_set_state(gst_pipeline_aic, GST_STATE_NULL);

  // Clean up
  gst_object_unref(gst_pipeline_aic);
  isStreaming_aic = false;
  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");

}

void AICP_streamingStart() {
  xlog("");
  if (isStreaming_aic) {
    xlog("thread already running");
    return;
  }
  isStreaming_aic = true;

  FW_setLED("2", "off");

  if (AICP_isUseCISCamera())
  {
    t_streaming_aic = std::thread(Thread_AICPStreaming);
  } else {
    t_streaming_aic = std::thread(Thread_AICPStreaming_usb);  
  }
  
  t_streaming_aic.detach();
}

void AICP_streamingStop() {
  xlog("");
  if (!isStreaming_aic) {
    xlog("Streaming not running");
    return;
  }

  if (gst_loop_aic) {
    xlog("g_main_loop_quit");
    g_main_loop_quit(gst_loop_aic);
  } else {
    xlog("gst_loop_aic is invalid or already destroyed.");
  }

  isStreaming_aic = false;
}

void AICP_streamingLED() {
  counterFrame_aic++;
  if (counterFrame_aic%15 == 0)
  {
    FW_toggleLED("2", "orange");
  }
}

void AICP_setFlip(const std::string & methodS) {

  VideoFlipMethod method = VideoFlipMethod::vfm_NONE;
  if (gst_flip) {
    if (isSameString(methodS, "horizontal") || isSameString(methodS, "h")) {
      method = VideoFlipMethod::vfm_HORIZONTAL_FLIP ;
    } else if (isSameString(methodS, "vertical") || isSameString(methodS, "v")) {
      method = VideoFlipMethod::vfm_VERTICAL_FLIP ;
    } else if (isSameString(methodS, "rotate180") || isSameString(methodS, "r180")) {
      method = VideoFlipMethod::vfm_ROTATE_180 ;
    } else if (isSameString(methodS, "none") || isSameString(methodS, "normal") || isSameString(methodS, "n")) {
      method = VideoFlipMethod::vfm_NONE ;
    }
    g_object_set(gst_flip, "method", static_cast<int>(method), nullptr);
    xlog("Flip method set to %d", static_cast<int>(method));
  } else {
    xlog("Flip element not initialized");
  }
}

void AICP_load_crop_saveImage() {
  // not use thread here
  // std::thread([]() {
    try {
      xlog("---- AICP_load_crop_saveImage start ----");
      auto start = std::chrono::high_resolution_clock::now();

      // Load the image
      cv::Mat image = cv::imread(pathName_inputImage_aic);
      if (image.empty()) {
        xlog("Failed to load image from %s", pathName_inputImage_aic.c_str());
        return;
      }

      cv::Mat outputImage;

      if (isCropPhoto_aic) {
        // Crop region
        cv::Rect roi = crop_roi_aic & cv::Rect(0, 0, image.cols, image.rows);  // safety clip
        if (roi.width <= 0 || roi.height <= 0) {
          xlog("Invalid ROI for cropping");
          return;
        }

        cv::Mat croppedImage = image(roi);

        if (isPaddingPhoto_aic) {
          int squareSize = std::max(croppedImage.cols, croppedImage.rows);
          cv::Mat paddedImage = cv::Mat::zeros(squareSize, squareSize, croppedImage.type());

          int offsetX = (squareSize - croppedImage.cols) / 2;
          int offsetY = (squareSize - croppedImage.rows) / 2;

          croppedImage.copyTo(paddedImage(cv::Rect(offsetX, offsetY, croppedImage.cols, croppedImage.rows)));
          outputImage = paddedImage;
        } else {
          outputImage = croppedImage;
        }

        // Reset crop ROI
        AICP_setCropROI(cv::Rect(0, 0, 0, 0));

      } else {
        outputImage = image;
      }

      // Save image
      std::vector<int> params;

      // Lowercase file extension check (C++17 compatible)
      std::string lower_path = pathName_savedImage_aic;
      std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);

      if (lower_path.size() >= 4 && lower_path.substr(lower_path.size() - 4) == ".png") {
        params = {cv::IMWRITE_PNG_COMPRESSION, 0};  // Fastest (no compression)
      } else if (
          (lower_path.size() >= 4 && lower_path.substr(lower_path.size() - 4) == ".jpg") ||
          (lower_path.size() >= 5 && lower_path.substr(lower_path.size() - 5) == ".jpeg")) {
        params = {cv::IMWRITE_JPEG_QUALITY, 95};  // Quality: 0–100 (default is 95)
      }

      bool isSaveOK;
      if (!params.empty()) {
        isSaveOK = cv::imwrite(pathName_savedImage_aic, outputImage, params);
      } else {
        isSaveOK = cv::imwrite(pathName_savedImage_aic, outputImage);
      }
      if (isSaveOK) {
        xlog("Saved frame to %s", pathName_savedImage_aic.c_str());
      } else {
        xlog("Failed to save frame to %s", pathName_savedImage_aic.c_str());
      }

      auto end = std::chrono::high_resolution_clock::now();
      xlog("Elapsed time: %ld ms", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

    } catch (const std::exception &e) {
      xlog("Exception during image save: %s", e.what());
    }
    xlog("---- AICP_load_crop_saveImage stop ----");
  // }).detach();  // Detach to run in the background
}

void AICP_publishDINState(int din_pin, const std::string &pin_state) {
  nlohmann::ordered_json j;
  j["cmd"] = "IO_DIN_EVENT_SET_RESP";
  j["args"]["din_pin"] = std::to_string(din_pin + 1);
  j["args"]["pin_state"] = pin_state;

  std::string json = j.dump();
  // ?? mqtt send
}

void AICP_publishDIODINState(int din_pin, const std::string &pin_state) {
  nlohmann::ordered_json j;
  j["cmd"] = "IO_DIO_DIN_EVENT_SET_RESP";
  j["args"]["din_pin"] = std::to_string(din_pin + 1);
  j["args"]["pin_state"] = pin_state;

  std::string json = j.dump();
  // ?? send mqtt
}

