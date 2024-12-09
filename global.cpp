#include "global.hpp"

bool isSave2Jpeg = false;

bool isTimerRunning = false;
int counterTimer = 0;

void startTimer(int ms) {
  if (!isTimerRunning) {
    isTimerRunning = true;
    counterTimer = 0;

    std::thread([ms]() {
      while (isTimerRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        counterTimer++;
        isSave2Jpeg = true;
      }
      xlog("timer stoped...");
    }).detach();  // Detach to run in the background

  } else {
    xlog("timer already running...");
  }
}

void stopTimer() {
    isTimerRunning = false;
}
