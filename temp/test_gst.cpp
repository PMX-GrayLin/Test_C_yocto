#include "test_gst.hpp"

#include <atomic>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

static volatile int counterFrame = 0;

static GstElement *gst_pipeline = nullptr;
static GMainLoop *gst_loop = nullptr;
static std::thread gst_thread;
static std::atomic<bool> gst_running{false};

void gst_thread_pipeline(int testCase) {
  xlog("++++ start ++++, testCase:%d", testCase);

  GstElement *gst_pipeline;
  GstBus *bus;
  GstMessage *msg;
  string pipelineS = "";

  // Initialize GStreamer
  // gst_init(&argc, &argv);
  gst_init(nullptr, nullptr);

  guint major, minor, micro, nano;
  gst_version(&major, &minor, &micro, &nano);
  xlog("%d.%d.%d.%d", major, minor, micro, nano);

  if (testCase == 0) {
    // OK
    // gst-launch-1.0 -v v4l2src device=/dev/video47 ! video/x-raw,width=1920,height=1080 ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream
    pipelineS =
        std::string("v4l2src device=/dev/csi_cam_preview ") +
        "! video/x-raw,width=2048,height=1536 " +
        "! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=dmabuf "
        "! rtspclientsink location=rtsp://localhost:8554/mystream";
  } else if (testCase == 1) {
    // OK
    // gst-launch-1.0 aravissrc camera-name="id1" ! videoconvert ! video/x-raw,format=NV12 ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream
    pipelineS = 
    R"(
        aravissrc camera-name=id1
        ! videoconvert
        ! video/x-raw,format=NV12
        ! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=dmabuf
        ! rtspclientsink location=rtsp://localhost:8554/mystream
    )";
  } else {
    xlog("not a testCase");
    return;
  }

  xlog("pipeline:%s", pipelineS.c_str());

  gst_pipeline = gst_parse_launch(pipelineS.c_str(), nullptr);
  if (!gst_pipeline) {
    xlog("gst_parse_launch fail");
    return;
  }

  // Start playing
  gst_element_set_state(gst_pipeline, GST_STATE_PLAYING);

  xlog("pipeline is running...");

  // Create and run main loop
  gst_running = true;
  gst_loop = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(gst_loop);

  // Cleanup after main loop exits
  gst_element_set_state(gst_pipeline, GST_STATE_NULL);
  gst_object_unref(gst_pipeline);
  g_main_loop_unref(gst_loop);
  gst_loop = nullptr;
  gst_running = false;

  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");
  return;
}

void test_gst_pipeline_start(int testCase) {
    if (gst_running) {
        xlog("Pipeline already running.");
        return;
    }
    gst_thread = std::thread(gst_thread_pipeline, testCase);
}

void test_gst_pipeline_stop() {
    if (gst_running && gst_loop) {
        xlog("Stopping GStreamer pipeline...");
        g_main_loop_quit(gst_loop);
        if (gst_thread.joinable())
            gst_thread.join();
    } else {
        xlog("Pipeline not running.");
    }
}

// Callback to handle incoming buffer data
GstPadProbeReturn cb_have_data(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
  if (buffer) {
    counterFrame++;
    if (counterFrame % 300 == 0) {
      xlog("frame captured, counterFrame:%d", counterFrame);
    }
  }
  return GST_PAD_PROBE_OK;
}

void gst_thread_src(int testCase) {
  xlog("++++ start ++++, testCase:%d", testCase);
  counterFrame = 0;

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
  g_object_set(G_OBJECT(source), "device", "/dev/csi_cam_preview", nullptr);

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
  gst_running = true;

  // Run the main loop
  gst_loop = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(gst_loop);

  // Stop the pipeline when finished or interrupted
  xlog("Stopping the pipeline...");

  // Cleanup after main loop exits
  gst_element_set_state(gst_pipeline, GST_STATE_NULL);
  gst_object_unref(gst_pipeline);
  g_main_loop_unref(gst_loop);
  gst_loop = nullptr;
  gst_running = false;
  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");
}

void test_gst_src_start(int testCase) {
    if (gst_running) {
        xlog("Pipeline already running.");
        return;
    }
    gst_thread = std::thread(gst_thread_src, testCase);
}

void test_gst_src_stop() {
    if (gst_running && gst_loop) {
        xlog("Stopping GStreamer pipeline...");
        g_main_loop_quit(gst_loop);
        if (gst_thread.joinable())
            gst_thread.join();
    } else {
        xlog("Pipeline not running.");
    }
}

static GstFlowReturn on_new_sample(GstAppSink *appsink, gpointer user_data) {
  GstSample *sample = gst_app_sink_pull_sample(appsink);
  if (sample) {
    counterFrame++;
    if (counterFrame % 300 == 0) {
      xlog("sample captured, counterFrame:%d", counterFrame);
    }
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }
  return GST_FLOW_ERROR;
}

void gst_thread_appsink(int testCase) {
  xlog("++++ start ++++, testCase:%d", testCase);
  counterFrame = 0;

  gst_init(nullptr, nullptr);

  gst_pipeline = gst_parse_launch(
      "videotestsrc ! videoconvert ! video/x-raw,format=RGB ! appsink name=mysink", nullptr);

  GstElement *appsink = gst_bin_get_by_name(GST_BIN(gst_pipeline), "mysink");
  gst_app_sink_set_emit_signals(GST_APP_SINK(appsink), true);
  gst_app_sink_set_drop(GST_APP_SINK(appsink), true);
  gst_app_sink_set_max_buffers(GST_APP_SINK(appsink), 1);

  g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), nullptr);

  gst_element_set_state(gst_pipeline, GST_STATE_PLAYING);

  gst_loop = g_main_loop_new(nullptr, FALSE);
  gst_running = true;
  xlog("Running main loop.");
  g_main_loop_run(gst_loop);
  xlog("Main loop exited.");

  gst_element_set_state(gst_pipeline, GST_STATE_NULL);
  gst_object_unref(gst_pipeline);
  gst_object_unref(appsink);
  g_main_loop_unref(gst_loop);
  gst_loop = nullptr;
  gst_running = false;

  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");
}

void test_gst_appsink_start(int testCase) {
    if (gst_running) {
        xlog("Pipeline already running.");
        return;
    }
    gst_thread = std::thread(gst_thread_appsink, testCase);
}

void test_gst_appsink_stop() {
    if (gst_running && gst_loop) {
        xlog("Stopping GStreamer pipeline...");
        g_main_loop_quit(gst_loop);
        if (gst_thread.joinable())
            gst_thread.join();
    } else {
        xlog("Pipeline not running.");
    }
}

void test_gst_stopPipeline() {
  if (gst_loop) {
    g_main_loop_quit(gst_loop);
    g_main_loop_unref(gst_loop);
    gst_loop = nullptr;
  } else {
    xlog("Main loop is invalid or already destroyed.");
  }
}
