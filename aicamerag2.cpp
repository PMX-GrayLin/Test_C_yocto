#include "aicamerag2.hpp"

// PWM
const std::string path_pwm = "/sys/devices/platform/soc/10048000.pwm/pwm/pwmchip0";
const std::string pwmTarget = path_pwm + "/pwm1";
const std::string path_pwmExport = path_pwm + "/export";
const int pwmPeriod = 200000;   // 5 kHz

// DI
int DI_GPIOs[NUM_DI] = {0, 1};  // DI GPIO
std::thread t_aicamera_monitorDI;
bool isMonitorDI = false;

// Triger
int Triger_GPIOs[NUM_Triger] = {17, 70};  // Triger GPIO
std::thread t_aicamera_monitorTriger;
bool isMonitorTriger = false;

// DO
int DO_GPIOs[NUM_DO] = {3, 7};  // DO GPIO

// DIO
int DIO_IN_GPIOs[NUM_DIO] = {2, 6, 12, 13};   // DI GPIO
int DIO_OUT_GPIOs[NUM_DIO] = {8, 9, 11, 5};  // DO GPIO
DIO_Direction dioDirection[NUM_DIO] = {diod_in};
std::thread t_aicamera_monitorDIO[NUM_DIO];
bool isMonitorDIO[NUM_DIO] = {false};

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

bool AICamrea_isUseCSICamera() {
  if (isPathExist(AICamreaCSIPath)) {
    xlog("path /dev/csi_cam_preview exist");
    return true;
  } else {
    xlog("path /dev/csi_cam_preview not exist");
    return false;
  }
}

std::string AICamrea_getVideoDevice() {
  std::string videoPath;

  bool isUseCSICamera = AICamrea_isUseCSICamera();

  if (isUseCSICamera) {
    videoPath = AICamreaCSIPath;
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
  int value = (int)(sec * 10000000.0);
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
      std::string directory = fs::path(path).parent_path().string();
      xlog("Raw path: [%s]", path.c_str());
      xlog("Parent directory: [%s]", directory.c_str());
      if (!isPathExist(directory.c_str())) {
        xlog("Directory does not exist, creating: %s", directory.c_str());
        fs::create_directories(directory);
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
      std::string directory = fs::path(path).parent_path().string();
      xlog("Raw path: [%s]", path.c_str());
      xlog("Parent directory: [%s]", directory.c_str());
      if (!isPathExist(directory.c_str())) {
        xlog("Directory does not exist, creating: %s", directory.c_str());
        fs::create_directories(directory);
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
      gst_caps_unref(caps);
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
  // gst-launch-1.0 -v v4l2src device=/dev/video18 ! video/x-raw,width=2048,height=1536 ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream

  // Create the elements
  gst_pipeline = gst_pipeline_new("video-pipeline");
  GstElement *source = gst_element_factory_make("v4l2src", "source");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!gst_pipeline || !source || !capsfilter || !encoder || !sink) {
    xlog("failed to create GStreamer elements");
    return;
  }

  // Set properties for the elements
  g_object_set(G_OBJECT(source), "device", AICamrea_getVideoDevice().c_str(), nullptr);

  // Define the capabilities for the capsfilter
  // AD : 2048 * 1536
  // elic : 1920 * 1080
  GstCaps *caps = gst_caps_new_simple(
      "video/x-raw",
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

  // Build the pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, capsfilter, encoder, sink, nullptr);
  if (!gst_element_link_many(source, capsfilter, encoder, sink, nullptr)) {
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

  if (AICamrea_isUseCSICamera()) {
    t_aicamera_streaming = std::thread(ThreadAICameraStreaming);
  } else {
    t_aicamera_streaming = std::thread(ThreadAICameraStreaming_usb);
  }

  t_aicamera_streaming.detach();
}

void AICamera_streamingStop() {
  xlog("");
  if (gst_loop) {
    g_main_loop_quit(gst_loop);
    g_main_loop_unref(gst_loop);
    gst_loop = nullptr;

    // ??
    isStreaming = false;
  } else {
    xlog("gst_loop is invalid or already destroyed.");
  }
}

void ThreadAICameraStreaming_GigE() {
  xlog("++++ start ++++");
  counterFrame = 0;
  counterImg = 0;

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // Create the pipeline
  gst_pipeline = gst_pipeline_new("video-pipeline");

  GstElement *source = gst_element_factory_make("aravissrc", "source");
  GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!gst_pipeline || !source || !videoconvert || !capsfilter || !encoder || !sink) {
    xlog("failed to create GStreamer elements");
    return;
  }

  // Set camera by ID or name (adjust "id1" if needed)
  g_object_set(G_OBJECT(source), "camera-name", "id1", nullptr);

  // Define the capabilities: NV12 format
  GstCaps *caps = gst_caps_new_simple(
      "video/x-raw",
      "format", G_TYPE_STRING, "NV12",
      nullptr);
  g_object_set(capsfilter, "caps", caps, nullptr);
  gst_caps_unref(caps);

  // Set encoder properties
  GstStructure *controls = gst_structure_new(
      "extra-controls",
      "video_gop_size", G_TYPE_INT, 30,
      nullptr);
  if (!controls) {
    xlog("Failed to create GstStructure");
    gst_object_unref(gst_pipeline);
    return;
  }
  g_object_set(G_OBJECT(encoder), "extra-controls", controls, nullptr);
  gst_structure_free(controls);

  g_object_set(encoder, "capture-io-mode", 4, nullptr);  // dmabuf
  g_object_set(sink, "location", "rtsp://localhost:8554/mystream", nullptr);

  // Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, videoconvert, capsfilter, encoder, sink, nullptr);
  if (!gst_element_link_many(source, videoconvert, capsfilter, encoder, sink, nullptr)) {
    xlog("failed to link elements in the pipeline");
    gst_object_unref(gst_pipeline);
    return;
  }

  // Optional: attach pad probe to monitor frames
  GstPad *pad = gst_element_get_static_pad(encoder, "sink");
  if (pad) {
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)AICAMERA_streamingDataCallback, nullptr, nullptr);
    gst_object_unref(pad);
  }

  // Start streaming
  GstStateChangeReturn ret = gst_element_set_state(gst_pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("failed to start the pipeline");
    gst_object_unref(gst_pipeline);
    return;
  }

  xlog("pipeline is running...");
  isStreaming = true;

  // Main loop
  gst_loop = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(gst_loop);

  // Clean up
  xlog("Stopping the pipeline...");
  gst_element_set_state(gst_pipeline, GST_STATE_NULL);
  gst_object_unref(gst_pipeline);
  isStreaming = false;
  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");
}

void AICamera_streamingStart_GigE() {
  xlog("");
  if (isStreaming) {
    xlog("thread already running");
    return;
  }
  isStreaming = true;
  
  t_aicamera_streaming = std::thread(ThreadAICameraStreaming_GigE);  

  t_aicamera_streaming.detach();
}

void AICamera_streamingStop_GigE() {
  xlog("");
  if (gst_loop) {
    g_main_loop_quit(gst_loop);
    g_main_loop_unref(gst_loop);
    gst_loop = nullptr;

    // ??
    isStreaming = false;
  } else {
    xlog("gst_loop is invalid or already destroyed.");
  }
}

void AICamera_setGPIO(int gpio_num, int value) {
  // xlog("gpiod version:%s", gpiod_version_string());

  gpiod_chip *chip;
  gpiod_line *line;

  // Open GPIO chip
  chip = gpiod_chip_open(GPIO_CHIP);
  if (!chip) {
    xlog("Failed to open GPIO chip:%s", GPIO_CHIP);
    return;
  }

  // Get GPIO line
  line = gpiod_chip_get_line(chip, gpio_num);
  if (!line) {
    xlog("Failed to get GPIO line");
    gpiod_chip_close(chip);
    return;
  }

  // Request line as output
  if (gpiod_line_request_output(line, "my_gpio_control", value) < 0) {
    xlog("Failed to request GPIO line as output");
    gpiod_chip_close(chip);
    return;
  }

  // Set the GPIO value
  if (gpiod_line_set_value(line, value) < 0) {
    xlog("Failed to set GPIO value");
  }

  // Release resources
  gpiod_line_release(line);
  gpiod_chip_close(chip);
}

void AICamera_setLED(string led_index, string led_color) {
  int gpio_index1 = 0;
  int gpio_index2 = 0;

  if (led_index == "1") {
    gpio_index1 = 79;
    gpio_index2 = 80;  
  } else if (led_index == "2") {
    gpio_index1 = 81;
    gpio_index2 = 82;  
  } else if (led_index == "3") {
    gpio_index1 = 107;
    gpio_index2 = 108;  
  }

  if (isSameString(led_color.c_str(), "red")) {
    AICamera_setGPIO(gpio_index1, 1);
    AICamera_setGPIO(gpio_index2, 0);
  } else if (isSameString(led_color.c_str(), "green")) {
    AICamera_setGPIO(gpio_index1, 0);
    AICamera_setGPIO(gpio_index2, 1);;
  } else if (isSameString(led_color.c_str(), "orange")) {
    AICamera_setGPIO(gpio_index1, 1);
    AICamera_setGPIO(gpio_index2, 1);
  } else if (isSameString(led_color.c_str(), "off")) {
    AICamera_setGPIO(gpio_index1, 0);
    AICamera_setGPIO(gpio_index2, 0);
  }
}

void ThreadAICameraMonitorDI() {
  struct gpiod_chip *chip;
  struct gpiod_line *lines[NUM_DI];
  struct pollfd fds[NUM_DI];
  int i, ret;

  // Open GPIO chip
  chip = gpiod_chip_open(GPIO_CHIP);
  if (!chip) {
    xlog("Failed to open GPIO chip: %s", GPIO_CHIP);
    return;
  }

  // Configure GPIOs for edge detection
  for (i = 0; i < NUM_DI; i++) {
    lines[i] = gpiod_chip_get_line(chip, DI_GPIOs[i]);
    if (!lines[i]) {
      xlog("Failed to get GPIO line %d", DI_GPIOs[i]);
      continue;
    }

    // Request GPIO line for both rising and falling edge events
    ret = gpiod_line_request_both_edges_events(lines[i], "gpio_interrupt");
    if (ret < 0) {
      xlog("Failed to request GPIO %d for edge events", DI_GPIOs[i]);
      gpiod_line_release(lines[i]);
      continue;
    }

    // Get file descriptor for polling
    fds[i].fd = gpiod_line_event_get_fd(lines[i]);
    fds[i].events = POLLIN;
  }

  xlog("^^^^ Start ^^^^");

  // Main loop to monitor GPIOs
  while (isMonitorDI) {
    ret = poll(fds, NUM_DI, -1);  // Wait indefinitely for an event
    if (ret < 0) {
      xlog("Error in poll");
      break;
    }

    // Check which GPIO triggered the event
    for (i = 0; i < NUM_DI; i++) {
      if (fds[i].revents & POLLIN) {
        struct gpiod_line_event event;
        gpiod_line_event_read(lines[i], &event);

        xlog("GPIO %d event detected! Type: %s", DI_GPIOs[i],
             (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) ? "rising" : "falling");
      }
    }
  }

  xlog("^^^^ Stop ^^^^");

  // Cleanup
  for (i = 0; i < NUM_DI; i++) {
    gpiod_line_release(lines[i]);
  }
  gpiod_chip_close(chip);
}

void AICamera_MonitorDIStart() {
  xlog("");
  if (isMonitorDI) {
    xlog("thread already running");
    return;
  }
  isMonitorDI = true;
  t_aicamera_monitorDI = std::thread(ThreadAICameraMonitorDI);  
  t_aicamera_monitorDI.detach();
}
void AICamera_MonitorDIStop() {
  isMonitorDI = false;
}

void ThreadAICameraMonitorTriger() {
  struct gpiod_chip *chip;
  struct gpiod_line *lines[NUM_Triger];
  struct pollfd fds[NUM_Triger];
  int i, ret;

  // Open GPIO chip
  chip = gpiod_chip_open(GPIO_CHIP);
  if (!chip) {
    xlog("Failed to open GPIO chip: %s", GPIO_CHIP);
    return;
  }

  // Configure GPIOs for edge detection
  for (i = 0; i < NUM_Triger; i++) {
    lines[i] = gpiod_chip_get_line(chip, Triger_GPIOs[i]);
    if (!lines[i]) {
      xlog("Failed to get GPIO line %d", Triger_GPIOs[i]);
      continue;
    }

    // Request GPIO line for both rising and falling edge events
    ret = gpiod_line_request_both_edges_events(lines[i], "gpio_interrupt");
    if (ret < 0) {
      xlog("Failed to request GPIO %d for edge events", Triger_GPIOs[i]);
      gpiod_line_release(lines[i]);
      continue;
    }

    // Get file descriptor for polling
    fds[i].fd = gpiod_line_event_get_fd(lines[i]);
    fds[i].events = POLLIN;
  }

  xlog("^^^^ Start ^^^^");

  // Main loop to monitor GPIOs
  while (isMonitorTriger) {
    ret = poll(fds, NUM_Triger, -1);  // Wait indefinitely for an event
    if (ret < 0) {
      xlog("Error in poll");
      break;
    }

    // Check which GPIO triggered the event
    for (i = 0; i < NUM_Triger; i++) {
      if (fds[i].revents & POLLIN) {
        struct gpiod_line_event event;
        gpiod_line_event_read(lines[i], &event);

        xlog("GPIO %d event detected! Type: %s", Triger_GPIOs[i],
             (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) ? "rising" : "falling");
      }
    }
  }

  xlog("^^^^ Stop ^^^^");

  // Cleanup
  for (i = 0; i < NUM_Triger; i++) {
    gpiod_line_release(lines[i]);
  }
  gpiod_chip_close(chip);
}

void AICamera_MonitorTrigerStart() {
  xlog("");
  if (isMonitorTriger) {
    xlog("thread already running");
    return;
  }
  isMonitorTriger = true;
  t_aicamera_monitorTriger = std::thread(ThreadAICameraMonitorTriger);  
  t_aicamera_monitorTriger.detach();
}
void AICamera_MonitorTrigerStop() {
  isMonitorTriger = false;
}

void AICamera_setDO(string index_do, string on_off) {
  int index_gpio = 0;
  bool isON = false;

  int index = std::stoi(index_do);
  if (index > 0 && index <= NUM_DO) {
    index_gpio = DO_GPIOs[index - 1];
  } else {
    xlog("index out of range...");
    return;
  }

  if (isSameString(on_off.c_str(), "on")) {
    isON = true;
  } else if (isSameString(on_off.c_str(), "off")) {
    isON = false;
  } else {
    xlog("input string should be on or off...");
    return;
  }

  AICamera_setGPIO(index_gpio, isON ? 1 : 0);
}

void ThreadAICameraMonitorDIOIn(int index_dio) {

  struct gpiod_chip *chip;
  struct gpiod_line *line;
  struct pollfd fd;
  int ret;

  if (index_dio < 0 || index_dio >= NUM_DIO) {
    xlog("Invalid DIO index: %d", index_dio);
    return;
  }

  // Open GPIO chip
  chip = gpiod_chip_open(GPIO_CHIP);
  if (!chip) {
    xlog("Failed to open GPIO chip: %s", GPIO_CHIP);
    return;
  }

  // Get specified GPIO line
  line = gpiod_chip_get_line(chip, DIO_IN_GPIOs[index_dio]);
  if (!line) {
    xlog("Failed to get GPIO line %d", DIO_IN_GPIOs[index_dio]);
    gpiod_chip_close(chip);
    return;
  }


  // Request GPIO line for both rising and falling edge events
  ret = gpiod_line_request_both_edges_events(line, "gpio_interrupt");
  if (ret < 0) {
    xlog("Failed to request GPIO %d for edge events", DIO_IN_GPIOs[index_dio]);
    gpiod_chip_close(chip);
    return;
  }

  // Get file descriptor for polling
  fd.fd = gpiod_line_event_get_fd(line);
  fd.events = POLLIN;

  xlog("Thread Monitoring GPIO %d for events...start", DIO_IN_GPIOs[index_dio]);

  // Main loop to monitor GPIO
  while (isMonitorDIO[index_dio]) {
    ret = poll(&fd, 1, -1);  // Wait indefinitely for an event
    if (ret < 0) {
      xlog("Error in poll");
      break;
    }

    if (fd.revents & POLLIN) {
      struct gpiod_line_event event;
      gpiod_line_event_read(line, &event);

      xlog("GPIO %d event detected! Type: %s", DIO_IN_GPIOs[index_dio],
           (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) ? "RISING" : "FALLING");
    }
  }

  xlog("Thread Monitoring GPIO %d for events...stop", DIO_IN_GPIOs[index_dio]);

  // Cleanup
  gpiod_line_release(line);
  gpiod_chip_close(chip);
}

void AICamera_MonitorDIOInStart(int index_dio) {
  xlog("");
  if (isMonitorDIO[index_dio]) {
    xlog("thread already running");
    return;
  }
  isMonitorDIO[index_dio] = true;
  t_aicamera_monitorDIO[index_dio] = std::thread(ThreadAICameraMonitorDIOIn, index_dio);  
  t_aicamera_monitorDIO[index_dio].detach();
}

void AICamera_MonitorDIOInStop(int index_dio) {
  isMonitorDIO[index_dio] = false;
}

void AICamera_setDIODirection(string index_dio, string di_do) {
  xlog("index:%s, direction:%s", index_dio.c_str(), di_do.c_str());
  int index_gpio_in = 0;
  int index_gpio_out = 0;

  int index = std::stoi(index_dio);
  if (index > 0 && index <= NUM_DIO) {
    index_gpio_in = DIO_IN_GPIOs[index - 1];
    index_gpio_out = DIO_OUT_GPIOs[index - 1];
  } else {
    xlog("index out of range...");
    return;
  }

  if (isSameString(di_do.c_str(), "di")) {
    // set flag
    dioDirection[index - 1] = diod_in;

    // make gpio out low
    AICamera_setGPIO(index_gpio_out, 0);

    // start monitor gpio input
    AICamera_MonitorDIOInStart(index - 1);

  } else if (isSameString(di_do.c_str(), "do")) {
    // set flag
    dioDirection[index - 1] = diod_out;

    // stop monitor gpio input
    AICamera_MonitorDIOInStop(index - 1);

  } else {
    xlog("input string should be di or do...");
    return;
  }
}

void AICamera_setDIOOut(string index_dio, string on_off) {
  int index_gpio = 0;
  bool isON = false;
  int index = std::stoi(index_dio);

  if (dioDirection[index - 1] != diod_out) {
    xlog("direction should be set first...");
    return;
  }

  if (index > 0 && index <= NUM_DIO) {
    index_gpio = DIO_OUT_GPIOs[index - 1];
  } else {
    xlog("index out of range...");
    return;
  }

  if (isSameString(on_off.c_str(), "on")) {
    isON = true;
  } else if (isSameString(on_off.c_str(), "off")) {
    isON = false;
  } else {
    xlog("input string should be on or off...");
    return;
  }

  AICamera_setGPIO(index_gpio, isON ? 1 : 0);
}

void AICamera_writePWMFile(const std::string &path, const std::string &value) {
  std::ofstream fs(path);
  if (fs) {
      fs << value;
      fs.close();
  } else {
      std::cerr << "Failed to write to " << path << std::endl;
  }
}

void AICamera_setPWM(string sPercent) {
  if (!isPathExist(pwmTarget.c_str())) {
    xlog("PWM init...");
    AICamera_writePWMFile(path_pwmExport, "1");
    usleep(500000);  // sleep 0.5s
    AICamera_writePWMFile(pwmTarget + "/period", std::to_string(pwmPeriod));
  }

  int percent = std::stoi(sPercent);
  if (percent != 0)
  {
    int duty_cycle = pwmPeriod * percent / 100;
    AICamera_writePWMFile(pwmTarget + "/duty_cycle", std::to_string(duty_cycle));
    AICamera_writePWMFile(pwmTarget + "/enable", "1");
  } else {
    AICamera_writePWMFile(pwmTarget + "/enable", "0");
  }

}
