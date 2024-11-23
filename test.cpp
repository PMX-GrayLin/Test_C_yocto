#include "test.h"
#include "gst/gst.h"

using namespace std; 


int main(int argc, char *argv[])
{
	xlog("%s:%d, argc:%d \n\r", __func__, __LINE__, argc);

	GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the pipeline
    std::string pipeline_description = "videotestsrc ! videoconvert ! autovideosink";
    pipeline = gst_parse_launch(pipeline_description.c_str(), nullptr);
    if (!pipeline) {
        // std::cerr << "Failed to create pipeline" << std::endl;
        return -1;
    }

    // Start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // // Wait until error or EOS
    // bus = gst_element_get_bus(pipeline);
    // msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
    //                                  static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // // Parse message
    // if (msg) {
    //     GError *err;
    //     gchar *debug_info;

    //     switch (GST_MESSAGE_TYPE(msg)) {
    //     case GST_MESSAGE_ERROR:
    //         gst_message_parse_error(msg, &err, &debug_info);
    //         std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src)
    //                   << ": " << err->message << std::endl;
    //         std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
    //         g_clear_error(&err);
    //         g_free(debug_info);
    //         break;
    //     case GST_MESSAGE_EOS:
    //         std::cout << "End-Of-Stream reached." << std::endl;
    //         break;
    //     default:
    //         std::cerr << "Unexpected message received." << std::endl;
    //         break;
    //     }
    //     gst_message_unref(msg);
    // }

    // // Free resources
    // gst_object_unref(bus);
    // gst_element_set_state(pipeline, GST_STATE_NULL);
    // gst_object_unref(pipeline);

	return 0;
}
