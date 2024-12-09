#include "global.hpp"
#include "aicamerag2.hpp"


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

int ioctl_get_value(int control_ID) {
  int fd = open(AICamrea_getVideoDevice().c_str(), O_RDWR);
  if (fd == -1) {
    xlog("Failed to open video device:%s", strerror(errno));
    return -1;
  }

  struct v4l2_queryctrl queryctrl;
  memset(&queryctrl, 0, sizeof(queryctrl));
  queryctrl.id = control_ID;
  if (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
    xlog("queryctrl.minimum:%d", queryctrl.minimum);
    xlog("queryctrl.maximum:%d", queryctrl.maximum);
  } else {
    xlog("ioctl fail, VIDIOC_QUERYCTRL... error:%s", strerror(errno));
  }

  struct v4l2_control ctrl;
  memset(&ctrl, 0, sizeof(ctrl));
  ctrl.id = control_ID;
  if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) == 0) {
    xlog("ctrl.value:%d", ctrl.value);
  } else {
    xlog("ioctl fail, VIDIOC_G_CTRL... error:%s", strerror(errno));
  }

  return ctrl.value;
}

int ioctl_set_value(int control_ID, int value) {
  int fd = open(AICamrea_getVideoDevice().c_str(), O_RDWR);
  if (fd == -1) {
    xlog("Failed to open video device:%s", strerror(errno));
    return -1;
  }

  struct v4l2_control ctrl;
  memset(&ctrl, 0, sizeof(ctrl));
  ctrl.id = control_ID;
  ctrl.value = value;

  if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) == 0) {
    xlog("set to:%d", ctrl.value);
  } else {
    xlog("fail to set value, error:%s", strerror(errno));
  }
  close(fd);

  return 0;
}

int AICamera_getBrightness() {
  return ioctl_get_value(V4L2_CID_BRIGHTNESS);
}

void AICamera_setBrightness(int value) {
  ioctl_set_value(V4L2_CID_BRIGHTNESS, value);
}

int AICamera_getContrast() {
  return ioctl_get_value(V4L2_CID_CONTRAST);
}

void AICamera_setContrast(int value) {
  ioctl_set_value(V4L2_CID_CONTRAST, value);
}

int AICamera_getSaturation() {
  return ioctl_get_value(V4L2_CID_SATURATION);
}

void AICamera_setSaturation(int value) {
  ioctl_set_value(V4L2_CID_SATURATION, value);
}

int AICamera_getHue() {
  return ioctl_get_value(V4L2_CID_HUE);
}

void AICamera_setHue(int value) {
  ioctl_set_value(V4L2_CID_HUE, value);
}

int AICamera_getWhiteBalanceAutomatic() {
  return ioctl_get_value(V4L2_CID_AUTO_WHITE_BALANCE);
}

void AICamera_setWhiteBalanceAutomatic(bool enable) {
  ioctl_set_value(V4L2_CID_AUTO_WHITE_BALANCE, enable ? 1 : 0);
}
