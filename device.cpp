#include "device.hpp"

#include <fstream>
#include <sstream>
#include <gpiod.h>
#include <poll.h>
#include <chrono>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <atomic>
#include <linux/if_link.h>

#include <libudev.h>

#include <fcntl.h>             // O_RDONLY
#include <unistd.h>            // open, close
#include <sys/ioctl.h>         // ioctl
#include <linux/videodev2.h>   // v4l2_capability, VIDIOC_QUERYCAP

#ifndef IFF_LOWER_UP
#define IFF_LOWER_UP 0x10000
#endif

#include "restfulx.hpp"

#define DEBOUNCE_TIME_MS 50

std::string product = "ai_camera_plus";         // ai_camera_plus or vision_hub_plus
std::string hostname_prefix = "aicamera";       // aicamera or visionhub

// PWM
const std::string path_pwm = "/sys/devices/platform/soc/10048000.pwm/pwm/pwmchip0";
const int pwmPeriod = 200000;                   // 5 kHz

// DI
int DI_GPIOs[NUM_DI] = {digp_1, digp_2};        // DI GPIO
std::thread t_aicamera_monitorDI;
std::atomic<bool> isMonitorDI = false;
GPIO_LEVEl DI_gpio_level_last[NUM_DI] = {gpiol_unknown, gpiol_unknown};
GPIO_LEVEl DI_gpio_level_new[NUM_DI] = {gpiol_unknown, gpiol_unknown};
uint64_t DI_last_event_time[NUM_DI] = {0};

// Triger
int Triger_GPIOs[NUM_Triger] = {tgp_1, tgp_2};  // Triger GPIO
std::thread t_aicamera_monitorTriger;
std::atomic<bool> isMonitorTriger = false;
GPIO_LEVEl Triger_gpio_level_last[NUM_Triger] = {gpiol_unknown, gpiol_unknown};
GPIO_LEVEl Triger_gpio_level_new[NUM_Triger] = {gpiol_unknown, gpiol_unknown};
uint64_t Triger_last_event_time[NUM_Triger] = {0};

// DO
int DO_GPIOs[NUM_DO] = {dogp_1, dogp_2};        // DO GPIO

// DIO
int DIO_DI_GPIOs[NUM_DIO] = {diodigp_1, diodigp_2, diodigp_3, diodigp_4};       // DI GPIO
int DIO_DO_GPIOs[NUM_DIO] = {diodogp_1, diodogp_2, diodogp_3, diodogp_4};       // DO GPIO
DIO_Direction dioDirection[NUM_DIO] = {diod_in};
std::thread t_aicamera_monitorDIO[NUM_DIO];
std::atomic<bool> isMonitorDIO[NUM_DIO] = {false};
GPIO_LEVEl DIODI_gpio_level_last[NUM_DIO] = {gpiol_unknown, gpiol_unknown};
GPIO_LEVEl DIODI_gpio_level_new[NUM_DIO] = {gpiol_unknown, gpiol_unknown};
uint64_t DIODI_last_event_time[NUM_DIO] = {0};

// Net Interface
std::thread t_monitorNetLink;
std::atomic<bool> isMonitorNetLink(false);
int wakeupFd = -1;

// UVC
static std::thread t_monitorUVC;
static std::atomic<bool> isMonitorUVC{false};
extern void UVC_setDevicePath(const string& devicePath);
extern void UVC_streamingStop();

void FW_getProduct() {
  product = exec_command("fw_printenv | grep '^product=' | cut -d '=' -f2");
  // xlog("product:%s", product.c_str());
}

void FW_getHostnamePrefix() {
  hostname_prefix = exec_command("hostname | awk -F'-' '{print $1}'");
  // xlog("hostname_prefix:%s", hostname_prefix.c_str());
}

void FW_getDeviceInfo() {
  FW_getProduct();
  FW_getHostnamePrefix();
}

bool FW_isDeviceAICamera() {
  return (product.find("ai_camera_plus") != std::string::npos ||
          hostname_prefix.find("aicamera") != std::string::npos);
}
bool FW_isDeviceAICameraPlus() {
  // i2c2, slave address 0x36 for cis camera
  return FW_isI2CAddressExist("2", "0x36");
}

bool FW_isDeviceVisionHub() {
  return (product.find("vision_hub_plus") != std::string::npos ||
          hostname_prefix.find("visionhub") != std::string::npos);
}

void FW_writePWMFile(const std::string &path, const std::string &value) {
  std::ofstream fs(path);
  if (fs) {
      fs << value;
      fs.close();
  } else {
    xlog("Failed to write to %s", path.c_str());
  }
}

void FW_setPWM(const std::string &pwmIndex, const std::string &sPercent) {
  // actual is pwm1 & pwm0 (aicamera always 1)
  // Map logical index to actual hardware index
  std::string actualPwmIndex;
  if (pwmIndex == "1") {
    actualPwmIndex = "1";
  } else if (pwmIndex == "2" && FW_isDeviceVisionHub()) {
    actualPwmIndex = "0";
  } else {
    xlog("Invalid pwmIndex: %s", pwmIndex.c_str());
    return;
  }

  std::string pwmTarget = path_pwm + "/pwm" + actualPwmIndex;
  std::string path_pwmExport = path_pwm + "/export";
  xlog("pwmTarget:%s, percent:%s", pwmTarget.c_str(), sPercent.c_str());

  // Export the PWM channel if not already present
  if (!isPathExist(pwmTarget.c_str())) {
    xlog("PWM init... pwm%s", actualPwmIndex.c_str());
    FW_writePWMFile(path_pwmExport, actualPwmIndex);
    usleep(500000);  // sleep 500ms
    FW_writePWMFile(pwmTarget + "/period", std::to_string(pwmPeriod));
  }

  int percent = std::stoi(sPercent);
  if (percent != 0) {
    int duty_cycle = pwmPeriod * percent / 100;
    FW_writePWMFile(pwmTarget + "/duty_cycle", std::to_string(duty_cycle));
    FW_writePWMFile(pwmTarget + "/enable", "1");
  } else {
    FW_writePWMFile(pwmTarget + "/enable", "0");
  }
}

int FW_getGPIO(int gpio_num) {
  // xlog("Getting GPIO status for gpio:%d", gpio_num);

  gpiod_chip *chip;
  gpiod_line *line;
  int value;

  // Open GPIO chip
  chip = gpiod_chip_open(GPIO_CHIP);
  if (!chip) {
    xlog("Failed to open GPIO chip:%s", GPIO_CHIP);
    return -1;
  }

  // Get GPIO line
  line = gpiod_chip_get_line(chip, gpio_num);
  if (!line) {
    xlog("Failed to get GPIO line");
    gpiod_chip_close(chip);
    return -1;
  }

  struct gpiod_line_request_config cfg = {
      .consumer = "my_gpio_control",
      .request_type = GPIOD_LINE_REQUEST_DIRECTION_AS_IS,
  };

  if (gpiod_line_request(line, &cfg, 0) < 0) {
    xlog("Failed to request GPIO line with direction as-is");
    gpiod_chip_close(chip);
    return -1;
  }

  // Read the GPIO value
  value = gpiod_line_get_value(line);
  if (value < 0) {
    xlog("Failed to get GPIO value");
  } else {
    // xlog("GPIO value:%d", value);
  }

  // Release resources
  gpiod_line_release(line);
  gpiod_chip_close(chip);

  return value;
}

void FW_setGPIO(int gpio_num, int value) {
  // xlog("gpiod version:%s", gpiod_version_string());
  // xlog("gpio:%d, value:%d", gpio_num, value);

  gpiod_chip *chip;
  gpiod_line *line;

  // Open GPIO chip
  chip = gpiod_chip_open(GPIO_CHIP);
  if (!chip) {
    xlog("Failed to open GPIO chip:%s", GPIO_CHIP);
    return;
  }

  // Get GPIO line
  line = gpiod_chip_get_line(chip, gpio_num);
  if (!line) {
    xlog("Failed to get GPIO line");
    gpiod_chip_close(chip);
    return;
  }

  // Request line as output
  if (gpiod_line_request_output(line, "my_gpio_control", value) < 0) {
    xlog("Failed to request GPIO line as output");
    gpiod_chip_close(chip);
    return;
  }

  // Set the GPIO value
  if (gpiod_line_set_value(line, value) < 0) {
    xlog("Failed to set GPIO value");
  }

  // Release resources
  gpiod_line_release(line);
  gpiod_chip_close(chip);
}

void FW_toggleGPIO(int gpio_num) {
  int current_value = FW_getGPIO(gpio_num);
  if (current_value < 0) {
    xlog("Failed to read GPIO:%d for toggle", gpio_num);
    return;
  }
  FW_setGPIO(gpio_num, !current_value);
}

void FW_setLED(string led_index, string led_color) {
  // xlog("led_index:%s, led_color:%s", led_index.c_str(), led_color.c_str());
  int gpio_index1 = 0;
  int gpio_index2 = 0;

  if (led_index == "1") {
    gpio_index1 = ledgp_1_red;
    gpio_index2 = ledgp_1_green;
  } else if (led_index == "2") {
    gpio_index1 = ledgp_2_red;
    gpio_index2 = ledgp_2_green;
  } else if (led_index == "3") {
    gpio_index1 = ledgp_3_red;
    gpio_index2 = ledgp_3_green;
  } else if (FW_isDeviceVisionHub()) {
    if (led_index == "4") {
      gpio_index1 = ledgp_4_red;
      gpio_index2 = ledgp_4_green;
    } else if (led_index == "5") {
      gpio_index1 = ledgp_5_red;
      gpio_index2 = ledgp_5_green;
    } else {
      xlog("LED index '%s' not supported.", led_index.c_str());
      return;
    }
  } else {
    xlog("LED index '%s' not supported", led_index.c_str());
    return;
  }

  if (isSameString(led_color, "red")) {
    FW_setGPIO(gpio_index1, 1);
    FW_setGPIO(gpio_index2, 0);
  } else if (isSameString(led_color, "green")) {
    FW_setGPIO(gpio_index1, 0);
    FW_setGPIO(gpio_index2, 1);
  } else if (isSameString(led_color, "orange")) {
    FW_setGPIO(gpio_index1, 1);
    FW_setGPIO(gpio_index2, 1);
  } else if (isSameString(led_color, "off")) {
    FW_setGPIO(gpio_index1, 0);
    FW_setGPIO(gpio_index2, 0);
  }
}

void FW_toggleLED(string led_index, string led_color) {
  int gpio_index1 = 0;
  int gpio_index2 = 0;

  if (led_index == "1") {
    gpio_index1 = ledgp_1_red;
    gpio_index2 = ledgp_1_green;
  } else if (led_index == "2") {
    gpio_index1 = ledgp_2_red;
    gpio_index2 = ledgp_2_green;
  } else if (led_index == "3") {
    gpio_index1 = ledgp_3_red;
    gpio_index2 = ledgp_3_green;
  } else if (FW_isDeviceVisionHub()) {
    if (led_index == "4") {
      gpio_index1 = ledgp_4_red;
      gpio_index2 = ledgp_4_green;
    } else if (led_index == "5") {
      gpio_index1 = ledgp_5_red;
      gpio_index2 = ledgp_5_green;
    } else {
      xlog("LED index '%s' not supported.", led_index.c_str());
      return;
    }
  } else {
    xlog("LED index '%s' not supported", led_index.c_str());
    return;
  }

  if (isSameString(led_color, "red")) {
    FW_toggleGPIO(gpio_index1);
  } else if (isSameString(led_color, "green")) {
    FW_toggleGPIO(gpio_index2);
  } else if (isSameString(led_color, "orange")) {
    FW_toggleGPIO(gpio_index1);
    FW_toggleGPIO(gpio_index2);
  } 
}

uint64_t get_current_millis() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

void Thread_FWMonitorDI() {
  struct gpiod_chip *chip;
  struct gpiod_line *lines[NUM_DI];
  struct pollfd fds[NUM_DI];
  int i, ret;
  int levelCounter = 0;

  // Open GPIO chip
  chip = gpiod_chip_open(GPIO_CHIP);
  if (!chip) {
    xlog("Failed to open GPIO chip: %s", GPIO_CHIP);
    return;
  }

  // Configure GPIOs for edge detection
  for (i = 0; i < NUM_DI; i++) {
    lines[i] = gpiod_chip_get_line(chip, DI_GPIOs[i]);
    if (!lines[i]) {
      xlog("Failed to get GPIO line %d", DI_GPIOs[i]);
      continue;
    }

    // Request GPIO line for both rising and falling edge events
    ret = gpiod_line_request_both_edges_events(lines[i], "gpio_interrupt");
    if (ret < 0) {
      xlog("Failed to request GPIO %d for edge events", DI_GPIOs[i]);
      gpiod_line_release(lines[i]);
      lines[i] = NULL;
      continue;
    }

    // Get file descriptor for polling
    fds[i].fd = gpiod_line_event_get_fd(lines[i]);
    fds[i].events = POLLIN;
  }

  // re-Sync state & update to App
  for (i = 0; i < NUM_DI; i++) {
    if (!lines[i]) continue;

    int val = gpiod_line_get_value(lines[i]);
    if (val < 0) {
      xlog("Failed to read GPIO %d value during sync", DI_GPIOs[i]);
      continue;
    }

    DI_gpio_level_last[i] = (val == 1) ? gpiol_high : gpiol_low;
    RESTful_send_DI(i, val == 1);
  }

  xlog("^^^^ Start ^^^^");

  // Main loop to monitor GPIOs
  while (isMonitorDI) {
    ret = poll(fds, NUM_DI, 500);  // timeout = 500ms, to check stop flag periodically

    if (ret == 0) {
      // timeout to check stop flag "isMonitorDI" periodically
      
      // Level-triggered validation
      levelCounter++;
      if (levelCounter > 3) {
        levelCounter = 0;
        // xlog("Level Check!!");

        for (i = 0; i < NUM_DI; i++) {
          if (!lines[i]) continue;

          int val = gpiod_line_get_value(lines[i]);
          if (val < 0) continue;

          GPIO_LEVEl current_level = (val == 1) ? gpiol_high : gpiol_low;
          if (current_level != DI_gpio_level_last[i]) {
            DI_gpio_level_last[i] = current_level;
            DI_last_event_time[i] = get_current_millis();;
            xlog("Level State Change...");
            RESTful_send_DI(i, val == 1);
          }
        }
      }
    }

    if (ret < 0) {
      xlog("Error in poll");
      break;
    }

    if (ret > 0) {
      // Check which GPIO triggered the event
      for (i = 0; i < NUM_DI; i++) {
        if (fds[i].revents & POLLIN) {
          struct gpiod_line_event event;
          if (gpiod_line_event_read(lines[i], &event) < 0) {
            xlog("Failed to read GPIO event on line %d", DI_GPIOs[i]);
            continue;
          }

          // Debounce per line
          uint64_t now = get_current_millis();
          if (now - DI_last_event_time[i] < DEBOUNCE_TIME_MS) {
            continue;
          }

          DI_gpio_level_new[i] = (gpiod_line_get_value(lines[i]) == 1) ? gpiol_high : gpiol_low;

          if (DI_gpio_level_new[i] != DI_gpio_level_last[i]) {
            DI_gpio_level_last[i] = DI_gpio_level_new[i];
            DI_last_event_time[i] = now;
            // xlog("GPIO %d event detected! status:%s", DI_GPIOs[i], (DI_gpio_level_last[i] == gpiol_high) ? "high" : "low");

            RESTful_send_DI(i, DI_gpio_level_last[i] == gpiol_high);
          }
        }
      }
    }
  }

  xlog("^^^^ Stop ^^^^");

  // Cleanup
  for (i = 0; i < NUM_DI; i++) {
    gpiod_line_release(lines[i]);
  }
  gpiod_chip_close(chip);
}

void FW_MonitorDIStart() {
  if (isMonitorDI) {
    xlog("thread already running");
    return;
  }
  isMonitorDI = true;
  t_aicamera_monitorDI = std::thread(Thread_FWMonitorDI);
}
void FW_MonitorDIStop() {
  if (!isMonitorDI) {
    xlog("thread not running...");
    return;
  }

  isMonitorDI = false;

  if (t_aicamera_monitorDI.joinable()) {
    t_aicamera_monitorDI.join();
  }
}

void Thread_FWMonitorTriger() {
  struct gpiod_chip *chip;
  struct gpiod_line *lines[NUM_Triger];
  struct pollfd fds[NUM_Triger];
  int i, ret;
  int levelCounter = 0;

  // Open GPIO chip
  chip = gpiod_chip_open(GPIO_CHIP);
  if (!chip) {
    xlog("Failed to open GPIO chip: %s", GPIO_CHIP);
    return;
  }

  // Configure GPIOs for edge detection
  for (i = 0; i < NUM_Triger; i++) {
    lines[i] = gpiod_chip_get_line(chip, Triger_GPIOs[i]);
    if (!lines[i]) {
      xlog("Failed to get GPIO line %d", Triger_GPIOs[i]);
      continue;
    }

    // Request GPIO line for both rising and falling edge events
    ret = gpiod_line_request_both_edges_events(lines[i], "gpio_interrupt");
    if (ret < 0) {
      xlog("Failed to request GPIO %d for edge events", Triger_GPIOs[i]);
      gpiod_line_release(lines[i]);
      lines[i] = NULL;
      continue;
    }

    // Get file descriptor for polling
    fds[i].fd = gpiod_line_event_get_fd(lines[i]);
    fds[i].events = POLLIN;
  }

  // re-Sync state & update to App
  for (i = 0; i < NUM_Triger; i++) {
    if (!lines[i]) continue;

    int val = gpiod_line_get_value(lines[i]);
    if (val < 0) {
      xlog("Failed to read GPIO %d value during sync", Triger_GPIOs[i]);
      continue;
    }

    Triger_gpio_level_last[i] = (val == 1) ? gpiol_high : gpiol_low;
    RESTful_send_Trigger(i, val == 1);
  }

  xlog("^^^^ Start ^^^^");

  // Main loop to monitor GPIOs
  while (isMonitorTriger) {
    ret = poll(fds, NUM_Triger, 500);  // timeout = 500ms, to check stop flag periodically

    if (ret == 0) {
      // timeout to check stop flag "isMonitorTriger" periodically

      // Level-triggered validation
      levelCounter++;
      if (levelCounter > 3) {
        levelCounter = 0;

        for (i = 0; i < NUM_Triger; i++) {
          if (!lines[i]) continue;

          int val = gpiod_line_get_value(lines[i]);
          if (val < 0) continue;

          GPIO_LEVEl current_level = (val == 1) ? gpiol_high : gpiol_low;
          if (current_level != Triger_gpio_level_last[i]) {
            Triger_gpio_level_last[i] = current_level;
            Triger_last_event_time[i] = get_current_millis();
            xlog("Level State Change (Triger %d)...", Triger_GPIOs[i]);
            RESTful_send_Trigger(i, val == 1);
          }
        }
      }
    }

    if (ret < 0) {
      xlog("Error in poll");
      break;
    }

    if (ret > 0) {
      // Check which GPIO triggered the event
      for (i = 0; i < NUM_Triger; i++) {
        if (fds[i].revents & POLLIN) {
          struct gpiod_line_event event;
          if (gpiod_line_event_read(lines[i], &event) < 0) {
            xlog("Failed to read GPIO event on line %d", Triger_GPIOs[i]);
            continue;
          }

          // Debounce per line
          uint64_t now = get_current_millis();
          if (now - Triger_last_event_time[i] < DEBOUNCE_TIME_MS) {
            continue;
          }

          Triger_gpio_level_new[i] = (gpiod_line_get_value(lines[i]) == 1) ? gpiol_high : gpiol_low;

          if (Triger_gpio_level_new[i] != Triger_gpio_level_last[i]) {
            Triger_gpio_level_last[i] = Triger_gpio_level_new[i];
            Triger_last_event_time[i] = now;
            // xlog("GPIO %d event detected! status:%s", Triger_GPIOs[i], (Triger_gpio_level_last[i] == gpiol_high) ? "high" : "low");

            RESTful_send_Trigger(i, Triger_gpio_level_last[i] == gpiol_high);
          }
        }
      }
    }
  }

  xlog("^^^^ Stop ^^^^");

  // Cleanup
  for (i = 0; i < NUM_Triger; i++) {
    gpiod_line_release(lines[i]);
  }
  gpiod_chip_close(chip);
}
void FW_MonitorTrigerStart() {
  if (!FW_isDeviceVisionHub()) {
    xlog("return... Triger function it's only for VisionHub");
    return;
  }

  if (isMonitorTriger) {
    xlog("thread already running");
    return;
  }
  isMonitorTriger = true;
  t_aicamera_monitorTriger = std::thread(Thread_FWMonitorTriger);
}
void FW_MonitorTrigerStop() {
  if (!isMonitorTriger) {
    xlog("thread not running...");
    return;
  }

  isMonitorTriger = false;

  if (t_aicamera_monitorTriger.joinable()) {
    t_aicamera_monitorTriger.join();
  }
}

void FW_setDO(string index_do, string on_off) {
  int index_gpio = 0;
  bool isON = false;

  int index = std::stoi(index_do);
  if (index > 0 && index <= NUM_DO) {
    index_gpio = DO_GPIOs[index - 1];
  } else {
    xlog("index out of range...");
    return;
  }

  if (isSameString(on_off, "on") || isSameString(on_off, "1")) {
    isON = true;
  } else if (isSameString(on_off, "off") || isSameString(on_off, "0")) {
    isON = false;
  } else {
    xlog("input string should be on or off...");
    return;
  }

  FW_setGPIO(index_gpio, isON ? 1 : 0);
}

void Thread_FWMonitorDIOIn(int index_dio) {

  struct gpiod_chip *chip;
  struct gpiod_line *line;
  struct pollfd fd;
  int ret;

  if (index_dio < 0 || index_dio >= FW_getDIONum()) {
    xlog("Invalid DIO index: %d", index_dio);
    return;
  }

  // Open GPIO chip
  chip = gpiod_chip_open(GPIO_CHIP);
  if (!chip) {
    xlog("Failed to open GPIO chip: %s", GPIO_CHIP);
    return;
  }

  // Get specified GPIO line
  line = gpiod_chip_get_line(chip, DIO_DI_GPIOs[index_dio]);
  if (!line) {
    xlog("Failed to get GPIO line %d", DIO_DI_GPIOs[index_dio]);
    gpiod_chip_close(chip);
    return;
  }

  // Request GPIO line for both rising and falling edge events
  ret = gpiod_line_request_both_edges_events(line, "gpio_interrupt");
  if (ret < 0) {
    xlog("Failed to request GPIO %d for edge events", DIO_DI_GPIOs[index_dio]);
    gpiod_chip_close(chip);
    return;
  }

  // Get file descriptor for polling
  fd.fd = gpiod_line_event_get_fd(line);
  fd.events = POLLIN;

  // get init value & update to App
  RESTful_send_DIODI(index_dio, (gpiod_line_get_value(line) == 1));

  xlog("Thread Monitoring GPIO %d for events...start", DIO_DI_GPIOs[index_dio]);

  // Main loop to monitor GPIO
  while (isMonitorDIO[index_dio]) {
    // ret = poll(&fd, 1, -1);  // Wait indefinitely for an event
    ret = poll(&fd, 1, 500);  // timeout = 500ms, to check stop flag periodically
    if (ret < 0) {
      xlog("Error in poll");
      break;
    }

    if (ret == 0) {
      // timeout, continue loop to check isMonitorDIO
      continue;
    }

    if (fd.revents & POLLIN) {
      struct gpiod_line_event event;
      gpiod_line_event_read(line, &event);

      // Debounce per line
      uint64_t now = get_current_millis();
      if (now - DIODI_last_event_time[index_dio] < DEBOUNCE_TIME_MS) {
        continue;
      }

      DIODI_gpio_level_new[index_dio] = (gpiod_line_get_value(line) == 1) ? gpiol_high : gpiol_low;

      if (DIODI_gpio_level_new[index_dio] != DIODI_gpio_level_last[index_dio]) {
        DIODI_gpio_level_last[index_dio] = DIODI_gpio_level_new[index_dio];
        DIODI_last_event_time[index_dio] = now;
        // xlog("GPIO %d event detected! status:%s", DIODI_GPIOs[index_dio], (DIODI_gpio_level_last[index_dio] == gpiol_high) ? "high" : "low");

        RESTful_send_DIODI(index_dio, DIODI_gpio_level_last[index_dio] == gpiol_high);
      }
    }
  }

  xlog("Thread Monitoring GPIO %d for events...stop", DIO_DI_GPIOs[index_dio]);

  // Cleanup
  gpiod_line_release(line);
  gpiod_chip_close(chip);
}

void FW_MonitorDIOInStart(int index_dio) {
  if (isMonitorDIO[index_dio]) {
    xlog("thread already running...");
    return;
  }

  isMonitorDIO[index_dio] = true;
  t_aicamera_monitorDIO[index_dio] = std::thread(Thread_FWMonitorDIOIn, index_dio);
}

void FW_MonitorDIOInStop(int index_dio) {
  if (!isMonitorDIO[index_dio]) {
    xlog("thread not running...");
    return;
  }

  isMonitorDIO[index_dio] = false;

  if (t_aicamera_monitorDIO[index_dio].joinable()) {
    t_aicamera_monitorDIO[index_dio].join();
  }
}

void FW_setDIODirection(string index_dio, string di_do) {
  xlog("index:%s, direction:%s", index_dio.c_str(), di_do.c_str());
  int index_gpio_in = 0;
  int index_gpio_out = 0;

  int index = std::stoi(index_dio);
  if (index > 0 && index <= FW_getDIONum()) {
    index_gpio_in = DIO_DI_GPIOs[index - 1];
    index_gpio_out = DIO_DO_GPIOs[index - 1];
  } else {
    xlog("index out of range...");
    return;
  }

  if (isSameString(di_do, "di")) {
    // set flag
    dioDirection[index - 1] = diod_in;

    // make gpio out low
    FW_setGPIO(index_gpio_out, 0);

    // start monitor gpio input
    FW_MonitorDIOInStart(index - 1);

  } else if (isSameString(di_do, "do")) {
    // set flag
    dioDirection[index - 1] = diod_out;

    // stop monitor gpio input
    FW_MonitorDIOInStop(index - 1);

  } else {
    xlog("input string should be di or do...");
    return;
  }
}

void FW_setDIOOut(string index_dio, string on_off) {
  int index_gpio = 0;
  bool isON = false;
  int index = std::stoi(index_dio);

  if (dioDirection[index - 1] != diod_out) {
    xlog("direction should be set first...");
    return;
  }

  if (index > 0 && index <= FW_getDIONum()) {
    index_gpio = DIO_DO_GPIOs[index - 1];
  } else {
    xlog("index out of range...");
    return;
  }

  if (isSameString(on_off, "on") || isSameString(on_off, "1")) {
    isON = true;
  } else if (isSameString(on_off, "off") || isSameString(on_off, "0")) {
    isON = false;
  } else {
    xlog("input string should be on or off...");
    return;
  }

  FW_setGPIO(index_gpio, isON ? 1 : 0);
}

int FW_getDIONum() {
  return FW_isDeviceVisionHub() ? 4 : 2;
}

bool FW_isI2CAddressExist(const std::string &busS, const std::string &addressS) {
  string cmd = "i2cdetect -y -r " + busS;

  // Convert address string to int, supporting "0x" prefix or pure decimal
  int address = std::strtol(addressS.c_str(), nullptr, 0);  // base 0 allows 0x for hex, otherwise decimal

  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    xlog("Failed to run i2cdetect");
    return false;
  }

  std::string row;
  int row_target = address & 0xF0;
  int col_index = address & 0x0F;

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe)) {
    row = buffer;

    // Look for the row starting with the address nibble
    std::istringstream iss(row);
    std::string row_label;
    iss >> row_label;

    if (row_label.length() >= 2 && std::stoi(row_label, nullptr, 16) == row_target) {
      std::string cell;
      int index = 0;
      while (iss >> cell) {
        if (index == col_index) {
          pclose(pipe);
          if (cell == "--") {
            return false;
          }
          return true;  // found address or UU
        }
        index++;
      }
    }
  }

  pclose(pipe);
  return false;
}

// void parseNetLinkMessage(struct nlmsghdr *nlh) {
//   struct ifinfomsg *ifi = (struct ifinfomsg *)NLMSG_DATA(nlh);
//   struct rtattr *attr = IFLA_RTA(ifi);
//   int attr_len = IFLA_PAYLOAD(nlh);

//   char ifname[IF_NAMESIZE] = {0};

//   for (; RTA_OK(attr, attr_len); attr = RTA_NEXT(attr, attr_len)) {
//     if (attr->rta_type == IFLA_IFNAME) {
//       strncpy(ifname, (char *)RTA_DATA(attr), IF_NAMESIZE);
//     }
//   }

//   if (ifname[0]) {
//     bool linkUp = ifi->ifi_flags & IFF_LOWER_UP;
//     xlog("[event] Interface %s is now %s", ifname, (linkUp ? "LINK UP" : "LINK DOWN"));

//     if (isSameString(ifname, "eth1")) {
//       if (linkUp) {
//         FW_setLED("2", "green");
//       } else {
//         FW_setLED("2", "off");
//       }
//     } else if (isSameString(ifname, "eth2")) {
//       if (linkUp) {
//         FW_setLED("3", "green");
//       } else {
//         FW_setLED("3", "off");
//       }
//     }
//   }
// }

// #include <linux/netlink.h>
// #include <linux/rtnetlink.h>
// #include <net/if.h>
// #include <string.h>

// Helper to parse link messages
void parseNetLinkMessage(struct nlmsghdr *nlh) {
    struct ifinfomsg *ifi = (struct ifinfomsg *)NLMSG_DATA(nlh);
    int len = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*ifi));

    struct rtattr *attr = IFLA_RTA(ifi);
    char ifname[IF_NAMESIZE] = {0};
    char operstate[32] = {0};
    int linkup = (ifi->ifi_flags & IFF_RUNNING) ? 1 : 0;

    // Parse attributes
    for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
        switch (attr->rta_type) {
            case IFLA_IFNAME:
                strncpy(ifname, (char *)RTA_DATA(attr), sizeof(ifname) - 1);
                break;
            case IFLA_OPERSTATE:
                {
                    unsigned char state = *(unsigned char *)RTA_DATA(attr);
                    switch (state) {
                        case IF_OPER_UNKNOWN:   strcpy(operstate, "UNKNOWN"); break;
                        case IF_OPER_NOTPRESENT: strcpy(operstate, "NOTPRESENT"); break;
                        case IF_OPER_DOWN:      strcpy(operstate, "DOWN"); break;
                        case IF_OPER_LOWERLAYERDOWN: strcpy(operstate, "LOWERLAYERDOWN"); break;
                        case IF_OPER_TESTING:   strcpy(operstate, "TESTING"); break;
                        case IF_OPER_DORMANT:   strcpy(operstate, "DORMANT"); break;
                        case IF_OPER_UP:        strcpy(operstate, "UP"); break;
                        default:                sprintf(operstate, "STATE_%d", state); break;
                    }
                }
                break;
        }
    }

    // Get name from index if missing
    if (ifname[0] == '\0') {
        if_indextoname(ifi->ifi_index, ifname);
    }

    // Print info
    xlog("Netlink: %s %s (ifindex=%d, flags=0x%x, IFF_RUNNING=%d)",
         (nlh->nlmsg_type == RTM_NEWLINK ? "NEWLINK" :
          nlh->nlmsg_type == RTM_DELLINK ? "DELLINK" : "LINK"),
         ifname, ifi->ifi_index, ifi->ifi_flags, linkup);

    if (operstate[0] != '\0') {
        xlog("   operstate=%s", operstate);
    }
}


void FW_CheckNetLinkState(const char *ifname, bool isInitcheck) {
  std::string path = std::string("/sys/class/net/") + ifname + "/operstate";
  std::ifstream file(path);

  if (!file.is_open()) {
    xlog("Initial check: failed to open %s", path.c_str());
    return;
  }

  std::string state;
  std::getline(file, state);
  file.close();

  xlog("[initial] Interface %s is %s", ifname, state.c_str());

  if (isSameString(ifname, "eth1")) {
    if (isSameString(state, "up")) {
      FW_setLED("2", "green");
    } else if (isSameString(state, "down")) {
      if (!isInitcheck) {
        FW_setLED("2", "off");
      }
    }
  } else if (isSameString(ifname, "eth2")) {
    if (isSameString(state, "up")) {
      FW_setLED("3", "green");
    } else if (isSameString(state, "down")) {
      if (!isInitcheck) {
        FW_setLED("3", "off");
      }
    }
  }
}

void Thread_FWMonitorNetLink() {
  int netlinkSock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (netlinkSock < 0) {
    xlog("socket error");
    return;
  }

  struct sockaddr_nl addr = {};
  addr.nl_family = AF_NETLINK;
  addr.nl_groups = RTMGRP_LINK;

  if (bind(netlinkSock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    xlog("bind error");
    close(netlinkSock);
    return;
  }

  // Set up wakeup pipe
  int pipefd[2];
  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, pipefd) != 0) {
    xlog("socketpair error");
    close(netlinkSock);
    return;
  }

  int readFd = pipefd[0];
  wakeupFd = pipefd[1];

  fd_set fds;
  char buffer[4096];

  xlog("---- Start ----");

  while (true) {
    FD_ZERO(&fds);
    FD_SET(netlinkSock, &fds);
    FD_SET(readFd, &fds);

    int maxfd = std::max(netlinkSock, readFd) + 1;
    int ret = select(maxfd, &fds, nullptr, nullptr, nullptr);

    if (ret < 0) {
      if (errno == EINTR) continue;
      xlog("select error");
      break;
    }

    if (FD_ISSET(readFd, &fds)) {
      xlog("wakeup received, exiting thread");
      break;  // graceful exit
    }

    if (FD_ISSET(netlinkSock, &fds)) {
      ssize_t len = recv(netlinkSock, buffer, sizeof(buffer), 0);
      if (len < 0) {
        if (errno == EINTR) continue;
        xlog("recv error");
        break;
      }

      struct nlmsghdr *nlh = (struct nlmsghdr *)buffer;
      while (NLMSG_OK(nlh, len)) {
        if (nlh->nlmsg_type == RTM_NEWLINK || nlh->nlmsg_type == RTM_DELLINK) {
          parseNetLinkMessage(nlh);
        }
        nlh = NLMSG_NEXT(nlh, len);
      }
    }
  }

  close(netlinkSock);
  close(readFd);
  close(wakeupFd);
  wakeupFd = -1;

  xlog("---- Stop ----");
}

void FW_MonitorNetLinkStart() {
    if (isMonitorNetLink.load()) {
    xlog("Monitor already running.");
    return;
  }

  FW_CheckNetLinkState("eth0", true);
  FW_CheckNetLinkState("eth1", true);
  FW_CheckNetLinkState("eth2", true);

  isMonitorNetLink = true;
  t_monitorNetLink = std::thread(Thread_FWMonitorNetLink);
  // t_monitorNetLink.detach();
}

void FW_MonitorNetLinkStop() {
 if (!isMonitorNetLink.load()) return;

  isMonitorNetLink = false;

  // Unblock recv() by sending dummy message or let timeout
  if (wakeupFd != -1) {
    const char c = 'x';
    send(wakeupFd, &c, 1, 0);
  }

  if (t_monitorNetLink.joinable()) {
    t_monitorNetLink.join();
  }

  // force kill, may unsafe, but test seems OK.
  // pthread_cancel(t_monitorNetLink.native_handle());
}

bool isUvcCamera(struct udev_device* dev) {
    const char* subsystem = udev_device_get_subsystem(dev);
    if (!subsystem || std::string(subsystem) != "video4linux")
        return false;

    const char* devNode = udev_device_get_devnode(dev);
    if (!devNode || std::string(devNode).find("/dev/video") != 0)
        return false;

    const char* cap = udev_device_get_property_value(dev, "ID_V4L_CAPABILITIES");
    const char* driver = udev_device_get_property_value(dev, "ID_USB_DRIVER");
    const char* id_model = udev_device_get_property_value(dev, "ID_MODEL");

    // Only consider real UVC video devices
    if (cap && std::string(cap).find("capture") != std::string::npos &&
        driver && std::string(driver) == "uvcvideo") {
        return true;
    }

    return false;
}

void FW_CheckUVCDevices(bool isInitcheck) {
  struct udev *udev = udev_new();
  if (!udev) {
    xlog("Failed to create udev context (initial check)");
    return;
  }

  struct udev_enumerate *enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "video4linux");
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *entry;

  bool isUVCDetected = false;

  udev_list_entry_foreach(entry, devices) {
    const char *path = udev_list_entry_get_name(entry);
    struct udev_device *dev = udev_device_new_from_syspath(udev, path);
    const char *devNode = udev_device_get_devnode(dev);

    if (isUvcCamera(dev)) {
      xlog("[UVC] Initial found : %s", (devNode ? devNode : "unknown"));
      isUVCDetected = true;
      UVC_setDevicePath(std::string(devNode));

      udev_device_unref(dev);
      break;
    } 

    udev_device_unref(dev);
  }

  if (isUVCDetected) {
    FW_setLED("2", "green");
  } else {
    if (!isInitcheck) {
      FW_setLED("2", "off");
    }
  }

  udev_enumerate_unref(enumerate);
  udev_unref(udev);
}

void Thread_FWMonitorUVC() {
  struct udev *udev = udev_new();
  if (!udev) {
    xlog("Failed to create udev context");
    return;
  }

  struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
  if (!mon) {
    xlog("Failed to create udev monitor");
    udev_unref(udev);
    return;
  }

  udev_monitor_filter_add_match_subsystem_devtype(mon, "video4linux", nullptr);
  udev_monitor_enable_receiving(mon);
  int fd = udev_monitor_get_fd(mon);

  xlog("---- Start ----");

  while (isMonitorUVC) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int ret = select(fd + 1, &fds, nullptr, nullptr, &tv);
    if (ret > 0 && FD_ISSET(fd, &fds)) {
      struct udev_device *dev = udev_monitor_receive_device(mon);
      if (dev) {
        const char *devNode = udev_device_get_devnode(dev);

        if (isUvcCamera(dev)) {
          const char *action = udev_device_get_action(dev);
          const char *devNode = udev_device_get_devnode(dev);
          xlog("[UVC] %s : %s", (action ? action : "unknown"), (devNode ? devNode : "unknown"));
          if (isSameString(action, "add")) {
            UVC_setDevicePath(std::string(devNode));
            FW_setLED("2", "green");
          } else if (isSameString(action, "remove")) {
            UVC_setDevicePath("");
            FW_setLED("2", "off");
            UVC_streamingStop();
          }
        }
        udev_device_unref(dev);
      }
    }
  }

  xlog("---- Stop ----");
  udev_monitor_unref(mon);
  udev_unref(udev);
}

// Public API to start monitoring
void FW_MonitorUVCStart() {
  if (isMonitorUVC)
    return;

  FW_CheckUVCDevices(true);

  isMonitorUVC = true;
  t_monitorUVC = std::thread(Thread_FWMonitorUVC);
}

// Public API to stop monitoring
void FW_MonitorUVCStop() {
  if (!isMonitorUVC)
    return;

  isMonitorUVC = false;
  if (t_monitorUVC.joinable())
    t_monitorUVC.join();
}

