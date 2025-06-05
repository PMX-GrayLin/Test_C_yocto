#include "device.hpp"

#include <fstream>
#include <gpiod.h>
#include <poll.h>
#include <chrono>

#include "restfulx.hpp"

#define DEBOUNCE_TIME_MS 20

// ai_camera_plus or vision_hub_plus 
std::string product = "ai_camera_plus";

// PWM
const std::string path_pwm = "/sys/devices/platform/soc/10048000.pwm/pwm/pwmchip0";
const int pwmPeriod = 200000;                   // 5 kHz

// DI
int DI_GPIOs[NUM_DI] = {digp_1, digp_2};        // DI GPIO
std::thread t_aicamera_monitorDI;
bool isMonitorDI = false;
GPIO_LEVEl DI_gpio_level_last[NUM_DI] = {gpiol_unknown, gpiol_unknown};
GPIO_LEVEl DI_gpio_level_new[NUM_DI] = {gpiol_unknown, gpiol_unknown};
uint64_t DI_last_event_time[NUM_DI] = {0};

// Triger
int Triger_GPIOs[NUM_Triger] = {tgp_1, tgp_2};  // Triger GPIO
std::thread t_aicamera_monitorTriger;
bool isMonitorTriger = false;
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
bool isMonitorDIO[NUM_DIO] = {false};

void FW_getProduct() {
  product = exec_command("fw_printenv | grep '^product=' | cut -d '=' -f2");
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
  // actual is pwm0 & pwm1
  // Map logical index to actual hardware index
  std::string actualPwmIndex;
  if (pwmIndex == "1") {
    actualPwmIndex = "0";
  } else if (pwmIndex == "2") {
    actualPwmIndex = "1";
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
    usleep(500000);  // sleep 0.5s
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

void FW_setGPIO(int gpio_num, int value) {
  // xlog("gpiod version:%s", gpiod_version_string());
  xlog("gpio:%d, value:%d", gpio_num, value);

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

void FW_setLED(string led_index, string led_color) {
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
  } else if (led_index == "4") {
    gpio_index1 = ledgp_4_red;
    gpio_index2 = ledgp_4_green;  
  } else if (led_index == "5") {
    gpio_index1 = ledgp_5_red;
    gpio_index2 = ledgp_5_green;  
  }

  if (isSameString(led_color, "red")) {
    FW_setGPIO(gpio_index1, 1);
    FW_setGPIO(gpio_index2, 0);
  } else if (isSameString(led_color, "green")) {
    FW_setGPIO(gpio_index1, 0);
    FW_setGPIO(gpio_index2, 1);;
  } else if (isSameString(led_color, "orange")) {
    FW_setGPIO(gpio_index1, 1);
    FW_setGPIO(gpio_index2, 1);
  } else if (isSameString(led_color, "off")) {
    FW_setGPIO(gpio_index1, 0);
    FW_setGPIO(gpio_index2, 0);
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
      continue;
    }

    // Get file descriptor for polling
    fds[i].fd = gpiod_line_event_get_fd(lines[i]);
    fds[i].events = POLLIN;

    // get init value & update to App
    sendRESTful_DI(i+1, (gpiod_line_get_value(lines[i]) == 1));
  }

  xlog("^^^^ Start ^^^^");

  // Main loop to monitor GPIOs
  while (isMonitorDI) {
    ret = poll(fds, NUM_DI, -1);  // Wait indefinitely for an event
    if (ret < 0) {
      xlog("Error in poll");
      break;
    }

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

          sendRESTful_DI(i+1, DI_gpio_level_last[i] == gpiol_high);
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
  xlog("");
  if (isMonitorDI) {
    xlog("thread already running");
    return;
  }
  isMonitorDI = true;
  t_aicamera_monitorDI = std::thread(Thread_FWMonitorDI);  
  t_aicamera_monitorDI.detach();
}
void FW_MonitorDIStop() {
  isMonitorDI = false;
}

void Thread_FWMonitorTriger() {
  struct gpiod_chip *chip;
  struct gpiod_line *lines[NUM_Triger];
  struct pollfd fds[NUM_Triger];
  int i, ret;

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
      continue;
    }

    // Get file descriptor for polling
    fds[i].fd = gpiod_line_event_get_fd(lines[i]);
    fds[i].events = POLLIN;

    // get init value & update to App
    sendRESTful_DI(i+1, (gpiod_line_get_value(lines[i]) == 1));
  }

  xlog("^^^^ Start ^^^^");

  // Main loop to monitor GPIOs
  while (isMonitorTriger) {
    ret = poll(fds, NUM_Triger, -1);  // Wait indefinitely for an event
    if (ret < 0) {
      xlog("Error in poll");
      break;
    }

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

          sendRESTful_DI(i+1, (Triger_gpio_level_last[i] == gpiol_high));
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
  xlog("");
  if (isMonitorTriger) {
    xlog("thread already running");
    return;
  }
  isMonitorTriger = true;
  t_aicamera_monitorTriger = std::thread(Thread_FWMonitorTriger);  
  t_aicamera_monitorTriger.detach();
}
void FW_MonitorTrigerStop() {
  isMonitorTriger = false;
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

  if (isSameString(on_off, "on")) {
    isON = true;
  } else if (isSameString(on_off, "off")) {
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

  if (index_dio < 0 || index_dio >= NUM_DIO) {
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

  xlog("Thread Monitoring GPIO %d for events...start", DIO_DI_GPIOs[index_dio]);

  // Main loop to monitor GPIO
  while (isMonitorDIO[index_dio]) {
    ret = poll(&fd, 1, -1);  // Wait indefinitely for an event
    if (ret < 0) {
      xlog("Error in poll");
      break;
    }

    if (fd.revents & POLLIN) {
      struct gpiod_line_event event;
      gpiod_line_event_read(line, &event);

      xlog("GPIO %d event detected! Type: %s", DIO_DI_GPIOs[index_dio],
           (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) ? "RISING" : "FALLING");
    }
  }

  xlog("Thread Monitoring GPIO %d for events...stop", DIO_DI_GPIOs[index_dio]);

  // Cleanup
  gpiod_line_release(line);
  gpiod_chip_close(chip);
}

void FW_MonitorDIOInStart(int index_dio) {
  xlog("");
  if (isMonitorDIO[index_dio]) {
    xlog("thread already running");
    return;
  }
  isMonitorDIO[index_dio] = true;
  t_aicamera_monitorDIO[index_dio] = std::thread(Thread_FWMonitorDIOIn, index_dio);  
  t_aicamera_monitorDIO[index_dio].detach();
}

void FW_MonitorDIOInStop(int index_dio) {
  isMonitorDIO[index_dio] = false;
}

void FW_setDIODirection(string index_dio, string di_do) {
  xlog("index:%s, direction:%s", index_dio.c_str(), di_do.c_str());
  int index_gpio_in = 0;
  int index_gpio_out = 0;

  int index = std::stoi(index_dio);
  if (index > 0 && index <= NUM_DIO) {
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

  if (index > 0 && index <= NUM_DIO) {
    index_gpio = DIO_DO_GPIOs[index - 1];
  } else {
    xlog("index out of range...");
    return;
  }

  if (isSameString(on_off, "on")) {
    isON = true;
  } else if (isSameString(on_off, "off")) {
    isON = false;
  } else {
    xlog("input string should be on or off...");
    return;
  }

  FW_setGPIO(index_gpio, isON ? 1 : 0);
}

