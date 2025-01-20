// check I2C BUS Speed is 100 kHz

#pragma once

#include "global.hpp"

#include <linux/i2c-dev.h>

#define OTI322_I2C_BUS "/dev/i2c-2"
#define OTI322_I2C_ADDR 0x10

class OTI322 {
 public:
  OTI322();
  ~OTI322();

  bool readTemperature(float &ambientTemp, float &objectTemp);
  float getLastAmbientTemp();
  float getLastObjectTemp();

  void startReading();  // Start the periodic reading thread
  void stopReading();   // Stop the thread

 private:
  int file;
  std::thread readThread;
  bool isStopThread;
  float lastAmbientTemp;
  float lastObjectTemp;
  void readTemperatureLoop();  // Thread function
};