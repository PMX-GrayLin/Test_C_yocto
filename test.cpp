#include "test.h"
#include "test_gst.h"
#include "test_ocv.h"

#include <regex>
#include <mosquittopp.h>
#include <iostream>

class MQTTClient : public mosqpp::mosquittopp {
public:
    MQTTClient(const char* id) : mosqpp::mosquittopp(id) {}

    void on_message(const struct mosquitto_message* message) override {
        std::string payload((char*)message->payload, message->payloadlen);
		xlog("payload:%s", payload.c_str());
        
        if (payload == "run_function") {
            custom_function();
        }
    }

    void custom_function() {
        std::cout << "Custom function triggered via MQTT" << std::endl;
    }
};

std::string getVideoDevice() {
  std::string result;
  FILE* pipe = popen("v4l2-ctl --list-devices | grep mtk-v4l2-camera -A 3", "r");
  if (pipe) {
    std::string output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
      output += buffer;
    }
    pclose(pipe);

    std::regex device_regex("/dev/video\\d+");
    std::smatch match;
    if (std::regex_search(output, match, device_regex)) {
      result = match[0];
    }
  }
  return result;
}

int main(int argc, char* argv[]) {
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
    int testCase = (argv[2] == nullptr) ? 0 : 1;
    ocv_test(testCase);
  } else if (!strcmp(argv[1], "dev")) {
    xlog("getVideoDevice:%s", getVideoDevice().c_str());
  }

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
