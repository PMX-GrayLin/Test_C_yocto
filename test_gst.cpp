#include "test_gst.h"

#include "test.h"
#include "aicamerag2.h"
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

  xlog("");

  guint major, minor, micro, nano;
  gst_version(&major, &minor, &micro, &nano);
  xlog("%d.%d.%d.%d", major, minor, micro, nano);

  // OK
  // gst-launch-1.0 -v v4l2src device=/dev/video47 ! video/x-raw,width=1920,height=1080 ! v4l2h264enc extra-controls="cid,video_gop_size=60" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream

  // Create the pipeline
  string pipelineS =
      "v4l2src device=" + AICamrea_getVideoDevice() + " " +
      "! video/x-raw,width=1920,height=1080 " +
      "! v4l2h264enc extra-controls=\"cid,video_gop_size=60\" capture-io-mode=dmabuf "
      "! rtspclientsink location=rtsp://localhost:8554/mystream";

  xlog("pipeline:%s", pipelineS.c_str());

  pipeline = gst_parse_launch(pipelineS.c_str(), nullptr);
  if (!pipeline) {
    xlog("gst_parse_launch fail");
    return;
  }

  xlog("");

  // Start playing
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  xlog("");

  // Wait until error or EOS
  bus = gst_element_get_bus(pipeline);
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                   static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
  xlog("");

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

  xlog("");

  // Free resources
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  xlog("");

  return;
}

// Callback to handle incoming buffer data
GstPadProbeReturn cb_have_data(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
  if (buffer) {
    counterFrame++;
    buffer = gst_buffer_ref(buffer);

    // Map the buffer to access its data
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
      xlog("frame captured, counterFrame:%d, Size:%ld bytes", counterFrame, map.size);

      // Define frame properties (update width, height, and type as per your caps)
      int width = 1920;    // Set your video width
      int height = 1080;   // Set your video height
      int channels = 3;    // Assuming BGR format
      int type = CV_8UC3;  // OpenCV type for BGR format

      // Create an OpenCV Mat from the raw buffer data
      cv::Mat frame(height, width, type, map.data);

      if (counterFrame % 200 == 0) {
        // Save the frame to a picture
        std::string filename = "frame_" + std::to_string(counterImg++) + ".png";
        if (cv::imwrite(filename, frame)) {
          xlog("Saved frame to %s", filename.c_str());
        } else {
          xlog("Failed to save frame");
        }
      }

      gst_buffer_unmap(buffer, &map);
    } else {
      std::cerr << "Failed to map buffer" << std::endl;
    }

    gst_buffer_unref(buffer);
  }
  return GST_PAD_PROBE_OK;
}

void gst_test2(int testCase) {
  xlog("testCase:%d", testCase);
  counterFrame = 0;
  counterImg = 0;

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // gst-launch-1.0 -v v4l2src device=/dev/video47 ! video/x-raw,width=1920,height=1080 ! v4l2h264enc extra-controls="cid,video_gop_size=60" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream

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
      "width", G_TYPE_INT, 1920,
      "height", G_TYPE_INT, 1080,
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
  GstPad *pad = gst_element_get_static_pad(encoder, "src");
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
