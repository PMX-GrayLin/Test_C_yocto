#include "cam_gige_hikrobot.hpp"

#include <gst/gst.h>

#include <atomic>
#include <chrono>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "device.hpp"
#include "image_utils.hpp"
#include "restfulx.hpp"

UsedGigeCam usedGigeCam = ugc_hikrobot;

static GstElement *pipeline_gige_hik[NUM_GigE] = {nullptr, nullptr};
static GMainLoop *loop_gige_hik[NUM_GigE] = {nullptr, nullptr};
static GstElement *source_gige_hik[NUM_GigE] = {nullptr, nullptr};

std::thread t_streaming_gige_hik[NUM_GigE];
std::atomic<bool> isStreaming_gige_hik[NUM_GigE]{false, false};
std::chrono::steady_clock::time_point lastStartTime_gige_hik[NUM_GigE];
static int resolution_width_gige_hik[NUM_GigE] = {1920, 1920};
static int resolution_height_gige_hik[NUM_GigE] = {1080, 1080};

struct GigeControlParams gigeControlParams[NUM_GigE] = {};

bool isCapturePhoto_hik[NUM_GigE] = {false, false};
std::string pathName_savedImage_hik[NUM_GigE] = {"", ""};

static volatile int counterFrame_hik[NUM_GigE] = {0, 0};

static bool isSDKInit_gige_hik = false;
void* handle_gige_hik[NUM_GigE] = {nullptr, nullptr};

void Gige_handle_RESTful_hik(std::vector<std::string> segments) {
  int index_cam = 0;
  if (isSameString(segments[0], "gige") || isSameString(segments[1], "gige1")) {
    index_cam = 0;
  } else if (isSameString(segments[0], "gige2")) {
    index_cam = 1;
  }

  if (isSameString(segments[1], "start")) {
    GigE_StreamingStart_Hik(index_cam);

  } else if (isSameString(segments[1], "stop")) {
    GigE_StreamingStop_Hik(index_cam);

  } else if (isSameString(segments[1], "set")) {
    if (isSameString(segments[2], "exposure")) {
      GigE_setExposure_hik(index_cam, segments[3]);
    } else if (isSameString(segments[2], "exposure-auto")) {
      GigE_setExposureAuto_hik(index_cam, segments[3]);
    } else if (isSameString(segments[2], "gain")) {
      GigE_setGain_hik(index_cam, segments[3]);
    } else if (isSameString(segments[2], "gain-auto")) {
      GigE_setGainAuto_hik(index_cam, segments[3]);
    } else if (isSameString(segments[2], "resolution")) {
      // with format "width*height"
      GigE_setResolution(index_cam, segments[3]);
    }

  } else if (isSameString(segments[1], "get")) {
    GigE_getSettings_hik(index_cam);
    if (isSameString(segments[2], "exposure")) {
      //
    } else if (isSameString(segments[2], "exposure-auto")) {
      //
    } else if (isSameString(segments[2], "gain")) {
      //
    } else if (isSameString(segments[2], "gain-auto")) {
      //
    } else if (isSameString(segments[2], "isStreaming")) {
      RESTful_send_streamingStatus_gige_hik(index_cam, isStreaming_gige_hik[index_cam]);
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
    GigE_setImagePath_hik(index_cam, path);
    GigE_captureImage_hik(index_cam);

  } else if (isSameString(segments[1], "t0")) {
    xlog("t0");
    if (isSameString(segments[2], "1")) {
      GigE_cameraOpen(index_cam);
    } else {
      GigE_cameraClose(index_cam);
    }

  } else if (isSameString(segments[1], "t1")) {
    xlog("t1");
    GigE_setTriggerMode(index_cam, segments[2]);
  } else if (isSameString(segments[1], "t2")) {
    xlog("t2");
    GigE_sendTriggerSoftware(index_cam);
  }
}

void GigE_saveImage_hik(int index_cam, GstPad *pad, GstPadProbeInfo *info) {
  if (isCapturePhoto_hik[index_cam]) {
    xlog("");
    isCapturePhoto_hik[index_cam] = false;

    imgu_saveImage((void *)pad, (void *)info, pathName_savedImage_hik[index_cam]);
  }
}

void GigE_getSettings_hik(int index_cam) {
  if (index_cam < 0 || index_cam >= NUM_GigE) {
    xlog("Invalid index_cam: %d", index_cam);
    return;
  }
  if (!source_gige_hik[index_cam]) {
    xlog("Camera source not initialized for index %d", index_cam);
    return;
  }
  // Get the current values
  double exposure, gain;
  int exposure_auto, gain_auto;

  g_object_get(G_OBJECT(source_gige_hik[index_cam]), "exposure", &exposure, NULL);
  g_object_get(G_OBJECT(source_gige_hik[index_cam]), "gain", &gain, NULL);
  g_object_get(G_OBJECT(source_gige_hik[index_cam]), "exposure-auto", &exposure_auto, NULL);
  g_object_get(G_OBJECT(source_gige_hik[index_cam]), "gain-auto", &gain_auto, NULL);
  xlog("exposure_auto:%d", exposure_auto);
  xlog("exposure:%f", exposure);
  xlog("gain_auto:%d", gain_auto);
  xlog("gain:%f", gain);

  gigeControlParams[index_cam].exposure_auto = exposure_auto;
  gigeControlParams[index_cam].exposure = exposure;
  gigeControlParams[index_cam].gain_auto = gain_auto;
  gigeControlParams[index_cam].gain = gain;
}

double GigE_getExposure_hik(int index_cam) {
  if (index_cam < 0 || index_cam >= NUM_GigE) {
    xlog("Invalid index_cam: %d", index_cam);
    return -1.0;
  }
  if (!source_gige_hik[index_cam]) {
    xlog("Camera source not initialized for index %d", index_cam);
    return -1.0;
  }
  return gigeControlParams[index_cam].exposure;
}

void GigE_setExposure_hik(int index_cam, const string &exposureTimeS) {
  // # arv-tool-0.8 control ExposureTime
  // Hikrobot-MV-CS060-10GM-PRO-K44474092 (192.168.11.22)
  // ExposureTime = 15000 min:25 max:2.49985e+06

  // ?? max value may incorrect, cause gige cam to crash...

  if (index_cam < 0 || index_cam >= NUM_GigE) {
    xlog("Invalid index_cam: %d", index_cam);
    return;
  }
  if (!source_gige_hik[index_cam]) {
    xlog("Camera source not initialized for index %d", index_cam);
    return;
  }

  double exposureTime = 0.0;
  try {
    exposureTime = std::stod(exposureTimeS);
  } catch (const std::exception &e) {
    xlog("Invalid exposureTime string: '%s' (%s)", exposureTimeS.c_str(), e.what());
    return;
  }

  if (!std::isfinite(exposureTime)) {
    xlog("Exposure time is not finite: %f", exposureTime);
    return;
  }

  exposureTime = limitValueInRange(std::stod(exposureTimeS), 25.0, 2490000.0);
  xlog("Setting exposureTime: %f", exposureTime);
  g_object_set(G_OBJECT(source_gige_hik[index_cam]), "exposure", exposureTime, nullptr);
}

GstArvAuto GigE_getExposureAuto_hik(int index_cam) {
  if (index_cam < 0 || index_cam >= NUM_GigE) {
    xlog("Invalid index_cam: %d", index_cam);
    return gaa_invalid;
  }
  if (!source_gige_hik[index_cam]) {
    xlog("Camera source not initialized for index %d", index_cam);
    return gaa_invalid;
  }
  return (GstArvAuto)gigeControlParams[index_cam].exposure_auto;
}

void GigE_setExposureAuto_hik(int index_cam, const string &gstArvAutoS) {
  // # arv-tool-0.8 control Gain
  // Hikrobot-MV-CS060-10GM-PRO-K44474092 (192.168.11.22)
  // Gain = 10.0161 dB min:0 max:23.9812

  if (index_cam < 0 || index_cam >= NUM_GigE) {
    xlog("Invalid index_cam: %d", index_cam);
    return;
  }
  if (!source_gige_hik[index_cam]) {
    xlog("Camera source not initialized for index %d", index_cam);
    return;
  }

  GstArvAuto gaa = gaa_off;  // default

  if (isSameString(gstArvAutoS, "off") || isSameString(gstArvAutoS, "0")) {
    gaa = gaa_off;
  } else if (isSameString(gstArvAutoS, "once") || isSameString(gstArvAutoS, "1")) {
    gaa = gaa_once;
  } else if (isSameString(gstArvAutoS, "cont") || isSameString(gstArvAutoS, "continuous") || isSameString(gstArvAutoS, "2")) {
    gaa = gaa_continuous;
  } else {
    xlog("Invalid exposure-auto string: '%s', defaulting to OFF", gstArvAutoS.c_str());
  }

  xlog("Setting exposure-auto mode: %d (%s)",
       gaa,
       (gaa == gaa_off ? "off" : gaa == gaa_once ? "once"
                                                 : "continuous"));

  g_object_set(G_OBJECT(source_gige_hik[index_cam]), "exposure-auto", gaa, nullptr);
}

double GigE_getGain_hik(int index_cam) {
  if (index_cam < 0 || index_cam >= NUM_GigE) {
    xlog("Invalid index_cam: %d", index_cam);
    return -1.0;
  }
  if (!source_gige_hik[index_cam]) {
    xlog("Camera source not initialized for index %d", index_cam);
    return -1.0;
  }
  return gigeControlParams[index_cam].gain;
}

void GigE_setGain_hik(int index_cam, const string &gainS) {
  if (index_cam < 0 || index_cam >= NUM_GigE) {
    xlog("Invalid index_cam: %d", index_cam);
    return;
  }
  if (!source_gige_hik[index_cam]) {
    xlog("Camera source not initialized for index %d", index_cam);
    return;
  }

  double gain = limitValueInRange(std::stod(gainS), 0.0, 23.9);
  xlog("set gain:%f", gain);
  g_object_set(G_OBJECT(source_gige_hik[index_cam]), "gain", gain, NULL);
}

GstArvAuto GigE_getGainAuto_hik(int index_cam) {
  if (index_cam < 0 || index_cam >= NUM_GigE) {
    xlog("Invalid index_cam: %d", index_cam);
    return gaa_invalid;
  }
  if (!source_gige_hik[index_cam]) {
    xlog("Camera source not initialized for index %d", index_cam);
    return gaa_invalid;
  }
  return (GstArvAuto)gigeControlParams[index_cam].gain_auto;
}

void GigE_setGainAuto_hik(int index_cam, const string &gstArvAutoS) {
  // Gain auto mode (0 - off, 1 - once, 2 - continuous)
  if (index_cam < 0 || index_cam >= NUM_GigE) {
    xlog("Invalid index_cam: %d", index_cam);
    return;
  }
  if (!source_gige_hik[index_cam]) {
    xlog("Camera source not initialized for index %d", index_cam);
    return;
  }

  GstArvAuto gaa = gaa_off;
  if (isSameString(gstArvAutoS, "off") || isSameString(gstArvAutoS, "0")) {
    gaa = gaa_off;
  } else if (isSameString(gstArvAutoS, "once") || isSameString(gstArvAutoS, "1")) {
    gaa = gaa_once;
  } else if (isSameString(gstArvAutoS, "cont") || isSameString(gstArvAutoS, "2")) {
    gaa = gaa_continuous;
  }
  xlog("Setting exposure-auto mode: %d (%s)",
       gaa,
       (gaa == gaa_off ? "off" : gaa == gaa_once ? "once"
                                                 : "continuous"));

  g_object_set(G_OBJECT(source_gige_hik[index_cam]), "gain-auto", gaa, NULL);
}

void GigE_setImagePath_hik(int index_cam, const string &imagePath) {
  pathName_savedImage_hik[index_cam] = imagePath;
  xlog("pathName_savedImage_hik[%d]:%s", index_cam, pathName_savedImage_hik[index_cam].c_str());
}

void GigE_captureImage_hik(int index_cam) {
  if (!isStreaming_gige_hik[index_cam].load()) {
    xlog("do nothing...camera is not streaming");
    return;
  }
  xlog("");
  isCapturePhoto_hik[index_cam] = true;
}

// Callback to handle incoming buffer data
// Generic callback for all cameras
GstPadProbeReturn streamingDataCallback_gige_hik(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  int index_cam = GPOINTER_TO_INT(user_data);
  GigE_streamingLED(index_cam);
  GigE_saveImage_hik(index_cam, pad, info);
  return GST_PAD_PROBE_OK;
}

void GigE_ThreadStreaming_Hik(int index_cam) {
  xlog("++++ start ++++");

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // working pipeline
  // gst-launch-1.0 aravissrc camera-name=id1 ! videoconvert ! video/x-raw,format=NV12 ! queue ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! h264parse config-interval=1 ! rtspclientsink location=rtsp://localhost:8554/mystream

  // Create the pipeline
  std::string pipelineS = "video-pipeline_" + std::to_string(index_cam);
  pipeline_gige_hik[index_cam] = gst_pipeline_new(pipelineS.c_str());

  std::string sourceS = "source_gige_hik_" + std::to_string(index_cam);
  source_gige_hik[index_cam] = gst_element_factory_make("aravissrc", sourceS.c_str());
  GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  GstElement *queue = gst_element_factory_make("queue", "queue");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *parser = gst_element_factory_make("h264parse", "parser");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!pipeline_gige_hik[index_cam] || !source_gige_hik[index_cam] || !videoconvert || !capsfilter || !queue || !encoder || !parser || !sink) {
    xlog("Failed to create GStreamer elements");
    return;
  }

  // Set camera by ID or name (adjust "id1" if needed)
  std::string cameraNameS = "id" + std::to_string(index_cam + 1);
  g_object_set(G_OBJECT(source_gige_hik[index_cam]), "camera-name", cameraNameS.c_str(), nullptr);

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
    gst_object_unref(pipeline_gige_hik[index_cam]);
    return;
  }
  g_object_set(G_OBJECT(encoder), "extra-controls", controls, nullptr);
  gst_structure_free(controls);
  g_object_set(G_OBJECT(encoder), "capture-io-mode", 4, nullptr);  // dmabuf

  // Parser settings
  g_object_set(G_OBJECT(parser), "config-interval", 1, nullptr);

  // RTSP Sink
  if (index_cam == 1) {
    g_object_set(G_OBJECT(sink), "location", "rtsp://localhost:8554/mystream2", nullptr);
  } else {
    g_object_set(G_OBJECT(sink), "location", "rtsp://localhost:8554/mystream", nullptr);
  }

  // Build pipeline
  gst_bin_add_many(GST_BIN(pipeline_gige_hik[index_cam]), source_gige_hik[index_cam], videoconvert, capsfilter, queue, encoder, parser, sink, nullptr);
  if (!gst_element_link_many(source_gige_hik[index_cam], videoconvert, capsfilter, queue, encoder, parser, sink, nullptr)) {
    xlog("Failed to link GStreamer elements");
    gst_object_unref(pipeline_gige_hik[index_cam]);
    return;
  }

  // Optional: attach pad probe to monitor frames
  GstPad *pad = gst_element_get_static_pad(encoder, "sink");
  if (pad) {
    gst_pad_add_probe(
        pad,
        GST_PAD_PROBE_TYPE_BUFFER,
        streamingDataCallback_gige_hik,
        GINT_TO_POINTER(index_cam),  // pass index_cam here
        nullptr);
    gst_object_unref(pad);
  }

  // Add bus watch to handle errors and EOS
  GstBus *bus = gst_element_get_bus(pipeline_gige_hik[index_cam]);
  gst_bus_add_watch(
      bus,
      [](GstBus *, GstMessage *msg, gpointer user_data) -> gboolean {
        int index_cam = GPOINTER_TO_INT(user_data);

        switch (GST_MESSAGE_TYPE(msg)) {
          case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *dbg;
            gst_message_parse_error(msg, &err, &dbg);
            xlog("Pipeline error: %s", err->message);
            g_error_free(err);
            g_free(dbg);

            GigE_StreamingStop_Hik(index_cam);
            if (index_cam == 0) {
              FW_setLED("2", "red");
            } else if (index_cam == 1) {
              FW_setLED("3", "red");
            }
            break;
          }
          case GST_MESSAGE_EOS:
            xlog("Received EOS, stopping...");
            GigE_StreamingStop_Hik(index_cam);
            if (index_cam == 0) {
              FW_setLED("2", "red");
            } else if (index_cam == 1) {
              FW_setLED("3", "red");
            }
            break;

          default:
            break;
        }
        return TRUE;
      },
      GINT_TO_POINTER(index_cam)  // <-- pass index_cam here
  );

  // Start streaming
  GstStateChangeReturn ret = gst_element_set_state(pipeline_gige_hik[index_cam], GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("failed to start the pipeline");
    gst_element_set_state(pipeline_gige_hik[index_cam], GST_STATE_NULL);
    gst_object_unref(pipeline_gige_hik[index_cam]);
    isStreaming_gige_hik[index_cam] = false;
    return;
  }

  xlog("pipeline is running...");
  isStreaming_gige_hik[index_cam] = true;
  RESTful_send_streamingStatus_gige_hik(index_cam, isStreaming_gige_hik[index_cam]);

  // Main loop
  loop_gige_hik[index_cam] = g_main_loop_new(nullptr, FALSE);
  gst_bus_set_sync_handler(bus, nullptr, nullptr, nullptr);
  gst_object_unref(bus);
  g_main_loop_run(loop_gige_hik[index_cam]);

  // Clean up
  xlog("Stopping the pipeline...");
  gst_element_set_state(pipeline_gige_hik[index_cam], GST_STATE_NULL);
  gst_object_unref(pipeline_gige_hik[index_cam]);
  if (loop_gige_hik[index_cam]) {
    g_main_loop_unref(loop_gige_hik[index_cam]);
    loop_gige_hik[index_cam] = nullptr;
  }

  std::string ifS = "eth" + std::to_string(index_cam + 1);
  FW_CheckNetLinkState(ifS.c_str());

  isStreaming_gige_hik[index_cam] = false;
  RESTful_send_streamingStatus_gige_hik(0, isStreaming_gige_hik[index_cam]);
  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");
}

void GigE_StreamingStart_Hik(int index_cam) {
  xlog("");
  auto now = std::chrono::steady_clock::now();
  if (now - lastStartTime_gige_hik[index_cam] < std::chrono::seconds(1)) {
    xlog("Start called too soon, ignoring");
    return;
  }
  lastStartTime_gige_hik[index_cam] = now;

  if (isStreaming_gige_hik[index_cam].load()) {
    xlog("thread already running");
    return;
  }

  if (index_cam == 0) {
    FW_setLED("2", "off");
  } else if (index_cam == 1) {
    FW_setLED("3", "off");
  }

  t_streaming_gige_hik[index_cam] = std::thread(GigE_ThreadStreaming_Hik, index_cam);
  t_streaming_gige_hik[index_cam].detach();
}

void GigE_StreamingStop_Hik(int index_cam) {
  xlog("");
  if (!isStreaming_gige_hik[index_cam].load()) {
    xlog("thread not running");
    return;
  }

  if (loop_gige_hik[index_cam]) {
    xlog("g_main_loop_quit");
    g_main_loop_quit(loop_gige_hik[index_cam]);  // Unref should only happen in the thread
  } else {
    xlog("loop_gige_hik is invalid or already destroyed.");
  }
}

void GigE_streamingLED(int index_cam) {
  if (index_cam == 0) {
    counterFrame_hik[index_cam]++;
    if (counterFrame_hik[index_cam] % 15 == 0) {
      FW_toggleLED("2", "orange");
    }
  } else if (index_cam == 1) {
    counterFrame_hik[index_cam]++;
    if (counterFrame_hik[index_cam] % 15 == 0) {
      FW_toggleLED("3", "orange");
    }
  }
}

void GigE_setResolution(int index, const string &resolutionS) {
  size_t sep = resolutionS.find('*');
  if (sep == std::string::npos) {
    xlog("Invalid resolution format. Expected format: width*height");
    return;
  }

  if (index < 0 || index >= NUM_GigE) {
    xlog("Invalid index: %d", index);
    return;
  }

  int width = std::stoi(resolutionS.substr(0, sep));
  int height = std::stoi(resolutionS.substr(sep + 1));

  resolution_width_gige_hik[index] = width;
  resolution_height_gige_hik[index] = height;
  xlog("index:%d, width:%d, height:%d", index, resolution_width_gige_hik[index], resolution_height_gige_hik[index]);
}


// from hikrobot sample
bool GigE_PrintDeviceInfo(MV_CC_DEVICE_INFO *pstMVDevInfo) {
    if (!pstMVDevInfo) {
        xlog("The Pointer of pstMVDevInfo is NULL!");
        return false;
    }

    switch (pstMVDevInfo->nTLayerType) {
        case MV_GIGE_DEVICE: {
            int nIp1 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24;
            int nIp2 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16;
            int nIp3 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8;
            int nIp4 =  pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff;

            xlog("Device Model Name: %s", pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
            xlog("CurrentIp: %d.%d.%d.%d", nIp1, nIp2, nIp3, nIp4);
            xlog("UserDefinedName: %s", pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
            break;
        }
        case MV_USB_DEVICE:
            xlog("Device Model Name: %s", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chModelName);
            xlog("UserDefinedName: %s", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
            break;

        case MV_GENTL_GIGE_DEVICE:
        case MV_GENTL_CAMERALINK_DEVICE:
        case MV_GENTL_CXP_DEVICE:
        case MV_GENTL_XOF_DEVICE: {
            // 統一輸出格式
            auto &info = pstMVDevInfo->SpecialInfo;
            xlog("UserDefinedName: %s", info.stGigEInfo.chUserDefinedName);
            xlog("Serial Number: %s", info.stGigEInfo.chSerialNumber);
            xlog("Model Name: %s", info.stGigEInfo.chModelName);
            break;
        }

        default:
            xlog("Not supported device type: %d", pstMVDevInfo->nTLayerType);
            break;
    }

    return true;
}

void GigE_cameraOpen(int index_cam) {
  xlog("index_cam:%d", index_cam);
  int nRet = MV_OK;

  // Initialize SDK
  if (!isSDKInit_gige_hik == false) {
    isSDKInit_gige_hik = true;
    nRet = MV_CC_Initialize();
    if (MV_OK != nRet) {
      xlog("Initialize SDK fail! nRet [0x%x]\n", nRet);
      return;
    }
  }

  MV_CC_DEVICE_INFO_LIST stDeviceList;
  memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

  // enum device
  nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE | MV_GENTL_CAMERALINK_DEVICE | MV_GENTL_CXP_DEVICE | MV_GENTL_XOF_DEVICE, &stDeviceList);
  if (MV_OK != nRet) {
    xlog("MV_CC_EnumDevices fail! nRet [%x]", nRet);
    return;
  }
  if (stDeviceList.nDeviceNum > 0) {
    for (int i = 0; i < stDeviceList.nDeviceNum; i++) {
      xlog("[device %d]", i);
      MV_CC_DEVICE_INFO *pDeviceInfo = stDeviceList.pDeviceInfo[i];
      if (NULL == pDeviceInfo) {
        return;
      }
      GigE_PrintDeviceInfo(pDeviceInfo);
    }
  } else {
    xlog("Find No Devices!");
    return;
  }

  unsigned int nIndex = 0;
  // select device and create handle
  if (handle_gige_hik[index_cam] == nullptr) {
    nRet = MV_CC_CreateHandle(&handle_gige_hik[index_cam], stDeviceList.pDeviceInfo[nIndex]);
    if (MV_OK != nRet) {
      xlog("MV_CC_CreateHandle fail! nRet [%x]", nRet);
      return;
    }
  }

  // open device
  nRet = MV_CC_OpenDevice(handle_gige_hik[index_cam]);
  if (MV_OK != nRet) {
    xlog("MV_CC_OpenDevice fail! nRet [%x]", nRet);
    return;
  }

  // Detection network optimal package size(It only works for the GigE camera)
  if (stDeviceList.pDeviceInfo[nIndex]->nTLayerType == MV_GIGE_DEVICE) {
    int nPacketSize = MV_CC_GetOptimalPacketSize(handle_gige_hik[index_cam]);
    if (nPacketSize > 0) {
      nRet = MV_CC_SetIntValueEx(handle_gige_hik[index_cam], "GevSCPSPacketSize", nPacketSize);
      if (nRet != MV_OK) {
        xlog("Warning: Set Packet Size fail nRet [0x%x]!", nRet);
      }
    } else {
      xlog("Warning: Get Packet Size fail nRet [0x%x]!", nPacketSize);
    }
  }

  nRet = MV_CC_SetBoolValue(handle_gige_hik[index_cam], "AcquisitionFrameRateEnable", false);
  if (MV_OK != nRet) {
    xlog("set AcquisitionFrameRateEnable fail! nRet [%x]", nRet);
  }
}

void GigE_cameraClose(int index_cam) {
  xlog("");
  if (handle_gige_hik[index_cam]) {
    MV_CC_StopGrabbing(handle_gige_hik[index_cam]);
    MV_CC_CloseDevice(handle_gige_hik[index_cam]);
    MV_CC_DestroyHandle(handle_gige_hik[index_cam]);
    handle_gige_hik[index_cam] = nullptr;
  }
}

void __stdcall GigE_imageCallback(MV_FRAME_OUT *pstFrame, void *pUser, bool bAutoFree) {
  if (pstFrame) {
    xlog("Get One Frame: W[%d], H[%d], Num[%d]",
         pstFrame->stFrameInfo.nExtendWidth,
         pstFrame->stFrameInfo.nExtendHeight,
         pstFrame->stFrameInfo.nFrameNum);

    // release manually
    if (!bAutoFree && pUser) {
      auto handle = reinterpret_cast<void *>(pUser);
      MV_CC_FreeImageBuffer(handle, pstFrame);
    }
  }
}

void GigE_setTriggerMode(int index_cam, const string &triggerModeS) {
  xlog("%s", triggerModeS.c_str());

  // set trigger mode
  bool enable = (triggerModeS == "on");
  int nRet = MV_CC_SetEnumValue(handle_gige_hik[index_cam], "TriggerMode", enable ? 1 : 0);
  if (MV_OK != nRet) {
    xlog("MV_CC_SetTriggerMode fail! nRet [%x]", nRet);
    return;
  }

  if (enable) {
    // set trigger source
    nRet = MV_CC_SetEnumValue(handle_gige_hik[index_cam], "TriggerSource", MV_TRIGGER_SOURCE_SOFTWARE);
    if (MV_OK != nRet) {
      xlog("MV_CC_SetTriggerSource fail! nRet [%x]", nRet);
      return;
    }

    // register image callback
    nRet = MV_CC_RegisterImageCallBackEx2(
        handle_gige_hik[index_cam],
        GigE_imageCallback,
        handle_gige_hik[index_cam],
        true);
    if (MV_OK != nRet) {
      xlog("MV_CC_RegisterImageCallBackEx fail! nRet [%x]", nRet);
      return;
    }

    // start grab image
    nRet = MV_CC_StartGrabbing(handle_gige_hik[index_cam]);
    if (MV_OK != nRet) {
      xlog("MV_CC_StartGrabbing fail! nRet [%x]", nRet);
      return;
    }

  } else {

    // end grab image
    nRet = MV_CC_StopGrabbing(handle_gige_hik[index_cam]);
    if (MV_OK != nRet) {
      xlog("MV_CC_StopGrabbing fail! nRet [%x]", nRet);
      return;
    }
  }
}

void GigE_sendTriggerSoftware(int index_cam) {
  if (handle_gige_hik[index_cam]) {
    int nRet = MV_CC_SetCommandValue(handle_gige_hik[index_cam], "TriggerSoftware");
    if (MV_OK != nRet) {
      xlog("failed in TriggerSoftware[%x]\n", nRet);
      return;
    }
  }
}
