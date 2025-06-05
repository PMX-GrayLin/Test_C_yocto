#include "test.hpp"

#include <vector>

#include "device.hpp"
#include "mqttx.hpp"

#if defined(ENABLE_CIS)
#include "cam_omnivision.hpp"
#endif

#if defined(ENABLE_Gige)
#include "cam_gige_hikrobot.hpp"
#endif

#if defined(ENABLE_OST)
#include "oti322.hpp"
#include "otpa8.hpp"
#endif

#if defined(ENABLE_TestCode)
#include "test_gst.hpp"
#include "test_ocv.hpp"
#endif

// #if defined(ENABLE_FTDI)
// #include "ftd2xx.h"
// #include "libft4222.h"
// #endif

void handle_RESTful(std::vector<std::string> segments) {
  if (isSameString(segments[0], "led")) {
    if (segments.size() == 3) {
      FW_setLED(segments[1], segments[2]);
    } else {
      xlog("param may missing...");
    }

  } else if (isSameString(segments[0], "do")) {
    FW_setDO(segments[1], segments[2]);

  } else if (isSameString(segments[0], "di")) {
    if (isSameString(segments[1], "on")) {
      FW_MonitorDIStart();
    } else if (isSameString(segments[1], "off")) {
      FW_MonitorDIStop();
    }

  } else if (isSameString(segments[0], "triger")) {
    if (isSameString(segments[1], "on")) {
      FW_MonitorTrigerStart();
    } else if (isSameString(segments[1], "off")) {
      FW_MonitorTrigerStop();
    }

  } else if (isSameString(segments[0], "pwm")) {
    FW_setPWM(segments[1], segments[2]);

  } else if (isSameString(segments[0], "dio")) {
    if (isSameString(segments[2], "set")) {
      FW_setDIODirection(segments[1], segments[3]);
    } else if (isSameString(segments[2], "do")) {
      FW_setDIOOut(segments[1], segments[3]);
    }

  } else if (segments.size() >= 3 && isSameString(segments[0].c_str(), "gpio")) {
    int gpio_num = stoi(segments[1]);
    int gpio_vaue = (segments[2] == "1") ? 1 : 0 ;
    FW_setGPIO(gpio_num, gpio_vaue);

#if defined(ENABLE_CIS)

  } else if (isSameString(segments[0], "gst")) {
    
    CIS_handle_RESTful(segments);

#endif

#if defined(ENABLE_Gige)

  } else if (isSameString(segments[0], "gige") || isSameString(segments[0], "gige1")) {
    Gige_handle_RESTful_hik(segments);

#endif

#if defined(ENABLE_TestCode)

  } else if (isSameString(segments[0], "gstt")) {
    int testCase = std::stoi(segments[1]);
    test_gst(testCase);
    
#endif

  } else if (isSameString(segments[0], "mqtt")) {
    if (isSameString(segments[1], "start")) {
      thread_mqtt_start();
    } else if (isSameString(segments[1], "stop")) {
      thread_mqtt_stop();
    } else if (isSameString(segments[1], "send")) {
      MQTTClient::send_message_static("topic123", "message123");
    }

  } else if (isSameString(segments[0], "cmd")) {
    exec_command(segments[1]);

  } else if (isSameString(segments[0], "info")) {
    // some information functions

  } else if (isSameString(segments[0], "tf")) {
    // test functions

    // xlog("time:%s", getTimeString().c_str());
    
    product = exec_command("fw_printenv | grep '^product=' | cut -d '=' -f2");
    xlog("product:%s", product.c_str());
  }
}

// bool isMQTTRunning = false;
// void thread_mqtt_start() {
//   if (!isMQTTRunning) {
//     std::thread([] {
//       xlog("thread_mqtt_start start >>>>");
//       isMQTTRunning = true;

//       // MQTT loop
//       mosqpp::lib_init();
//       MQTTClient client("my_client");

//       client.connect("localhost", 1883);
//       client.subscribe(nullptr, "PX/VBS/Cmd");

//       while (true) {
//         client.loop();  // This triggers on_message()
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//       }
//       mosqpp::lib_cleanup();

//       xlog("thread_mqtt_start stop >>>>");
//     }).detach();  // Detach to run in the background

//   } else {
//     xlog("thread_mqtt_start already running...");
//   }
// }

// void thread_mqtt_stop() {
//   isMQTTRunning = false;
// }

// void handle_mqtt(std::string payload) {
//   xlog("MQTT payload:%s", payload.c_str());
// }

// MQTTClient::MQTTClient(const char* id)
//     : mosqpp::mosquittopp(id) {}

// void MQTTClient::on_message(const struct mosquitto_message* message) {
//   std::string payload(static_cast<char*>(message->payload), message->payloadlen);

//   handle_mqtt(payload);
// }

#if defined(ENABLE_FTDI)

#define SLAVE_ADDR 0x68
#define I2C_SPEED_KHZ 400
#define READ_LEN 525

void test_ftdi() {
  FT_STATUS ftStatus;
  FT_HANDLE ftHandle = NULL;
  DWORD devCount = 0;

  // Initialize D2XX
  ftStatus = FT_CreateDeviceInfoList(&devCount);
  if (ftStatus != FT_OK || devCount == 0) {
      fprintf(stderr, "No FTDI devices found\n");
      return;
  }

  // Open first FTDI device
  ftStatus = FT_Open(0, &ftHandle);
  if (ftStatus != FT_OK) {
      fprintf(stderr, "FT_Open failed\n");
      return;
  }

  // Initialize as I2C Master
  ftStatus = FT4222_I2CMaster_Init(ftHandle, I2C_SPEED_KHZ);
  if (ftStatus != FT_OK) {
      fprintf(stderr, "FT4222_I2CMaster_Init failed\n");
      FT_Close(ftHandle);
      return;
  }

  // Write two bytes: register 0x4E and offset 0x00
  uint8_t writeBuf[2] = { 0x4E, 0x00 };
  uint16_t bytesWritten = 0;
  ftStatus = FT4222_I2CMaster_Write(ftHandle, SLAVE_ADDR, writeBuf, 2, &bytesWritten);
  if (ftStatus != FT_OK || bytesWritten != 2) {
      fprintf(stderr, "I2C write failed\n");
      FT_Close(ftHandle);
      return;
  }

  // Read 525 bytes from device
  uint8_t readBuf[READ_LEN] = {0};
  uint16_t bytesRead = 0;
  ftStatus = FT4222_I2CMaster_Read(ftHandle, SLAVE_ADDR, readBuf, READ_LEN, &bytesRead);
  if (ftStatus != FT_OK || bytesRead != READ_LEN) {
      fprintf(stderr, "I2C read failed\n");
      FT_Close(ftHandle);
      return;
  }

  // Print data
  for (int i = 0; i < READ_LEN; i++) {
      printf("%02X ", readBuf[i]);
      if ((i + 1) % 16 == 0) printf("\n");
  }
  printf("\n");

  FT_Close(ftHandle);
}

#endif // ENABLE_FTDI

int main(int argc, char* argv[]) {
  xlog("Compiled at %s %s", __DATE__, __TIME__);
  for (int i = 0; i < argc; ++i) {
    xlog("argv[%d]:%s", i, argv[i]);
  }

  httplib::Server svr;

#if defined(ENABLE_OST)

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
    float objectTemp[256] = {0.0};
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

#endif  // ENABLE_OST

  svr.Get(R"(/fw/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
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

  return 0;
}
