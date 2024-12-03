#include "test.h"
#include "test_gst.h"
#include "test_ocv.h"

#include <regex>

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

int main(int argc, char *argv[]) {
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

  return 0;
}
