#include "device.hpp"

#include <fstream>
#include <gpiod.h>

// PWM
const std::string path_pwm = "/sys/devices/platform/soc/10048000.pwm/pwm/pwmchip0";
const std::string pwmTarget = path_pwm + "/pwm1";
const std::string path_pwmExport = path_pwm + "/export";
const int pwmPeriod = 200000;                   // 5 kHz

// DI
int DI_GPIOs[NUM_DI] = {digp_1, digp_2};        // DI GPIO
std::thread t_aicamera_monitorDI;
bool isMonitorDI = false;

// Triger
int Triger_GPIOs[NUM_Triger] = {tgp_1, tgp_1};  // Triger GPIO
std::thread t_aicamera_monitorTriger;
bool isMonitorTriger = false;

// DO
int DO_GPIOs[NUM_DO] = {dogp_1, dogp_2};        // DO GPIO

// DIO
int DIO_DI_GPIOs[NUM_DIO] = {diodigp_1, diodigp_2, diodigp_3, diodigp_4};       // DI GPIO
int DIO_DO_GPIOs[NUM_DIO] = {diodogp_1, diodogp_2, diodogp_3, diodogp_4};       // DO GPIO
DIO_Direction dioDirection[NUM_DIO] = {diod_in};
std::thread t_aicamera_monitorDIO[NUM_DIO];
bool isMonitorDIO[NUM_DIO] = {false};


void AICamera_writePWMFile(const std::string &path, const std::string &value) {
  std::ofstream fs(path);
  if (fs) {
      fs << value;
      fs.close();
  } else {
    xlog("Failed to write to %s", path.c_str());
  }
}

// void AICamera_setPWM(string sPercent) {
//   if (!isPathExist(pwmTarget.c_str())) {
//     xlog("PWM init...");
//     AICamera_writePWMFile(path_pwmExport, "1");
//     usleep(500000);  // sleep 0.5s
//     AICamera_writePWMFile(pwmTarget + "/period", std::to_string(pwmPeriod));
//   }

//   int percent = std::stoi(sPercent);
//   if (percent != 0)
//   {
//     int duty_cycle = pwmPeriod * percent / 100;
//     AICamera_writePWMFile(pwmTarget + "/duty_cycle", std::to_string(duty_cycle));
//     AICamera_writePWMFile(pwmTarget + "/enable", "1");
//   } else {
//     AICamera_writePWMFile(pwmTarget + "/enable", "0");
//   }
// }

void AICamera_setPWM(const std::string &pwmIndex, const std::string &sPercent) {
  std::string pwmTarget = path_pwm + "/pwm" + pwmIndex;
  std::string path_pwmExport = path_pwm + "/export";

  // Export the PWM channel if not already present
  if (!isPathExist(pwmTarget)) {
    xlog("PWM init... pwm" + pwmIndex);
    AICamera_writePWMFile(path_pwmExport, pwmIndex);
    usleep(500000);  // sleep 0.5s
    AICamera_writePWMFile(pwmTarget + "/period", std::to_string(pwmPeriod));
  }

  int percent = std::stoi(sPercent);
  if (percent != 0) {
    int duty_cycle = pwmPeriod * percent / 100;
    AICamera_writePWMFile(pwmTarget + "/duty_cycle", std::to_string(duty_cycle));
    AICamera_writePWMFile(pwmTarget + "/enable", "1");
  } else {
    AICamera_writePWMFile(pwmTarget + "/enable", "0");
  }
}

void AICamera_setGPIO(int gpio_num, int value) {
  // xlog("gpiod version:%s", gpiod_version_string());

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

void AICamera_setLED(string led_index, string led_color) {
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

  if (isSameString(led_color.c_str(), "red")) {
    AICamera_setGPIO(gpio_index1, 1);
    AICamera_setGPIO(gpio_index2, 0);
  } else if (isSameString(led_color.c_str(), "green")) {
    AICamera_setGPIO(gpio_index1, 0);
    AICamera_setGPIO(gpio_index2, 1);;
  } else if (isSameString(led_color.c_str(), "orange")) {
    AICamera_setGPIO(gpio_index1, 1);
    AICamera_setGPIO(gpio_index2, 1);
  } else if (isSameString(led_color.c_str(), "off")) {
    AICamera_setGPIO(gpio_index1, 0);
    AICamera_setGPIO(gpio_index2, 0);
  }
}

void ThreadAICameraMonitorDI() {
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
        gpiod_line_event_read(lines[i], &event);

        xlog("GPIO %d event detected! Type: %s", DI_GPIOs[i],
             (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) ? "rising" : "falling");
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

void AICamera_MonitorDIStart() {
  xlog("");
  if (isMonitorDI) {
    xlog("thread already running");
    return;
  }
  isMonitorDI = true;
  t_aicamera_monitorDI = std::thread(ThreadAICameraMonitorDI);  
  t_aicamera_monitorDI.detach();
}
void AICamera_MonitorDIStop() {
  isMonitorDI = false;
}

void ThreadAICameraMonitorTriger() {
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
        gpiod_line_event_read(lines[i], &event);

        xlog("GPIO %d event detected! Type: %s", Triger_GPIOs[i],
             (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) ? "rising" : "falling");
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

void AICamera_MonitorTrigerStart() {
  xlog("");
  if (isMonitorTriger) {
    xlog("thread already running");
    return;
  }
  isMonitorTriger = true;
  t_aicamera_monitorTriger = std::thread(ThreadAICameraMonitorTriger);  
  t_aicamera_monitorTriger.detach();
}
void AICamera_MonitorTrigerStop() {
  isMonitorTriger = false;
}

void AICamera_setDO(string index_do, string on_off) {
  int index_gpio = 0;
  bool isON = false;

  int index = std::stoi(index_do);
  if (index > 0 && index <= NUM_DO) {
    index_gpio = DO_GPIOs[index - 1];
  } else {
    xlog("index out of range...");
    return;
  }

  if (isSameString(on_off.c_str(), "on")) {
    isON = true;
  } else if (isSameString(on_off.c_str(), "off")) {
    isON = false;
  } else {
    xlog("input string should be on or off...");
    return;
  }

  AICamera_setGPIO(index_gpio, isON ? 1 : 0);
}

void ThreadAICameraMonitorDIOIn(int index_dio) {

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

void AICamera_MonitorDIOInStart(int index_dio) {
  xlog("");
  if (isMonitorDIO[index_dio]) {
    xlog("thread already running");
    return;
  }
  isMonitorDIO[index_dio] = true;
  t_aicamera_monitorDIO[index_dio] = std::thread(ThreadAICameraMonitorDIOIn, index_dio);  
  t_aicamera_monitorDIO[index_dio].detach();
}

void AICamera_MonitorDIOInStop(int index_dio) {
  isMonitorDIO[index_dio] = false;
}

void AICamera_setDIODirection(string index_dio, string di_do) {
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

  if (isSameString(di_do.c_str(), "di")) {
    // set flag
    dioDirection[index - 1] = diod_in;

    // make gpio out low
    AICamera_setGPIO(index_gpio_out, 0);

    // start monitor gpio input
    AICamera_MonitorDIOInStart(index - 1);

  } else if (isSameString(di_do.c_str(), "do")) {
    // set flag
    dioDirection[index - 1] = diod_out;

    // stop monitor gpio input
    AICamera_MonitorDIOInStop(index - 1);

  } else {
    xlog("input string should be di or do...");
    return;
  }
}

void AICamera_setDIOOut(string index_dio, string on_off) {
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

  if (isSameString(on_off.c_str(), "on")) {
    isON = true;
  } else if (isSameString(on_off.c_str(), "off")) {
    isON = false;
  } else {
    xlog("input string should be on or off...");
    return;
  }

  AICamera_setGPIO(index_gpio, isON ? 1 : 0);
}

