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
      while (isTimerRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        // counterTimer++;
        // isSave2Jpeg = true;

        OTI322 oti322;
        float ambientTemp = 0.0;
        float objectTemp = 0.0;
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
