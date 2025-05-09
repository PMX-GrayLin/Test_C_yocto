#include "global.hpp"

#include "oti322.hpp"

// test vars
int testCounter = 0;

bool isSave2Jpeg = false;

bool isTimerRunning = false;
int counterTimer = 0;


void startTimer(int ms) {
  if (!isTimerRunning) {
    isTimerRunning = true;
    counterTimer = 0;

    std::thread([ms]() {
      xlog("timer start >>>>");

      OTI322 oti322;
      float ambientTemp = 0.0;
      float objectTemp = 0.0;

      while (isTimerRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        // counterTimer++;
        // isSave2Jpeg = true;

        oti322.readTemperature(ambientTemp, objectTemp);
      }
      xlog("timer stop >>>>");
    }).detach();  // Detach to run in the background

  } else {
    xlog("timer already running...");
  }
}

void stopTimer() {
    isTimerRunning = false;
}

void printBuffer(const uint8_t* buffer, size_t len) {
  printf("len:%ld: ", len);
  for (size_t i = 0; i < len; i++) {
    printf("0x%02X ", buffer[i]);
  }
  printf("\n\r");
}

void printArray_float(const float* buffer, size_t len) {
  printf("len:%ld: ", len);
  for (size_t i = 0; i < len; i++) {
    printf("%.3f ", buffer[i]);
  }
  printf("\n\r");
}

void printArray_forUI(const float* buffer, size_t len) {
  // printf("len:%d: ", len);
  printf("\033[3J\033[H\033[2J");
  for (size_t i = 0; i < len; i++) {
    if (i % 8 == 0) {
      printf("\n\n");
    }
    printf("%.3f \t", buffer[i]);
  }
  printf("\n\n\r");
}

bool isSameString(const char* s1, const char* s2, bool isCaseSensitive) {
  if (s1 == NULL || s2 == NULL) {
    return false;  // Handle NULL pointers safely
  }

  return isCaseSensitive ? strcmp(s1, s2) == 0 : strcasecmp(s1, s2) == 0;
}

bool isPathExist(const char* path) {
  struct stat buffer;
  return (stat(path, &buffer) == 0);
}

std::string getTimeString() {
  std::time_t now = std::time(nullptr);

  setenv("TZ", "UTC-8", 1);   // Set timezone to UTC+8
  tzset();                    // Apply timezone setting

  std::tm* localTime = std::localtime(&now);

  std::ostringstream oss;
  oss << std::setw(2) << std::setfill('0') << localTime->tm_hour
      << std::setw(2) << std::setfill('0') << localTime->tm_min
      << std::setw(2) << std::setfill('0') << localTime->tm_sec;

  return oss.str();
}

void aravisTest() {
  xlog("");

  arv_update_device_list();

  // Optional: list devices
  guint n_devices = arv_get_n_devices();
  xlog("n_devices:%d", n_devices);
  if (n_devices == 0) {
    xlog("no camera found!");
    return;
  }
}