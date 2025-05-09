#include "test_gst.hpp"

volatile int counterFrame = 0;
int counterImg = 0;

GstElement *gst_pipeline = nullptr;
GMainLoop *gst_loop = nullptr;

void gst_test(int testCase) {
  xlog("testCase:%d", testCase);

  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;

  // Initialize GStreamer
  // gst_init(&argc, &argv);
  gst_init(nullptr, nullptr);

  guint major, minor, micro, nano;
  gst_version(&major, &minor, &micro, &nano);
  xlog("%d.%d.%d.%d", major, minor, micro, nano);

  // OK
  // gst-launch-1.0 -v v4l2src device=/dev/video47 ! video/x-raw,width=1920,height=1080 ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream

  // Create the pipeline
  string pipelineS =
      "v4l2src device=" + AICamrea_getVideoDevice() + " " +
      "! video/x-raw,width=2048,height=1536 " +
      "! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=dmabuf "
      "! rtspclientsink location=rtsp://localhost:8554/mystream";

  xlog("pipeline:%s", pipelineS.c_str());

  pipeline = gst_parse_launch(pipelineS.c_str(), nullptr);
  if (!pipeline) {
    xlog("gst_parse_launch fail");
    return;
  }

  // Start playing
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  xlog("pipeline is running...");

  // Wait until error or EOS
  bus = gst_element_get_bus(pipeline);
  msg = gst_bus_timed_pop_filtered(
      bus,
      GST_CLOCK_TIME_NONE,
      static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  // Parse message
  if (msg) {
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);
        g_clear_error(&err);
        g_free(debug_info);
        break;
      case GST_MESSAGE_EOS:
        break;
      default:
        break;
    }
    gst_message_unref(msg);
  }

  // Free resources
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return;
}

// Callback to handle incoming buffer data
GstPadProbeReturn cb_have_data(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
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

void gst_test2(int testCase) {
  xlog("testCase:%d", testCase);
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
  xlog("AICamrea_getVideoDevice:%s", AICamrea_getVideoDevice().c_str());
  g_object_set(G_OBJECT(source), "device", AICamrea_getVideoDevice().c_str(), nullptr);

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
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_have_data, nullptr, nullptr);
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
  xlog("Pipeline stopped and resources cleaned up.");
}

void stopPipeline() {
  if (gst_loop) {
    g_main_loop_quit(gst_loop);
    g_main_loop_unref(gst_loop);
    gst_loop = nullptr;
  } else {
    xlog("Main loop is invalid or already destroyed.");
  }
}

void aravisTest() {
  gst_init(nullptr, nullptr);
  // arv_update_device_list();

  // guint n_devices = arv_get_n_devices();
  // if (n_devices == 0) {
  //   xlog("No camera found!");
  //     return;
  // }

  // const char *camera_id = arv_get_device_id(0);
  // xlog("Using camera:%s", camera_id);

  // GError *error = nullptr;
  // ArvCamera *camera = arv_camera_new(camera_id, &error);
  // if (!camera) {
  //   xlog("Failed to create ArvCamera:%s", error->message);
  //     g_error_free(error);
  //     return;
  // }

  // // Set initial exposure
  // arv_camera_set_exposure_time(camera, 5000.0, &error);
  // if (error) {
  //   xlog("Failed to set exposure:%s", error->message);
  //     g_error_free(error);
  //     error = nullptr;
  // }
  xlog("Setting exposure to 5000 µs...");
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // // Create GStreamer pipeline
  // GstElement *pipeline = gst_parse_launch(
  //     "aravissrc camera-name=id1 name=src ! videoconvert ! autovideosink", nullptr);
  // GstElement *source = gst_bin_get_by_name(GST_BIN(pipeline), "src");

  // g_object_set(source, "camera-name", camera_id, nullptr);

  // Create GStreamer pipeline
  // GstElement *pipeline = gst_parse_launch(
  //     "aravissrc camera-name=id1 ! videoconvert ! autovideosink", nullptr);

  // Build the pipeline equivalent to:
  // gst-launch-1.0 aravissrc camera-name=id1 ! videoconvert ! autovideosink
  // const gchar *pipeline_description =
  //     "aravissrc camera-name=id1 ! videoconvert ! autovideosink";

  const gchar *pipeline_description =
      "aravissrc camera-name=id1 ! videoconvert ! video/x-raw,format=NV12 ! v4l2h264enc extra-controls=cid,video_gop_size=30 capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream";
  GstElement *pipeline = gst_parse_launch(pipeline_description, &error);

  if (!pipeline) {
    xlog("Failed to create pipeline:%s", error->message);
    g_error_free(error);
    return;
  }

  // Start playing
  GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("Failed to start pipeline.");
    gst_object_unref(pipeline);
    return;
  }

  std::this_thread::sleep_for(std::chrono::seconds(5));

  // xlog("Setting exposure to 15000 µs...");
  // arv_camera_set_exposure_time(camera, 15000.0, &error);
  // if (error) {
  //   xlog("Failed to change exposure:%s", error->message);
  //     g_error_free(error);
  // }

  std::this_thread::sleep_for(std::chrono::seconds(10));

  xlog("Stopping pipeline...");

  gst_element_set_state(pipeline, GST_STATE_NULL);

  // g_object_unref(source);
  g_object_unref(pipeline);
  g_object_unref(camera);

  return;
}
