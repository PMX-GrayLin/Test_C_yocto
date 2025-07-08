#include "cam_gige_hikrobot.hpp"

#include <atomic>
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
std::atomic<bool> isStreaming_uvc{false};

bool isCapturePhoto_uvc = false;
bool isCropPhoto_uvc = false;
bool isPaddingPhoto_uvc = false;
SavedPhotoFormat savedPhotoFormat_uvc = spf_PNG;
std::string pathName_savedImage_uvc = "";
// std::string pathName_croppedImage_uvc = "";
// std::string pathName_inputImage_uvc = "";
// cv::Rect crop_roi_uvc(0, 0, 0, 0);

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
  if (!isStreaming_uvc.load()) {
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
  gst_init(nullptr, nullptr);

  GstCaps *caps = nullptr;
  gst_pipeline_uvc = gst_pipeline_new("video-pipeline");
  GstElement *source = gst_element_factory_make("v4l2src", "source");
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
  GstElement *jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
  GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
  GstElement *sink = gst_element_factory_make("rtspclientsink", "sink");

  if (!gst_pipeline_uvc || !source || !capsfilter || !jpegdec || !videoconvert || !encoder || !sink) {
    xlog("Failed to create GStreamer elements");
  } else {
    g_object_set(G_OBJECT(source), "device", devicePath_uvc.c_str(), nullptr);
    xlog("devicePath_uvc: %s", devicePath_uvc.c_str());

    caps = gst_caps_new_simple(
        "image/jpeg",
        "width", G_TYPE_INT, 1920,
        "height", G_TYPE_INT, 1080,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        nullptr);

    if (!caps) {
      xlog("Failed to create caps");
    } else {
      g_object_set(capsfilter, "caps", caps, nullptr);

      GstStructure *controls = gst_structure_new("extra-controls", "video_gop_size", G_TYPE_INT, 30, nullptr);
      if (!controls) {
        xlog("Failed to create controls");
      } else {
        g_object_set(encoder, "extra-controls", controls, nullptr);
        g_object_set(encoder, "capture-io-mode", 4, nullptr);
        g_object_set(sink, "location", "rtsp://localhost:8554/mystream", nullptr);
        gst_structure_free(controls);

        gst_bin_add_many(GST_BIN(gst_pipeline_uvc), source, capsfilter, jpegdec, videoconvert, encoder, sink, nullptr);
        if (!gst_element_link_many(source, capsfilter, jpegdec, videoconvert, encoder, sink, nullptr)) {
          xlog("Failed to link elements");
        } else {
          GstPad *pad = gst_element_get_static_pad(encoder, "sink");
          if (pad) {
            gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)UVC_streamingDataCallback, nullptr, nullptr);
            gst_object_unref(pad);
          }

          GstStateChangeReturn ret = gst_element_set_state(gst_pipeline_uvc, GST_STATE_PLAYING);
          if (ret == GST_STATE_CHANGE_FAILURE) {
            xlog("Failed to start pipeline");
          } else {
            xlog("Pipeline is running...");
            isStreaming_uvc = true;

            gst_loop_uvc = g_main_loop_new(nullptr, FALSE);
            g_main_loop_run(gst_loop_uvc);

            xlog("Stopping pipeline...");
            if (devicePath_uvc != "") {
              FW_setLED("2", "green");
            }
            
            gst_element_set_state(gst_pipeline_uvc, GST_STATE_NULL);
          }
        }
      }
    }
  }

  if (caps) {
    gst_caps_unref(caps);
  }

  if (gst_pipeline_uvc) {
    gst_object_unref(gst_pipeline_uvc);
    gst_pipeline_uvc = nullptr;
  }

  isStreaming_uvc = false;
  xlog("++++ stop ++++, pipeline stopped and cleaned");
}

void UVC_streamingStart() {
  xlog("");
  if (isStreaming_uvc.load()) {
    xlog("Thread already running");
    return;
  }

  FW_setLED("2", "off");
  t_streaming_uvc = std::thread(Thread_UVCStreaming);
  t_streaming_uvc.detach();
}

void UVC_streamingStop() {
  xlog("");
  if (!isStreaming_uvc.load()) {
    xlog("Streaming not running");
    return;
  }

  if (gst_loop_uvc) {
    xlog("g_main_loop_quit");
    g_main_loop_quit(gst_loop_uvc);
  } else {
    xlog("gst_loop_uvc is invalid or already destroyed.");
  }
}

void UVC_streamingLED() {
  counterFrame_uvc++;
  if (counterFrame_uvc % 15 == 0) {
    FW_toggleLED("2", "orange");
  }
}
