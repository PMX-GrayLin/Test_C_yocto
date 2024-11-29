#include "test.h"
#include "test_gst.h"

#include <opencv2/opencv.hpp>

using namespace std; 

void opencv_test() {
	// Define the GStreamer pipeline
    // std::string pipeline = "videotestsrc ! videoconvert ! appsink";
    // v4l2src device=/dev/video{CAM_ID} ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert ! appsink
    // v4l2src device=${VIDEO_DEV[0]} ! video/x-raw,width=640,height=480 ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=mmap !

    // std::string pipeline = "v4l2src device=/dev/video60 ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert ! appsink";
    // std::string pipeline = "v4l2src device=/dev/video43 ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert  ! tee name=t
    //                         t. ! queue ! rtspclientsink location=rtsp://localhost:8554/mystream
    //                         t. ! queue ! appsink";

                            // ! videoconvert ! tee name=t 
                            // ! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=mmap

    // std::string pipeline = "v4l2src device=/dev/video52 "
    //                         "! video/x-raw, width=640, height=480, framerate=30/1 "
    //                         "! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=mmap "
    //                         "! tee name=t "
    //                         "t. ! queue ! rtspclientsink location=rtsp://localhost:8554/mystream "
    //                         "t. ! queue ! appsink emit-signals=true sync=false";


    // OK
// gst-launch-1.0 \
//   videotestsrc \
//   ! v4l2h264enc extra-controls="cid,video_gop_size=30" capture-io-mode=mmap \
//   ! rtspclientsink location=rtsp://localhost:8554/mystream

    std::string pipeline = "videotestsrc "
                            "! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=mmap "
                            "! appsink";
    xlog("pipeline:%s", pipeline.c_str());

    // Open the pipeline with OpenCV
    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);

    if (!cap.isOpened()) {
        return;
    }

	xlog("");

	// Declare the frame variable (cv::Mat) to hold the captured frame
    cv::Mat frame;

	while (true) {
        // Capture a frame
        cap >> frame;

        if (frame.empty()) {
			xlog("");
            // std::cerr << "Empty frame received. Exiting..." << std::endl;
            break;
        }

        // Display the frame
        cv::imshow("GStreamer Video", frame);
        xlog("");

        // Break on 'q' key press
        if (cv::waitKey(10) == 'q') {
            break;
        }
    }

	xlog("");

	// Release resources
    cap.release();
    cv::destroyAllWindows();

	xlog("");
	
	return;
}
int main(int argc, char *argv[]) {
  xlog("");

  for (int i = 0; i < argc; ++i) {
    xlog("argv[%d]:%s", i, argv[i]);
  }

  if (argc < 2) {
    xlog("to input more than 1 params...");
    return -1;
  }

  if (!strcmp(argv[1], "test")) {
    xlog("");
  } else if (!strcmp(argv[1], "gst")) {
	int testCase = (argv[2] == nullptr) ? 0 : 1;
    gst_test(testCase);
  } else if (!strcmp(argv[1], "ocv")) {
    opencv_test();
  }

  return 0;
}
