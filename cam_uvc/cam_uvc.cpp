#include "cam_gige_hikrobot.hpp"

#include <gst/gst.h>

// apply only used header
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "image_utils.hpp"
#include "restfulx.hpp"
#include "device.hpp"
#include "cam_uvc.hpp"

std::thread t_streaming_uvc;
bool isStreaming_uvc = false;

bool isCapturePhoto_uvc = false;
bool isCropPhoto_uvc = false;
bool isPaddingPhoto_uvc = false;
SavedPhotoFormat savedPhotoFormat_uvc = spf_PNG;
std::string pathName_savedImage_uvc = "";
// std::string pathName_croppedImage_uvc = "";
// std::string pathName_inputImage_uvc = "";

cv::Rect crop_roi_uvc(0, 0, 0, 0);

static volatile int counterFrame_uvc = 0;

static GstElement *gst_pipeline_uvc = nullptr;
static GstElement *gst_flip_uvc = nullptr;
static GMainLoop *gst_loop_uvc = nullptr;

static std::string devicePath_uvc = "";

void UVC_handle_RESTful(std::vector<std::string> segments) {

  if (isSameString(segments[1], "start")) {
    UVC_streamingStart();

  } else if (isSameString(segments[1], "stop")) {
    UVC_streamingStop();

  } else if (isSameString(segments[1], "flip")) {
    // UVC_setFlip(segments[2]);

  } else if (isSameString(segments[1], "tp")) {
    xlog("take picture");
    std::string path = "";
    if (segments.size() > 2 && !segments[2].empty()) {
      //ex: curl http://localhost:8765/fw/gige/tp/%252Fhome%252Froot%252Fprimax%252F12345.png
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
    UVC_setImagePath(path.c_str());
    UVC_captureImage();
  }

}

void UVC_setDevicePath(const string &devicePath) {
    devicePath_uvc = devicePath;
}

void UVC_setImagePath(const string &imagePath) {
    pathName_savedImage_uvc = imagePath;
}

void UVC_captureImage() {
  if (!isStreaming_uvc) {
    xlog("do nothing...camera is not streaming");
    return;
  }

  xlog("");
  isCapturePhoto_uvc = true;
    
}

void UVC_saveImage(GstPad *pad, GstPadProbeInfo *info) {
  if (isCapturePhoto_uvc) {
    isCapturePhoto_uvc = false;
    imgu_saveImage_thread((void *)pad, (void *)info, pathName_savedImage_uvc, &syncSignal_save);
  }
}

// Callback to handle incoming buffer data
GstPadProbeReturn UVC_streamingDataCallback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  UVC_streamingLED();
  UVC_saveImage(pad, info);
  return GST_PAD_PROBE_OK;
}

void Thread_UVCStreaming() {
  xlog("++++ start ++++");
  counterFrame_uvc = 0;

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // final gst pipeline
  // gst-launch-1.0 v4l2src device="/dev/video137" ! image/jpeg,width=2048,height=1536,framerate=30/1 ! jpegdec ! videoconvert ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=dmabuf ! rtspclientsink location=rtsp://localhost:8554/mystream

  // Create the elements
  gst_pipeline_uvc = gst_pipeline_new("video-pipeline");
  GstElement *source = gst_element_factory_make("v4l2src", "source");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  GstElement *jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
  GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!gst_pipeline_uvc || !source || !capsfilter || !jpegdec || !videoconvert || !encoder || !sink) {
    xlog("failed to create GStreamer elements");
    return;
  }

  // Set properties for the elements
  g_object_set(G_OBJECT(source), "device", devicePath_uvc.c_str(), nullptr);
  xlog("devicePath_uvc:%s", devicePath_uvc.c_str());

  // Define the capabilities for the capsfilter
  GstCaps *caps = gst_caps_new_simple(
      "image/jpeg",
      "width", G_TYPE_INT, 1920,
      "height", G_TYPE_INT, 1080,
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
    gst_object_unref(gst_pipeline_uvc);
    return;
  }
  g_object_set(G_OBJECT(encoder), "extra-controls", controls, nullptr);
  // Free the GstStructure after use
  gst_structure_free(controls);

  g_object_set(encoder, "capture-io-mode", 4, nullptr);  // dmabuf = 4
  g_object_set(sink, "location", "rtsp://localhost:8554/mystream", nullptr);

  // // Build the pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline_uvc), source, capsfilter, jpegdec, videoconvert, encoder, sink, nullptr);
  if (!gst_element_link_many(source, capsfilter, jpegdec, videoconvert, encoder, sink, nullptr)) {
    xlog("failed to link elements in the pipeline");
    gst_object_unref(gst_pipeline_uvc);
    return;
  }

  // Attach pad probe to capture frames
  GstPad *pad = gst_element_get_static_pad(encoder, "sink");
  if (pad) {
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)UVC_streamingDataCallback, nullptr, nullptr);
    gst_object_unref(pad);
  }

  // Start the pipeline
  GstStateChangeReturn ret = gst_element_set_state(gst_pipeline_uvc, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    xlog("failed to start the pipeline");
    gst_object_unref(gst_pipeline_uvc);
    return;
  }

  xlog("pipeline is running...");
  isStreaming_uvc = true;

  // Run the main loop
  gst_loop_uvc = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(gst_loop_uvc);

  // Stop the pipeline when finished or interrupted
  xlog("Stopping the pipeline...");
  gst_element_set_state(gst_pipeline_uvc, GST_STATE_NULL);

  // Clean up
  gst_object_unref(gst_pipeline_uvc);
  isStreaming_uvc = false;
  xlog("++++ stop ++++, Pipeline stopped and resources cleaned up");
}

void UVC_streamingStart() {
  xlog("");
  if (isStreaming_uvc) {
    xlog("thread already running");
    return;
  }
  isStreaming_uvc = true;

  FW_setLED("2", "off");

  t_streaming_uvc = std::thread(Thread_UVCStreaming);  
  t_streaming_uvc.detach();
}

void UVC_streamingStop() {
  xlog("");
  if (!isStreaming_uvc) {
    xlog("Streaming not running");
    return;
  }

  if (gst_loop_uvc) {
    xlog("g_main_loop_quit");
    g_main_loop_quit(gst_loop_uvc);
  } else {
    xlog("gst_loop_uvc is invalid or already destroyed.");
  }

  isStreaming_uvc = false;
}

void UVC_streamingLED() {
  counterFrame_uvc++;
  if (counterFrame_uvc % 15 == 0) {
    FW_toggleLED("2", "orange");
  }
}
