#include "test_gst.h"

#include "test.h"
#include "aicamerag2.h"

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

  // Create the pipeline
  // std::string pipeline = "v4l2src device=/dev/video45 ! video/x-raw,width=640,height=480 ! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=mmap ! rtspclientsink location=rtsp://localhost:8554/mystream";

  string pipelineS =
      "v4l2src device=" + AICamrea_getVideoDevice() + " " +
      "! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=mmap "
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
        // std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src)
        //           << ": " << err->message << std::endl;
        // std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
        g_clear_error(&err);
        g_free(debug_info);
        break;
      case GST_MESSAGE_EOS:
        // std::cout << "End-Of-Stream reached." << std::endl;
        break;
      default:
        // std::cerr << "Unexpected message received." << std::endl;
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
