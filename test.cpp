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
        } else if (payload == "bn1") {
            AICamera_setBrightness(0);
        } else if (payload == "bn2") {
            AICamera_setBrightness(25);
        } else if (payload == "bn3") {
            AICamera_setBrightness(50);
        } else if (payload == "bn4") {
            AICamera_setBrightness(75);
        } else if (payload == "bn5") {
            AICamera_setBrightness(100);

        } else if (payload == "cg") {
            AICamera_getContrast();
        } else if (payload == "ct1") {
            AICamera_setContrast(0);
        } else if (payload == "ct2") {
            AICamera_setContrast(2);
        } else if (payload == "ct3") {
            AICamera_setContrast(4);
        } else if (payload == "ct4") {
            AICamera_setContrast(6);
        } else if (payload == "ct5") {
            AICamera_setContrast(8);
        } else if (payload == "ct6") {
            AICamera_setContrast(10);

        } else if (payload == "sg") {
            AICamera_getSaturation();
        } else if (payload == "sr1") {
            AICamera_setSaturation(0);
        } else if (payload == "sr2") {
            AICamera_setSaturation(2);
        } else if (payload == "sr3") {
            AICamera_setSaturation(4);
        } else if (payload == "sr4") {
            AICamera_setSaturation(6);
        } else if (payload == "sr5") {
            AICamera_setSaturation(8);
        } else if (payload == "sr6") {
            AICamera_setSaturation(10);

        } else if (payload == "hg") {
            AICamera_getHue();
        } else if (payload == "hu1") {
            AICamera_setHue(0);
        } else if (payload == "hu2") {
            AICamera_setHue(25);
        } else if (payload == "hu3") {
            AICamera_setHue(50);
        } else if (payload == "hu4") {
            AICamera_setHue(75);
        } else if (payload == "hu5") {
            AICamera_setHue(100);

        } else if (payload == "wbag") {
            AICamera_getWhiteBalanceAutomatic();
        } else if (payload == "wba0") {
            AICamera_setWhiteBalanceAutomatic(0);
        } else if (payload == "wba1") {
            AICamera_setWhiteBalanceAutomatic(1);

        } else if (payload == "wbtg") {
          AICamera_getWhiteBalanceTemperature();
        } else if (payload == "wbt1") {
          AICamera_setWhiteBalanceTemperature(2700);
        } else if (payload == "wbt2") {
          AICamera_setWhiteBalanceTemperature(4600);
        } else if (payload == "wbt3") {
          AICamera_setWhiteBalanceTemperature(6500);

        } else if (payload == "spg") {
          AICamera_getSharpness();
        } else if (payload == "sp1") {
          AICamera_setSharpness(0);
        } else if (payload == "sp2") {
          AICamera_setSharpness(2);
        } else if (payload == "sp3") {
          AICamera_setSharpness(4);
        } else if (payload == "sp4") {
          AICamera_setSharpness(6);
        } else if (payload == "sp5") {
          AICamera_setSharpness(6);
        } else if (payload == "sp6") {
          AICamera_setSharpness(10);

        } else if (payload == "eg") {
          AICamera_getExposure();
        } else if (payload == "ep0") {
          AICamera_setExposure(0);
        } else if (payload == "ep-40") {
          AICamera_setExposure(-40);
        } else if (payload == "ep40") {
          AICamera_setExposure(40);

        } else if (payload == "eag") {
          AICamera_getExposureAuto();
        } else if (payload == "epa0") {
          AICamera_setExposureAuto(0);
        } else if (payload == "epa1") {
          AICamera_setExposureAuto(1);

        } else if (payload == "etag") {
          AICamera_getExposureTimeAbsolute();
        } else if (payload == "ept1") {
          AICamera_setExposureTimeAbsolute(0.01);
        } else if (payload == "ept2") {
          AICamera_setExposureTimeAbsolute(3.3);
        } else if (payload == "ept3") {
          AICamera_setExposureTimeAbsolute(6.6);
        } else if (payload == "ept4") {
          AICamera_setExposureTimeAbsolute(10);

          // } else if (payload == "fag") {
          //     AICamera_getFocusAuto();
          // } else if (payload == "fas0") {
          //     AICamera_setFocusAuto(0);
          // } else if (payload == "fas1") {
          //     AICamera_setFocusAuto(1);

          // } else if (payload == "fabsg") {
          //     AICamera_getFocusAbsolute();
          // } else if (payload == "fabss0") {
          //     AICamera_setFocusAbsolute(0);
          // } else if (payload == "fabss255") {
          //     AICamera_setFocusAbsolute(255);

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
