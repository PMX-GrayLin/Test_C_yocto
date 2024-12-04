#include "test_ocv.h"

#include "test.h"
#include "aicamerag2.h"

void ocv_test(int testCase) {
  xlog("testCase:%d", testCase);

  // OK
  // std::string pipeline = "videotestsrc ! videoconvert ! appsink";

  // Define the GStreamer pipeline
  
  string pipelineS =
    "videotestsrc " +
    "! videoconvert " +
    "! appsink";

  // NG
  // string pipelineS =
  //     "v4l2src device=" + AICamrea_getVideoDevice() + " " +
  //     "! video/x-raw,width=640,height=480 " +
  //     "! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=mmap "
  //     "! appsink";


  xlog("pipeline:%s", pipelineS.c_str());

  // Open the pipeline with OpenCV
  cv::VideoCapture cap(pipelineS, cv::CAP_GSTREAMER);

  if (!cap.isOpened()) {
    xlog("cap.isOpened false");
    return;
  }

  // Declare the frame variable (cv::Mat) to hold the captured frame
  cv::Mat frame;

  while (true) {
    
    // Capture a frame
    if (!cap.read(frame)) {
      xlog("fail to capture frame");
      break;
    }

    if (frame.empty()) {
      xlog("frame is empty");
      break;
    }

    // Display the frame
    // cv::imshow("GStreamer Video", frame);
    xlog("");

    // Break on 'q' key press
    if (cv::waitKey(10) == 'q') {
      break;
    }
  }

  xlog("");

  // Release resources
  cap.release();
  // cv::destroyAllWindows();

  xlog("");

  return;
}
