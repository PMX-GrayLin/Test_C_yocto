#include "cam_gige_hikrobot.hpp"

// temp
#include "aicamera.hpp"

UsedGigeCam usedGigeCam = ugc_hikrobot;

static GstElement *pipeline_gige_hik = nullptr;
static GMainLoop *loop_gige_hik = nullptr;
static GstElement *source_gige_hik = nullptr;

std::thread t_streaming_gige_hik;
bool isStreaming_gige_hik = false;

struct GigeControlParams gigeControlParams = { 0 };

// Callback to handle incoming buffer data
GstPadProbeReturn streamingDataCallback_gige_hik(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  AICAMERA_saveImage(pad, info);
  return GST_PAD_PROBE_OK;
}

void Gige_handle_RESTful(std::vector<std::string> segments) {

  if (isSameString(segments[1].c_str(), "start")) {
    GigE_StreamingStart_Hik();

  } else if (isSameString(segments[1].c_str(), "stop")) {
    GigE_StreamingStop_Hik();
    
  } else if (isSameString(segments[1].c_str(), "set")) {
    if (isSameString(segments[2].c_str(), "exposure")) {
      GigE_setExposure_hik(segments[3]);
    } else if (isSameString(segments[2].c_str(), "exposure-auto")) {
      GigE_setExposureAuto_hik(segments[3]);
    } else if (isSameString(segments[2].c_str(), "gain")) {
      GigE_setGain_hik(segments[3]);
    } else if (isSameString(segments[2].c_str(), "gain-auto")) {
      GigE_setGainAuto_hik(segments[3]);
    }

  } else if (isSameString(segments[1].c_str(), "get")) {
    GigE_getSettings_hik();
    if (isSameString(segments[2].c_str(), "exposure")) {
      //
    } else if (isSameString(segments[2].c_str(), "exposure-auto")) {
      //
    } else if (isSameString(segments[2].c_str(), "gain")) {
      //
    } else if (isSameString(segments[2].c_str(), "gain-auto")) {
      //
    }
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
  if (isSameString(gstArvAutoS.c_str(), "off") || isSameString(gstArvAutoS.c_str(), "0")) {
    gaa = gaa_off;
  } else if (isSameString(gstArvAutoS.c_str(), "once") || isSameString(gstArvAutoS.c_str(), "1")) {
    gaa = gaa_once;
  } else if (isSameString(gstArvAutoS.c_str(), "cont") || isSameString(gstArvAutoS.c_str(), "2")) {
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
  if (isSameString(gstArvAutoS.c_str(), "off") || isSameString(gstArvAutoS.c_str(), "0")) {
    gaa = gaa_off;
  } else if (isSameString(gstArvAutoS.c_str(), "once") || isSameString(gstArvAutoS.c_str(), "1")) {
    gaa = gaa_once;
  } else if (isSameString(gstArvAutoS.c_str(), "cont") || isSameString(gstArvAutoS.c_str(), "2")) {
    gaa = gaa_continuous;
  }
  xlog("set gain-auto:%d", gaa);
  g_object_set(G_OBJECT(source_gige_hik), "gain-auto", gaa, NULL);
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
  
          GMainLoop *loop = static_cast<GMainLoop *>(user_data);
          g_main_loop_quit(loop);
          break;
        }
  
        case GST_MESSAGE_EOS:
          xlog("Received EOS, stopping...");
          g_main_loop_quit(static_cast<GMainLoop *>(user_data));
          break;
  
        default:
          break;
      }
      return TRUE; }, nullptr);

  // Start streaming
  GstStateChangeReturn ret = gst_element_set_state(pipeline_gige_hik, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("failed to start the pipeline");
    gst_object_unref(pipeline_gige_hik);
    return;
  }

  xlog("pipeline is running...");
  isStreaming_gige_hik = true;

  // Main loop
  loop_gige_hik = g_main_loop_new(nullptr, FALSE);
  gst_bus_set_sync_handler(bus, nullptr, nullptr, nullptr);
  gst_object_unref(bus);
  g_main_loop_run(loop_gige_hik);

  // Clean up
  xlog("Stopping the pipeline...");
  gst_element_set_state(pipeline_gige_hik, GST_STATE_NULL);
  gst_object_unref(pipeline_gige_hik);
  // isStreaming_gige_hik = false;
  
  if (loop_gige_hik) {
    g_main_loop_unref(loop_gige_hik);
    loop_gige_hik = nullptr;
  }

  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");
}

  void GigE_StreamingStart_Hik() {
    xlog("");
    if (isStreaming_gige_hik) {
      xlog("thread already running");
      return;
    }
    isStreaming_gige_hik = true;
    
    // t_streaming_gige_hik = std::thread(GigE_ThreadStreaming_Hik);  
    // t_streaming_gige_hik.detach();

    t_streaming_gige_hik = std::thread([] {
      try {
        GigE_ThreadStreaming_Hik();
      } catch (const std::exception &e) {
        xlog("Exception in streaming thread: %s", e.what());
      } catch (...) {
        xlog("Unknown exception in streaming thread");
      }
    });

  }
  
  void GigE_StreamingStop_Hik() {
    xlog("");
    // if (loop_gige_hik) {
    //   g_main_loop_quit(loop_gige_hik);
    //   g_main_loop_unref(loop_gige_hik);
    //   loop_gige_hik = nullptr;
  
    //   isStreaming_gige_hik = false;
    // } else {
    //   xlog("loop_gige_hik is invalid or already destroyed.");
    // }

    if (!isStreaming_gige_hik) {
      xlog("Streaming not running");
      return;
    }
  
    if (loop_gige_hik) {
      g_main_loop_quit(loop_gige_hik);  // Unref should only happen in the thread
    }
  
    // Optional: wait for thread to finish (if needed)
    if (t_streaming_gige_hik.joinable()) {
      t_streaming_gige_hik.join();
    }
  
    isStreaming_gige_hik = false;
  }
  