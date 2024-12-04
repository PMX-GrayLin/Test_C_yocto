#include "test.h"
#include "test_gst.h"
#include "test_ocv.h"


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
        }
    }
};

std::string AICamrea_getVideoDevice() {
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

int AICamera_getBrightness() {

  int brightness = -1;

  int fd = open(AICamrea_getVideoDevice().c_str(), O_RDWR);
  if (fd == -1) {
    xlog("Failed to open video device:%s", strerror(errno));
    return -1;
  }

  struct v4l2_queryctrl queryctrl;
  memset(&queryctrl, 0, sizeof(queryctrl));
  queryctrl.id = V4L2_CID_BRIGHTNESS;
  if (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
    xlog("queryctrl.minimum:%d", queryctrl.minimum);
    xlog("queryctrl.maximum:%d", queryctrl.maximum);
  } else {
    xlog("ioctl fail, VIDIOC_QUERYCTRL... error:%s", strerror(errno));
  }

  struct v4l2_control ctrl;
  memset(&ctrl, 0, sizeof(ctrl));
  ctrl.id = V4L2_CID_BRIGHTNESS;
  if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) == 0) {
    xlog("Current brightness:%d", ctrl.value);
  } else {
    xlog("ioctl fail, VIDIOC_G_CTRL... error:%s", strerror(errno));
  }

  close(fd);
  return 0;
}

void AICamera_setBrightness(int value) {

  int fd = open(AICamrea_getVideoDevice().c_str(), O_RDWR);
  if (fd == -1) {
    xlog("Failed to open video device:%s", strerror(errno));
    return;
  }

  struct v4l2_control ctrl;
  memset(&ctrl, 0, sizeof(ctrl));
  ctrl.id = V4L2_CID_BRIGHTNESS;
  ctrl.value = value;

  if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) == 0) {
    xlog("Brightness set to:%d", value);
  } else {
    xlog("Failed to set brightness: %s", strerror(errno));
  }
  close(fd);
}

void AICamera_setWhiteBalanceAutomatic(bool enable) {
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
    xlog("getVideoDevice:%s", AICamrea_getVideoDevice().c_str());
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
