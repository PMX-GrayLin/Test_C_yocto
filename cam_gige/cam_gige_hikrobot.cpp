#include "cam_gige_hikrobot.hpp"

#include <atomic>
#include <chrono>

#include <gst/gst.h>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "image_utils.hpp"
#include "restfulx.hpp"
#include "device.hpp"

UsedGigeCam usedGigeCam = ugc_hikrobot;

static GstElement *pipeline_gige_hik = nullptr;
static GMainLoop *loop_gige_hik = nullptr;
static GstElement *source_gige_hik = nullptr;

std::thread t_streaming_gige_hik;
std::atomic<bool> isStreaming_gige_hik{false};
std::chrono::steady_clock::time_point lastStartTime_gige_hik;

struct GigeControlParams gigeControlParams = {0};

bool isCapturePhoto_hik = false;
std::string pathName_savedImage_hik = "";

static volatile int counterFrame_hik = 0;

void Gige_handle_RESTful_hik(std::vector<std::string> segments) {
  if (isSameString(segments[1], "start")) {
    GigE_StreamingStart_Hik();

  } else if (isSameString(segments[1], "stop")) {
    GigE_StreamingStop_Hik();

  } else if (isSameString(segments[1], "set")) {
    if (isSameString(segments[2], "exposure")) {
      GigE_setExposure_hik(segments[3]);
    } else if (isSameString(segments[2], "exposure-auto")) {
      GigE_setExposureAuto_hik(segments[3]);
    } else if (isSameString(segments[2], "gain")) {
      GigE_setGain_hik(segments[3]);
    } else if (isSameString(segments[2], "gain-auto")) {
      GigE_setGainAuto_hik(segments[3]);
    } else if (isSameString(segments[2], "resolution")) {
      // should set before streaming start

    }

  } else if (isSameString(segments[1], "get")) {
    GigE_getSettings_hik();
    if (isSameString(segments[2], "exposure")) {
      //
    } else if (isSameString(segments[2], "exposure-auto")) {
      //
    } else if (isSameString(segments[2], "gain")) {
      //
    } else if (isSameString(segments[2], "gain-auto")) {
      //
    } else if (isSameString(segments[2], "isStreaming")) {
      RESTful_send_streamingStatus_gige_hik(0, isStreaming_gige_hik);
    }

  } else if (isSameString(segments[1], "tp")) {
    xlog("take picture");
    std::string path = "";
    if (segments.size() > 2 && !segments[2].empty()) {
      // ex: curl http://localhost:8765/fw/gige/tp/%252Fhome%252Froot%252Fprimax%252F12345.png
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
    GigE_setImagePath_hik(path.c_str());
    GigE_captureImage_hik();
  }
}

void GigE_saveImage_hik(GstPad *pad, GstPadProbeInfo *info) {
  if (isCapturePhoto_hik) {
    xlog("");
    isCapturePhoto_hik = false;

    imgu_saveImage((void *)pad, (void *)info, pathName_savedImage_hik);
  }
}

void GigE_getSettings_hik() {
  // Get the current values
  double exposure, gain;
  int exposure_auto, gain_auto;

  g_object_get(G_OBJECT(source_gige_hik), "exposure", &exposure, NULL);
  g_object_get(G_OBJECT(source_gige_hik), "gain", &gain, NULL);
  g_object_get(G_OBJECT(source_gige_hik), "exposure-auto", &exposure_auto, NULL);
  g_object_get(G_OBJECT(source_gige_hik), "gain-auto", &gain_auto, NULL);
  xlog("exposure_auto:%d", exposure_auto);
  xlog("exposure:%f", exposure);
  xlog("gain_auto:%d", gain_auto);
  xlog("gain:%f", gain);

  gigeControlParams.exposure_auto = exposure_auto;
  gigeControlParams.exposure = exposure;
  gigeControlParams.gain_auto = gain_auto;
  gigeControlParams.gain = gain;
}

double GigE_getExposure_hik() {
  return gigeControlParams.exposure;
}

void GigE_setExposure_hik(string exposureTimeS) {
  // # arv-tool-0.8 control ExposureTime
  // Hikrobot-MV-CS060-10GM-PRO-K44474092 (192.168.11.22)
  // ExposureTime = 15000 min:25 max:2.49985e+06

  // ?? max value may incorrect, cause gige cam to crash...

  double exposureTime = limitValueInRange(std::stod(exposureTimeS), 25.0, 2490000.0);
  xlog("set exposureTime:%f", exposureTime);
  g_object_set(G_OBJECT(source_gige_hik), "exposure", exposureTime, NULL);
}

GstArvAuto GigE_getExposureAuto_hik() {
  return (GstArvAuto)gigeControlParams.exposure_auto;
}

void GigE_setExposureAuto_hik(string gstArvAutoS) {
  // Exposure auto mode (0 - off, 1 - once, 2 - continuous)
  GstArvAuto gaa = gaa_off;
  if (isSameString(gstArvAutoS, "off") || isSameString(gstArvAutoS, "0")) {
    gaa = gaa_off;
  } else if (isSameString(gstArvAutoS, "once") || isSameString(gstArvAutoS, "1")) {
    gaa = gaa_once;
  } else if (isSameString(gstArvAutoS, "cont") || isSameString(gstArvAutoS, "2")) {
    gaa = gaa_continuous;
  }
  xlog("set exposure-auto:%d", gaa);
  g_object_set(G_OBJECT(source_gige_hik), "exposure-auto", gaa, NULL);
}

double GigE_getGain_hik() {
  return gigeControlParams.gain;
}

void GigE_setGain_hik(string gainS) {
  // # arv-tool-0.8 control Gain
  // Hikrobot-MV-CS060-10GM-PRO-K44474092 (192.168.11.22)
  // Gain = 10.0161 dB min:0 max:23.9812

  double gain = limitValueInRange(std::stod(gainS), 0.0, 23.9);
  xlog("set gain:%f", gain);
  g_object_set(G_OBJECT(source_gige_hik), "gain", gain, NULL);
}

GstArvAuto GigE_getGainAuto_hik() {
  return (GstArvAuto)gigeControlParams.gain_auto;
}

void GigE_setGainAuto_hik(string gstArvAutoS) {
  // Gain auto mode (0 - off, 1 - once, 2 - continuous)
  GstArvAuto gaa = gaa_off;
  if (isSameString(gstArvAutoS, "off") || isSameString(gstArvAutoS, "0")) {
    gaa = gaa_off;
  } else if (isSameString(gstArvAutoS, "once") || isSameString(gstArvAutoS, "1")) {
    gaa = gaa_once;
  } else if (isSameString(gstArvAutoS, "cont") || isSameString(gstArvAutoS, "2")) {
    gaa = gaa_continuous;
  }
  xlog("set gain-auto:%d", gaa);
  g_object_set(G_OBJECT(source_gige_hik), "gain-auto", gaa, NULL);
}

void GigE_setImagePath_hik(const string &imagePath) {
  pathName_savedImage_hik = imagePath;
  xlog("pathName_savedImage_hik:%s", pathName_savedImage_hik.c_str());
}

void GigE_captureImage_hik() {
  if (!isStreaming_gige_hik.load()) {
    xlog("do nothing...camera is not streaming");
    return;
  }
  xlog("");
  isCapturePhoto_hik = true;
}

// Callback to handle incoming buffer data
GstPadProbeReturn streamingDataCallback_gige_hik(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  GigE_streamingLED();
  GigE_saveImage_hik(pad, info);
  return GST_PAD_PROBE_OK;
}

void GigE_ThreadStreaming_Hik() {
  xlog("++++ start ++++");

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // working pipeline
  // gst-launch-1.0 aravissrc camera-name=id1 ! videoconvert ! video/x-raw,format=NV12 ! queue ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! h264parse config-interval=1 ! rtspclientsink location=rtsp://localhost:8554/mystream

  // Create the pipeline
  pipeline_gige_hik = gst_pipeline_new("video-pipeline");

  source_gige_hik = gst_element_factory_make("aravissrc", "source_gige_hik");
  GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  GstElement *queue = gst_element_factory_make("queue", "queue");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *parser = gst_element_factory_make("h264parse", "parser");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!pipeline_gige_hik || !source_gige_hik || !videoconvert || !capsfilter || !queue || !encoder || !parser || !sink) {
    xlog("Failed to create GStreamer elements");
    return;
  }

  // Set camera by ID or name (adjust "id1" if needed)
  g_object_set(G_OBJECT(source_gige_hik), "camera-name", "id1", nullptr);

  // Define the capabilities: NV12 format
  GstCaps *caps = gst_caps_new_simple(
      "video/x-raw",
      // "width", G_TYPE_INT, 3072,
      // "height", G_TYPE_INT, 2048,
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
    gst_object_unref(pipeline_gige_hik);
    return;
  }
  g_object_set(G_OBJECT(encoder), "extra-controls", controls, nullptr);
  gst_structure_free(controls);
  g_object_set(G_OBJECT(encoder), "capture-io-mode", 4, nullptr);  // dmabuf

  // Parser settings
  g_object_set(G_OBJECT(parser), "config-interval", 1, nullptr);

  // RTSP Sink
  g_object_set(G_OBJECT(sink), "location", "rtsp://localhost:8554/mystream", nullptr);

  // Build pipeline
  gst_bin_add_many(GST_BIN(pipeline_gige_hik), source_gige_hik, videoconvert, capsfilter, queue, encoder, parser, sink, nullptr);
  if (!gst_element_link_many(source_gige_hik, videoconvert, capsfilter, queue, encoder, parser, sink, nullptr)) {
    xlog("Failed to link GStreamer elements");
    gst_object_unref(pipeline_gige_hik);
    return;
  }

  // Optional: attach pad probe to monitor frames
  GstPad *pad = gst_element_get_static_pad(encoder, "sink");
  if (pad) {
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)streamingDataCallback_gige_hik, nullptr, nullptr);
    gst_object_unref(pad);
  }

  // Add bus watch to handle errors and EOS
  GstBus *bus = gst_element_get_bus(pipeline_gige_hik);
  gst_bus_add_watch(bus, [](GstBus *, GstMessage *msg, gpointer user_data) -> gboolean {
      switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
          GError *err;
          gchar *dbg;
          gst_message_parse_error(msg, &err, &dbg);
          xlog("Pipeline error: %s", err->message);
          g_error_free(err);
          g_free(dbg);
  
          GigE_StreamingStop_Hik();
          FW_setLED("2","red");
          break;
        }
  
        case GST_MESSAGE_EOS:
          xlog("Received EOS, stopping...");

          GigE_StreamingStop_Hik();
          FW_setLED("2","red");
          break;
  
        default:
          break;
      }
      return TRUE; }, nullptr);

  // Start streaming
  GstStateChangeReturn ret = gst_element_set_state(pipeline_gige_hik, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("failed to start the pipeline");
    gst_element_set_state(pipeline_gige_hik, GST_STATE_NULL);
    gst_object_unref(pipeline_gige_hik);
    isStreaming_gige_hik = false;
    return;
  }

  xlog("pipeline is running...");
  isStreaming_gige_hik = true;
  RESTful_send_streamingStatus_gige_hik(0, isStreaming_gige_hik);

  // Main loop
  loop_gige_hik = g_main_loop_new(nullptr, FALSE);
  gst_bus_set_sync_handler(bus, nullptr, nullptr, nullptr);
  gst_object_unref(bus);
  g_main_loop_run(loop_gige_hik);

  // Clean up
  xlog("Stopping the pipeline...");
  gst_element_set_state(pipeline_gige_hik, GST_STATE_NULL);
  gst_object_unref(pipeline_gige_hik);
  if (loop_gige_hik) {
    g_main_loop_unref(loop_gige_hik);
    loop_gige_hik = nullptr;
  }

  FW_CheckNetLinkState("eth1");

  isStreaming_gige_hik = false;
  RESTful_send_streamingStatus_gige_hik(0, isStreaming_gige_hik);
  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");
}

void GigE_StreamingStart_Hik() {
  xlog("");
  auto now = std::chrono::steady_clock::now();
  if (now - lastStartTime_gige_hik < std::chrono::seconds(1)) {
    xlog("Start called too soon, ignoring");
    return;
  }
  lastStartTime_gige_hik = now;

  if (isStreaming_gige_hik.load()) {
    xlog("thread already running");
    return;
  }

  FW_setLED("2", "off");
  t_streaming_gige_hik = std::thread(GigE_ThreadStreaming_Hik);
  t_streaming_gige_hik.detach();
}

void GigE_StreamingStop_Hik() {
  xlog("");
  if (!isStreaming_gige_hik.load()) {
    xlog("thread not running");
    return;
  }

  if (loop_gige_hik) {
    xlog("g_main_loop_quit");
    g_main_loop_quit(loop_gige_hik);  // Unref should only happen in the thread
  } else {
    xlog("gst_loop_uvc is invalid or already destroyed.");
  }

  // isStreaming_gige_hik = false;
}

void GigE_streamingLED() {
  counterFrame_hik++;
  if (counterFrame_hik%15 == 0)
  {
    FW_toggleLED("2", "orange");
  }
}
