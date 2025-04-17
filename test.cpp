#include "test.hpp"

#include "test_gst.hpp"
#include "test_ocv.hpp"
#include "aicamerag2.hpp"
#include "oti322.hpp"
#include "otpa8.hpp"
#include "httplib.h"

// MQTTClient gClient;
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

        } else if (payload == "isog") {
          AICamera_getISO();
        } else if (payload == "iso1") {
          AICamera_setISO(100);
        } else if (payload == "iso2") {
          AICamera_setISO(200);
        } else if (payload == "iso3") {
          AICamera_setISO(400);
        } else if (payload == "iso4") {
          AICamera_setISO(1600);
        } else if (payload == "iso5") {
          AICamera_setISO(6400);

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

        } else if (payload == "gsttest") {
          std::thread t(gst_test, 0);
          t.detach();
        } else if (payload == "gst") {
          AICamera_streamingStart();
        } else if (payload == "gstx") {
          AICamera_streamingStop();

        } else if (payload == "tms") {
          xlog("timer start...");
          startTimer(1000);
        } else if (payload == "tmx") {
          xlog("timer stop...");
          stopTimer();

        } else if (payload == "ocv") {
          std::thread t(ocv_test, 0);
          t.detach();
        
        } else if (payload == "t1") {
          OTI322 oti322;
          float ambientTemp = 0.0;
          float objectTemp = 0.0;
          oti322.readTemperature(ambientTemp, objectTemp);

          string s = "{ \"temperature\" : " + std::to_string(objectTemp) + " }";
          publish(nullptr, "PX/VBS/Cmd", s.length(), s.c_str() , 1, false);

        } else if (payload == "t2") {
          OTPA8 otpa8;
          float ambientTemp = 0.0;
          float objectTemp = 0.0;
          otpa8.startReading();
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

  bool isUseMQTT = false;
  if (!isUseMQTT) {
    xlog("USE RESTful...");
    
    OTI322 oti322;
    OTPA8 otpa8;

    // REST API: Get Temperature
    httplib::Server svr;
    // svr.Get("/temperature", [&](const httplib::Request& req, httplib::Response& res) {

    //   xlog("query oti322...");
    //   float ambientTemp = 0.0;
    //   float objectTemp = 0.0;
    //   oti322.readTemperature(ambientTemp, objectTemp);
    //   std::string response = "{ \"ambient\": " + std::to_string(ambientTemp) +
    //                          ", \"object\": " + std::to_string(objectTemp) + " }";
    //   res.set_content(response, "application/json");
    // });
    svr.Get("/temperatures", [&](const httplib::Request& req, httplib::Response& res) {
      
      xlog("query otpa8...");
      float ambientTemp = 0.0;
      float objectTemp = 0.0;
      otpa8.readTemperature_max(ambientTemp, objectTemp);
      std::string response = "{ \"ambient\": " + std::to_string(ambientTemp) +
                             ", \"object\": " + std::to_string(objectTemp) + " }";
      res.set_content(response, "application/json");

    });
    svr.Get("/temperature_array", [&](const httplib::Request& req, httplib::Response& res) {
      
      xlog("query otpa16...");
      float ambientTemp = 0.0;
      float objectTemp[256] = { 0.0 };
      otpa8.readTemperature_array(ambientTemp, objectTemp);
      std::ostringstream response;
      response << "{ \"ambient\": " << std::fixed << std::setprecision(1) << ambientTemp << ", \"object\": [";
      for (int i = 0; i < 256; ++i) {
          response << std::fixed << std::setprecision(1) << objectTemp[i];
          if (i < 255) response << ", ";  // Don't add comma after the last element
      }
      response << "] }";
      res.set_content(response.str(), "application/json");
    });
    svr.Get(R"(/fw/(.*))", [&](const httplib::Request &req, httplib::Response &res) {
      std::smatch match;
      std::regex regex(R"(/fw/(.*))");
  
      xlog("req.path:%s", req.path.c_str());
      if (std::regex_match(req.path, match, regex)) {
        // Create a dynamic array (vector) to store path segments
        std::vector<std::string> segments;
  
        // Get the matched string (e.g., "1/2/3")
        std::string matched_str = match[1].str();
        xlog("Matched string: %s", matched_str.c_str());
  
        // Split the matched string by '/' and store each segment in the vector
        std::stringstream ss(matched_str);
        std::string segment;
        while (std::getline(ss, segment, '/')) {
          segments.push_back(segment);
        }
        for (const auto& seg : segments) {
          xlog("Stored segment: %s", seg.c_str());
        }
        for (size_t i = 0; i < segments.size(); ++i) {
          xlog("Stored segment at index %zu: %s", i, segments[i].c_str());
        }
  
        if (isSameString(segments[0].c_str(), "gst")) {
          if (isSameString(segments[1].c_str(), "start")) {
            AICamera_streamingStart();
          } else if (isSameString(segments[1].c_str(), "stop")) {
            AICamera_streamingStop();
          }

        } else if (isSameString(segments[0].c_str(), "tp")) {
          xlog("take picture");
          std::string path = "";
          if (segments.size() > 1 && !segments[1].empty()) {
            path = "/home/root/primax/" + segments[1];
          } else {
            path = "/home/root/primax/fw_" + getTimeString() + ".png";
          }
          AICamera_setImagePath(path.c_str());
          AICamera_captureImage();
        
        } else if (isSameString(segments[0].c_str(), "led")) {
          if (segments.size() == 3) {
            AICamera_setLED(segments[1], segments[2]);
          } else {
            xlog("param may missing...");
          }

        } else if (isSameString(segments[0].c_str(), "do")) {
          AICamera_setDO(segments[1], segments[2]);
        
        } else if (isSameString(segments[0].c_str(), "di")) {
          if (isSameString(segments[1].c_str(), "on")) {
            AICamera_MonitorDIStart();
          } else if (isSameString(segments[1].c_str(), "off")) {
            AICamera_MonitorDIStop();
          }

        } else if (isSameString(segments[0].c_str(), "pwm")) {
          AICamera_setPWM(segments[1]);

        } else if (isSameString(segments[0].c_str(), "dio")) {
          if (isSameString(segments[1].c_str(), "in")) {
            AICamera_setDIODirection("1", "in");
          } else if (isSameString(segments[1].c_str(), "out")) {
            AICamera_setDIODirection("1", "out");
          } else if (isSameString(segments[1].c_str(), "o")) {
            AICamera_setDIOOut("1", segments[2]);
          }
        }

      } else {
        res.status = 400;  // Bad Request
        res.set_content("{ \"error\": \"Invalid request\" }", "application/json");
      }
    });
    svr.listen("0.0.0.0", 8765);

  } else {
    xlog("USE MQTT...");

    // MQTT loop
    mosqpp::lib_init();
    MQTTClient client("my_client");

    client.connect("localhost", 1883);
    client.subscribe(nullptr, "PX/VBS/Cmd");

    while (true) {
      client.loop();
    }
    mosqpp::lib_cleanup();
  }

  return 0;
}
