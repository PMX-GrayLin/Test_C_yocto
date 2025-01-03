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
        } else if (payload == "bs100") {
            AICamera_setBrightness(100);

        } else if (payload == "cg") {
            AICamera_getContrast();
        } else if (payload == "cs0") {
            AICamera_setContrast(0);
        } else if (payload == "cs10") {
            AICamera_setContrast(10);

        } else if (payload == "sg") {
            AICamera_getSaturation();
        } else if (payload == "ss0") {
            AICamera_setSaturation(0);
        } else if (payload == "ss10") {
            AICamera_setSaturation(10);

        } else if (payload == "hg") {
            AICamera_getHue();
        } else if (payload == "hs0") {
            AICamera_setHue(0);
        } else if (payload == "hs100") {
            AICamera_setHue(100);

        } else if (payload == "wbag") {
            AICamera_getWhiteBalanceAutomatic();
        } else if (payload == "wbas0") {
            AICamera_setWhiteBalanceAutomatic(0);
        } else if (payload == "wbas1") {
            AICamera_setWhiteBalanceAutomatic(1);

        } else if (payload == "eg") {
            AICamera_getExposure();
        } else if (payload == "es0") {
            AICamera_setExposure(0);
        } else if (payload == "es-40") {
            AICamera_setExposure(-40);
        } else if (payload == "es40") {
            AICamera_setExposure(40);

        } else if (payload == "eag") {
            AICamera_getExposureAuto();
        } else if (payload == "eas0") {
            AICamera_setExposureAuto(0);
        } else if (payload == "eas1") {
            AICamera_setExposureAuto(1);

        } else if (payload == "fag") {
            AICamera_getFocusAuto();
        } else if (payload == "fas0") {
            AICamera_setFocusAuto(0);
        } else if (payload == "fas1") {
            AICamera_setFocusAuto(1);

        } else if (payload == "fabsg") {
            AICamera_getFocusAbsolute();
        } else if (payload == "fabss0") {
            AICamera_setFocusAbsolute(0);
        } else if (payload == "fabss255") {
            AICamera_setFocusAbsolute(255);

        } else if (payload == "wbtg") {
            AICamera_getWhiteBalanceTemperature();
        } else if (payload == "wbts2700") {
            AICamera_setWhiteBalanceTemperature(2700);
        } else if (payload == "wbts6500") {
            AICamera_setWhiteBalanceTemperature(6500);

        } else if (payload == "gsttest") {
            std::thread t(gst_test, 0);
            t.detach();
        } else if (payload == "gst") {
            AICamera_startStreaming();
        } else if (payload == "gstx") {
            AICamera_stopStreaming();
     
        } else if (payload == "ts") {
            startTimer(60000);
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
//   client.subscribe(nullptr, "my_topic");
  client.subscribe(nullptr, "PX/VBS/Cmd");

  while (true) {
    client.loop();
  }
  mosqpp::lib_cleanup();

  return 0;
}
