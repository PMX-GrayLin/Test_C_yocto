#include "test_gst.hpp"

#include "test.hpp"
#include "aicamerag2.hpp"
#include <opencv2/opencv.hpp>

int counterFrame = 0;
int counterImg = 0;

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
  // gst-launch-1.0 -v v4l2src device=/dev/video47 ! video/x-raw,width=1920,height=1080 ! v4l2h264enc extra-controls="cid,video_gop_size=60" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream

  // Create the pipeline
  string pipelineS =
      "v4l2src device=" + AICamrea_getVideoDevice() + " " +
      "! video/x-raw,width=2048,height=1536 " +
      "! v4l2h264enc extra-controls=\"cid,video_gop_size=60\" capture-io-mode=dmabuf "
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
    xlog("frame captured, counterFrame:%d", counterFrame);

    // set some conditions to save pic
    if (counterFrame % 200 == 0) {
      buffer = gst_buffer_ref(buffer);

      // Get the capabilities of the pad to understand the format
      GstCaps *caps = gst_pad_get_current_caps(pad);
      if (!caps) {
        xlog("Failed to get caps");
        return GST_PAD_PROBE_PASS;
      }
      // Print the entire caps for debugging
      // xlog("caps: %s", gst_caps_to_string(caps));

      // Map the buffer to access its data
      GstMapInfo map;
      if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        xlog("frame captured, counterFrame:%d, Size:%ld bytes", counterFrame, map.size);
      } else {
        xlog("Failed to map buffer");
      }

      // Get the structure of the first capability (format)
      GstStructure *str = gst_caps_get_structure(caps, 0);
      const gchar *format = gst_structure_get_string(str, "format");
      // xlog("format:%s", format);

      // Only proceed if the format is NV12
      if (format && g_strcmp0(format, "NV12") == 0) {
        int width = 0, height = 0;
        if (gst_structure_get_int(str, "width", &width) &&
            gst_structure_get_int(str, "height", &height)) {
          // xlog("Video dimensions: %dx%d", width, height);
        } else {
          xlog("Failed to get video dimensions");
        }

        // NV12 has 1.5x the size of the Y plane
        // size_t y_size = width * height;
        // size_t uv_size = y_size / 2;  // UV plane is half the size of Y plane

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
        std::string filename = oss.str();
        if (cv::imwrite(filename, bgr_frame)) {
          xlog("Saved frame to %s", filename.c_str());
        } else {
          xlog("Failed to save frame");
        }
      }
      gst_buffer_unref(buffer);
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

  // gst-launch-1.0 -v v4l2src device=/dev/video18 ! video/x-raw,width=2048,height=1536 ! v4l2h264enc extra-controls="cid,video_gop_size=60" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream

  // Create the elements
  GstElement *pipeline = gst_pipeline_new("video-pipeline");
  GstElement *source = gst_element_factory_make("v4l2src", "source");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!pipeline || !source || !capsfilter || !encoder || !sink) {
    xlog("failed to create GStreamer elements");
    return;
  }

  // Set properties for the elements
  xlog("AICamrea_getVideoDevice:%s", AICamrea_getVideoDevice().c_str());
  g_object_set( G_OBJECT(source), "device", AICamrea_getVideoDevice().c_str(), nullptr );

  // Create a GstStructure for extra-controls
  GstStructure *controls = gst_structure_new(
      "extra-controls",                  // Name of the structure
      "video_gop_size", G_TYPE_INT, 60,  // Key-value pair
      nullptr                            // End of key-value pairs
  );
  if (!controls) {
    xlog("Failed to create GstStructure");
    gst_object_unref(pipeline);
    return;
  }
  g_object_set(G_OBJECT(encoder), "extra-controls", controls, nullptr);
  // Free the GstStructure after use
  gst_structure_free(controls);

  g_object_set(encoder, "capture-io-mode", 4, nullptr);  // dmabuf = 4
  g_object_set(sink, "location", "rtsp://localhost:8554/mystream", nullptr);

  // Define the capabilities for the capsfilter
  GstCaps *caps = gst_caps_new_simple(
      "video/x-raw",
      "width", G_TYPE_INT, 2048,
      "height", G_TYPE_INT, 1536,
      nullptr);
  g_object_set(capsfilter, "caps", caps, nullptr);
  gst_caps_unref(caps);

  // Build the pipeline
  gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, encoder, sink, nullptr);
  if (!gst_element_link_many(source, capsfilter, encoder, sink, nullptr)) {
    xlog("failed to link elements in the pipeline");
    gst_object_unref(pipeline);
    return;
  }

  // Attach pad probe to capture frames
  GstPad *pad = gst_element_get_static_pad(encoder, "sink");
  if (pad) {
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_have_data, nullptr, nullptr);
    gst_object_unref(pad);
  }

  // Start the pipeline
  GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("failed to start the pipeline");
    gst_object_unref(pipeline);
    return;
  }

  xlog("pipeline is running...");

  // Wait until an error or EOS
  GstBus *bus = gst_element_get_bus(pipeline);
  GstMessage *msg;
  do {
    msg = gst_bus_timed_pop_filtered(
      bus, 
      GST_CLOCK_TIME_NONE,
      (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != nullptr) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error(msg, &err, &debug_info);
          xlog("GST_MESSAGE_ERROR. err->message%s", err->message);
          g_error_free(err);
          g_free(debug_info);
          break;

        case GST_MESSAGE_EOS:
          xlog("GST_MESSAGE_EOS, End-Of-Stream reached");
          break;

        default:
          xlog("default, Unexpected message received");
          break;
      }
      gst_message_unref(msg);
    }
  } while (msg != nullptr);

  // Clean up
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
}
