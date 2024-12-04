#include "test.h"
#include "test_gst.h"
#include "test_ocv.h"
#include "aicamerag2.h"

class MQTTClient : public mosqpp::mosquittopp {
public:
    MQTTClient(const char* id) : mosqpp::mosquittopp(id) {}

    void on_message(const struct mosquitto_message* message) override {
        std::string payload((char*)message->payload, message->payloadlen);
		xlog("MQTT payload:%s", payload.c_str());
        
        if (payload == "1") {
            AICamera_getBrightness();
        } else if (payload == "2") {
            int brightness =0;
            AICamera_setBrightness(brightness);
            AICamera_getBrightness();
        } else if (payload == "3") {
            int brightness = 100;
            AICamera_setBrightness(brightness);
            AICamera_getBrightness();
        } else if (payload == "4") {
            AICamera_setWhiteBalanceAutomatic(0);
        } else if (payload == "5") {
            AICamera_setWhiteBalanceAutomatic(1);
        } else if (payload == "gst") {
            std::thread t1(gst_test, 0);
            t1.detach();
        }
    }
};

int main(int argc, char* argv[]) {
  
  xlog("");
  for (int i = 0; i < argc; ++i) {
    xlog("argv[%d]:%s", i, argv[i]);
  }

//   if (argc < 2) {
//     xlog("to input more than 1 params...");
//     return -1;
//   }

//   if (!strcmp(argv[1], "test")) {
//     xlog("");
//   } else if (!strcmp(argv[1], "gst")) {
//     int testCase = (argv[2] == nullptr) ? 0 : 1;
//     gst_test(testCase);
//   } else if (!strcmp(argv[1], "ocv")) {
//     int testCase = (argv[2] == nullptr) ? 0 : 1;
//     ocv_test(testCase);
//   } else if (!strcmp(argv[1], "dev")) {
//     xlog("getVideoDevice:%s", AICamrea_getVideoDevice().c_str());
//   }

  mosqpp::lib_init();
  MQTTClient client("my_client");
  client.connect("localhost", 1883);
  client.subscribe(nullptr, "my_topic");

  while (true) {
    client.loop();
  }

  mosqpp::lib_cleanup();

  return 0;
}
