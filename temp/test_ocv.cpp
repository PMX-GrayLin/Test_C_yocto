#include "test_ocv.hpp"

// apply only used header
// #include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include "cam_omnivision.hpp"

void test_ocv(int testCase) {
  xlog("testCase:%d", testCase);

  // OK
  // std::string pipeline = "videotestsrc ! videoconvert ! appsink";

    // NG
  // string pipelineS =
  //     "v4l2src device=" + AICP_getVideoDevice() + " " +
  //     "! video/x-raw, width=640, height=480, framerate=30/1  " +
  //     "! videoconvert "
  //     "! v4l2h264enc "
  //     "! appsink";

  // NG
  // string pipelineS =
  //     "v4l2src device=" + AICP_getVideoDevice() + " " +
  //     "! video/x-raw, width=640, height=480, framerate=30/1  " +
  //     "! videoconvert "
  //     "! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=mmap "
  //     "! appsink";

  // NG
  // string pipelineS =
  //     "v4l2src device=" + AICP_getVideoDevice() + " " +
  //     "! video/x-raw,width=640,height=480 " +
  //     "! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=mmap "
  //     "! appsink";

  // OK
  // string pipelineS = "videotestsrc ! videoconvert ! appsink";

  // OK
  // string pipelineS =
  //     "v4l2src device=" + AICP_getVideoDevice() + " " +
  //     "! video/x-raw, width=640, height=480, framerate=30/1  " +
  //     "! videoconvert "
  //     "! appsink";

  // NG
  string pipelineS =
      "v4l2src device=" + AICP_getVideoDevice() + " " +
      "! video/x-raw, width=640, height=480 " +
      "! videoconvert "
      "! appsink";

  xlog("pipeline:%s", pipelineS.c_str());

  // Define the GStreamer pipeline
  // Open the pipeline with OpenCV
  cv::VideoCapture cap(pipelineS, cv::CAP_GSTREAMER);

  if (!cap.isOpened()) {
    xlog("cap.isOpened false");
    return;
  }

  // Declare the frame variable (cv::Mat) to hold the captured frame
  cv::Mat frame;
  int frameCount = 0;
  int imgCount = 0;
  while (true) {
    // Capture a frame
    if (!cap.read(frame)) {
      xlog("fail to capture frame");
      break;
    }
    frameCount++;

    if (frame.empty()) {
      xlog("frame is empty");
      break;
    }

    xlog("frameCount:%d", frameCount);

    if (frameCount % 200 == 0) {
      // Save the frame to a picture
      std::string filename = "frame_" + std::to_string(imgCount++) + ".png";
      if (cv::imwrite(filename, frame)) {
        xlog("Saved frame to %s", filename.c_str());
      } else {
        xlog("Failed to save frame");
      }
    }

    // Break on 'q' key press
    if (cv::waitKey(10) == 'q') {
      break;
    }
  }

  // Release resources
  cap.release();
  return;
}
