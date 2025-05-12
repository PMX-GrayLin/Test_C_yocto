#include "test.hpp"

#include "test_gst.hpp"
#include "test_ocv.hpp"
#include "aicamera.hpp"
#include "oti322.hpp"
#include "otpa8.hpp"

void handle_RESTful(std::vector<std::string> segments) {
  if (isSameString(segments[0].c_str(), "gst")) {
    if (isSameString(segments[1].c_str(), "start")) {
      AICamera_streamingStart();
    } else if (isSameString(segments[1].c_str(), "stop")) {
      AICamera_streamingStop();
    }

  } else if (isSameString(segments[0].c_str(), "gige")) {
    if (isSameString(segments[1].c_str(), "start")) {
      AICamera_streamingStart_GigE();
    } else if (isSameString(segments[1].c_str(), "stop")) {
      AICamera_streamingStop_GigE();
    }

  } else if (isSameString(segments[0].c_str(), "tp")) {
    xlog("take picture");
    std::string path = "";
    if (segments.size() > 1 && !segments[1].empty()) {

      path = segments[1];
      const std::string from = "%2F";
      const std::string to = "/";
      size_t start_pos = 0;
      while ((start_pos = path.find(from, start_pos)) != std::string::npos) {
        path.replace(start_pos, from.length(), to);
        start_pos += to.length();
      }

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

  } else if (isSameString(segments[0].c_str(), "triger")) {
    if (isSameString(segments[1].c_str(), "on")) {
      AICamera_MonitorTrigerStart();
    } else if (isSameString(segments[1].c_str(), "off")) {
      AICamera_MonitorTrigerStop();
    }

  } else if (isSameString(segments[0].c_str(), "pwm")) {
    AICamera_setPWM(segments[1]);

  } else if (isSameString(segments[0].c_str(), "dio")) {
    if (isSameString(segments[2].c_str(), "set")) {
      AICamera_setDIODirection(segments[1], segments[3]);
    } else if (isSameString(segments[2].c_str(), "do")) {
      AICamera_setDIOOut(segments[1], segments[3]);
    }

  } else if (isSameString(segments[0].c_str(), "arv")) {
    aravisTest();

  } else if (isSameString(segments[0].c_str(), "gstt")) {
    int testCase = std::stoi(segments[1]);
    gst_test(testCase);
  }
}

bool isMQTTRunning = false;
void thread_mqtt_start() {
  if (!isMQTTRunning) {
    std::thread([] {
      xlog("thread_mqtt_start start >>>>");
      isMQTTRunning = true;

      // MQTT loop
      mosqpp::lib_init();
      MQTTClient client("my_client");

      client.connect("localhost", 1883);
      client.subscribe(nullptr, "PX/VBS/Cmd");

      while (true) {
        xlog("");
        client.loop();
        xlog("");
      }
      mosqpp::lib_cleanup();

      xlog("thread_mqtt_start stop >>>>");
    }).detach();  // Detach to run in the background

  } else {
    xlog("thread_mqtt_start already running...");
  }
}

void thread_mqtt_stop() {
}

void handle_mqtt(std::string payload) {
  xlog("MQTT payload:%s", payload.c_str());
}

// // MQTTClient gClient;
// class MQTTClient : public mosqpp::mosquittopp {
//  public:
//   MQTTClient(const char* id) : mosqpp::mosquittopp(id) {}

//   void on_message(const struct mosquitto_message* message) override {
//     std::string payload((char*)message->payload, message->payloadlen);

//     handle_mqtt(payload);
//   }
// };

MQTTClient::MQTTClient(const char* id)
    : mosqpp::mosquittopp(id) {}
    
void MQTTClient::on_message(const struct mosquitto_message* message) {
  std::string payload(static_cast<char*>(message->payload), message->payloadlen);

  handle_mqtt(payload);
}

int main(int argc, char* argv[]) {
  xlog("");
  for (int i = 0; i < argc; ++i) {
    xlog("argv[%d]:%s", i, argv[i]);
  }

  bool isUseMQTT = false;
  if (!isUseMQTT) {
    xlog("USE RESTful...");
    
    // REST API: Get Temperature
    httplib::Server svr;
    svr.Get("/temperatures", [&](const httplib::Request& req, httplib::Response& res) {
      
      xlog("query otpa8...");
      float ambientTemp = 0.0;
      float objectTemp = 0.0;
      OTPA8 otpa8;
      otpa8.readTemperature_max(ambientTemp, objectTemp);
      std::string response = "{ \"ambient\": " + std::to_string(ambientTemp) +
                             ", \"object\": " + std::to_string(objectTemp) + " }";
      res.set_content(response, "application/json");
    });
    svr.Get("/temperature_array", [&](const httplib::Request& req, httplib::Response& res) {
      
      xlog("query otpa16...");
      float ambientTemp = 0.0;
      float objectTemp[256] = { 0.0 };
      OTPA8 otpa8;
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
        xlog("RESTful string:%s", matched_str.c_str());
  
        // Split the matched string by '/' and store each segment in the vector
        std::stringstream ss(matched_str);
        std::string segment;
        while (std::getline(ss, segment, '/')) {
          segments.push_back(segment);
        }
        // for (const auto& seg : segments) {
        //   xlog("segment:%s", seg.c_str());
        // }
        for (size_t i = 0; i < segments.size(); ++i) {
          xlog("segment[%zu]:%s", i, segments[i].c_str());
        }

        handle_RESTful(segments);
  
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
