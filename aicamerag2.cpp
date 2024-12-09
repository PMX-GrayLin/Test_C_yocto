#include "global.hpp"
#include "aicamerag2.hpp"

std::thread t_aicamera_streaming;
bool is_aicamera_streaming = false;

static volatile int counterFrame = 0;
static int counterImg = 0;

static GstElement *gst_pipeline = nullptr;
static GMainLoop *gst_loop = nullptr;

std::string AICamrea_getVideoDevice() {
  std::string result;
  FILE* pipe = popen("v4l2-ctl --list-devices | grep mtk-v4l2-camera -A 3", "r");
  if (pipe) {
    std::string output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
      output += buffer;
    }
    pclose(pipe);

    std::regex device_regex("/dev/video\\d+");
    std::smatch match;
    if (std::regex_search(output, match, device_regex)) {
      result = match[0];
    }
  }
  return result;
}

int ioctl_get_value(int control_ID) {
  int fd = open(AICamrea_getVideoDevice().c_str(), O_RDWR);
  if (fd == -1) {
    xlog("Failed to open video device:%s", strerror(errno));
    return -1;
  }

  struct v4l2_queryctrl queryctrl;
  memset(&queryctrl, 0, sizeof(queryctrl));
  queryctrl.id = control_ID;
  if (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
    xlog("queryctrl.minimum:%d", queryctrl.minimum);
    xlog("queryctrl.maximum:%d", queryctrl.maximum);
  } else {
    xlog("ioctl fail, VIDIOC_QUERYCTRL... error:%s", strerror(errno));
  }

  struct v4l2_control ctrl;
  memset(&ctrl, 0, sizeof(ctrl));
  ctrl.id = control_ID;
  if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) == 0) {
    xlog("ctrl.value:%d", ctrl.value);
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

  struct v4l2_control ctrl;
  memset(&ctrl, 0, sizeof(ctrl));
  ctrl.id = control_ID;
  ctrl.value = value;

  if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) == 0) {
    xlog("set to:%d", ctrl.value);
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
  ioctl_set_value(V4L2_CID_BRIGHTNESS, value);
}

int AICamera_getContrast() {
  return ioctl_get_value(V4L2_CID_CONTRAST);
}

void AICamera_setContrast(int value) {
  ioctl_set_value(V4L2_CID_CONTRAST, value);
}

int AICamera_getSaturation() {
  return ioctl_get_value(V4L2_CID_SATURATION);
}

void AICamera_setSaturation(int value) {
  ioctl_set_value(V4L2_CID_SATURATION, value);
}

int AICamera_getHue() {
  return ioctl_get_value(V4L2_CID_HUE);
}

void AICamera_setHue(int value) {
  ioctl_set_value(V4L2_CID_HUE, value);
}

int AICamera_getWhiteBalanceAutomatic() {
  return ioctl_get_value(V4L2_CID_AUTO_WHITE_BALANCE);
}

void AICamera_setWhiteBalanceAutomatic(bool enable) {
  ioctl_set_value(V4L2_CID_AUTO_WHITE_BALANCE, enable ? 1 : 0);
}

int AICamera_getExposure() {
  return ioctl_get_value(V4L2_CID_EXPOSURE);
}

void AICamera_setExposure(int value) {
  ioctl_set_value(V4L2_CID_EXPOSURE, value);
}

int AICamera_getWhiteBalanceTemperature() {
  return ioctl_get_value(V4L2_CID_WHITE_BALANCE_TEMPERATURE);
}

void AICamera_setWhiteBalanceTemperature(int value) {
  ioctl_set_value(V4L2_CID_WHITE_BALANCE_TEMPERATURE, value);
}

int AICamera_getExposureAuto() {
  return ioctl_get_value(V4L2_CID_EXPOSURE_AUTO);
}

void AICamera_setExposureAuto(bool enable) {
  ioctl_set_value(V4L2_CID_EXPOSURE_AUTO, enable ? 1 : 0);
}

int AICamera_getFocusAbsolute() {
  return ioctl_get_value(V4L2_CID_FOCUS_ABSOLUTE);
}

void AICamera_setFocusAbsolute(int value) {
  ioctl_set_value(V4L2_CID_FOCUS_ABSOLUTE, value);
}

int AICamera_getFocusAuto() {
  return ioctl_get_value(V4L2_CID_FOCUS_AUTO);
}

void AICamera_setFocusAuto(bool enable) {
  ioctl_set_value(V4L2_CID_FOCUS_AUTO, enable ? 1 : 0);
}

// Callback to handle incoming buffer data
GstPadProbeReturn cb_streaming_data(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
  if (buffer) {
    counterFrame++;
    // xlog("frame captured, counterFrame:%d", counterFrame);

    // set some conditions to save pic
    if (counterFrame % 300 == 0) {
      // Get the capabilities of the pad to understand the format
      GstCaps *caps = gst_pad_get_current_caps(pad);
      if (!caps) {
        xlog("Failed to get caps");
        gst_caps_unref(caps);
        return GST_PAD_PROBE_PASS;
      }
      // Print the entire caps for debugging
      // xlog("caps: %s", gst_caps_to_string(caps));

      // Map the buffer to access its data
      GstMapInfo map;
      if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        // xlog("frame captured, counterFrame:%d, Size:%ld bytes", counterFrame, map.size);
      } else {
        gst_caps_unref(caps);
        xlog("Failed to map buffer");
        return GST_PAD_PROBE_PASS;
      }

      // Get the structure of the first capability (format)
      GstStructure *str = gst_caps_get_structure(caps, 0);
      const gchar *format = gst_structure_get_string(str, "format");
      // xlog("format:%s", format);

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
        oss << "frame_" << std::setw(5) << std::setfill('0') << counterImg << ".jpg";
        // oss << "frame_" << std::setw(5) << std::setfill('0') << counterImg << ".png";
        std::string filename = oss.str();
        if (cv::imwrite(filename, bgr_frame)) {
          xlog("Saved frame to %s", filename.c_str());
        } else {
          xlog("Failed to save frame");
        }
      }
      // Cleanup
      gst_buffer_unmap(buffer, &map);
      gst_caps_unref(caps);
    }
  }
  return GST_PAD_PROBE_OK;
}

void ThreadAICameraStreaming(int param) {
  xlog("start >>>>, param:%d", param);
  counterFrame = 0;
  counterImg = 0;

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // final gst pipeline
  // gst-launch-1.0 -v v4l2src device=/dev/video18 ! video/x-raw,width=2048,height=1536 ! v4l2h264enc extra-controls="cid,video_gop_size=60" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream

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
  xlog("AICamrea_getVideoDevice:%s", AICamrea_getVideoDevice().c_str());
  g_object_set(G_OBJECT(source), "device", AICamrea_getVideoDevice().c_str(), nullptr);

  // Create a GstStructure for extra-controls
  GstStructure *controls = gst_structure_new(
      "extra-controls",                  // Name of the structure
      "video_gop_size", G_TYPE_INT, 60,  // Key-value pair
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

  // Define the capabilities for the capsfilter
  // 2048 * 1536
  GstCaps *caps = gst_caps_new_simple(
      "video/x-raw",
      "width", G_TYPE_INT, 2048,
      "height", G_TYPE_INT, 1536,
      "framerate", GST_TYPE_FRACTION, 30, 1,  // Add frame rate as 30/1
      nullptr);
  g_object_set(capsfilter, "caps", caps, nullptr);
  gst_caps_unref(caps);

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
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_streaming_data, nullptr, nullptr);
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

  // Run the main loop
  gst_loop = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(gst_loop);

  // Stop the pipeline when finished or interrupted
  xlog("Stopping the pipeline...");
  gst_element_set_state(gst_pipeline, GST_STATE_NULL);

  // Clean up
  gst_object_unref(gst_pipeline);
  is_aicamera_streaming = false;
  xlog("stop >>>>, Pipeline stopped and resources cleaned up.");
}

void AICamera_startStreaming() {
  if (is_aicamera_streaming) {
    xlog("thread already running");
    return;
  }
  is_aicamera_streaming = true;
  t_aicamera_streaming = std::thread(ThreadAICameraStreaming, 0);
  t_aicamera_streaming.detach();
}

void AICamera_stopStreaming() {
    if (gst_loop) {
    g_main_loop_quit(gst_loop);
    g_main_loop_unref(gst_loop);
    gst_loop = nullptr;
  } else {
    xlog("gst_loop is invalid or already destroyed.");
  }
}
