#include "test.h"

#include <gst/gst.h>
#include <opencv2/opencv.hpp>

#include <string>
using namespace std; 

#define TEST_GST
#define TEST_OCV

void gst_test() {

	GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    // Initialize GStreamer
    // gst_init(&argc, &argv);
    gst_init(nullptr, nullptr);

	xlog("");

	guint major, minor, micro, nano;
    gst_version (&major, &minor, &micro, &nano);
	xlog("%d.%d.%d.%d", major, minor, micro, nano);

    // Create the pipeline
    // std::string pipeline_description = "videotestsrc ! videoconvert ! autovideosink";
    std::string pipeline_description = "v4l2src device=/dev/video45 ! video/x-raw,width=640,height=480 ! v4l2h264enc extra-controls=\"cid,video_gop_size=30\" capture-io-mode=mmap ! rtspclientsink location=rtsp://localhost:8554/mystream";
    
    pipeline = gst_parse_launch(pipeline_description.c_str(), nullptr);
    if (!pipeline) {
        // std::cerr << "Failed to create pipeline" << std::endl;
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

void opencv_test() {
	// Define the GStreamer pipeline
    // std::string pipeline = "videotestsrc ! videoconvert ! appsink";
    // v4l2src device=/dev/video{CAM_ID} ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert ! appsink

    // std::string pipeline = "v4l2src device=/dev/video60 ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert ! appsink";
    std::string pipeline = "v4l2src device=/dev/video60 ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert  ! tee name=t \
                            t. ! queue ! appsink \
                            t. ! queue ! rtspclientsink location=rtsp://localhost:8554/mystream"
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
    gst_test();
  } else if (!strcmp(argv[1], "ocv")) {
    opencv_test();
  }

  return 0;
}
