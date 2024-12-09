#include "test.hpp"
#include "test_gst.hpp"
#include "test_ocv.hpp"
#include "aicamerag2.hpp"

class MQTTClient : public mosqpp::mosquittopp {
public:
    MQTTClient(const char* id) : mosqpp::mosquittopp(id) {}

    void on_message(const struct mosquitto_message* message) override {
        std::string payload((char*)message->payload, message->payloadlen);
		xlog("MQTT payload:%s", payload.c_str());
        
        if (payload == "bg") {
            AICamera_getBrightness();
        } else if (payload == "bs0") {
            AICamera_setBrightness(0);
            AICamera_getBrightness();
        } else if (payload == "bs100") {
            AICamera_setBrightness(100);
            AICamera_getBrightness();

        } else if (payload == "cg") {
            AICamera_getContrast();
        } else if (payload == "cs0") {
            AICamera_setContrast(0);
            AICamera_getContrast();
        } else if (payload == "cs10") {
            AICamera_setContrast(10);
            AICamera_getContrast();

        } else if (payload == "wbag") {
            AICamera_getWhiteBalanceAutomatic();
        } else if (payload == "wbas0") {
            AICamera_setWhiteBalanceAutomatic(0);
        } else if (payload == "wbas1") {
            AICamera_setWhiteBalanceAutomatic(1);

        } else if (payload == "gst") {
            std::thread t(gst_test, 0);
            t.detach();
        } else if (payload == "gst2") {
            std::thread t(gst_test2, 0);
            t.detach();
        } else if (payload == "gst2x") {
            stopPipeline();
     
        } else if (payload == "ts") {
            startTimer(1000);
        } else if (payload == "tx") {
            stopTimer();
        
        } else if (payload == "ocv") {
            std::thread t(ocv_test, 0);
            t.detach();
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
