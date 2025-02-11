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
  printf("len:%d: ", len);
  for (size_t i = 0; i < len; i++) {
    printf("0x%02X ", buffer[i]);
  }
  printf("\n\r");
}

void printArray_float(const float* buffer, size_t len) {
  printf("len:%d: ", len);
  for (size_t i = 0; i < len; i++) {
    printf("%.3f ", buffer[i]);
  }
  printf("\n\r");
}
