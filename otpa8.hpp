
// OST
// THERMOPILE ARRAY
// OTPA-8 series

#pragma once

#include "global.hpp"

#include <linux/i2c-dev.h>

#define OTPA8_I2C_BUS "/dev/i2c-2"
#define OTPA8_I2C_ADDR 0x68


class OTPA8 {
 public:
  OTPA8();
  ~OTPA8();

  bool readTemperature_avg(float &ambientTemp, float &objectTemp);
  bool readTemperature_max(float &ambientTemp, float &objectTemp);

  void startReading();  // Start the periodic reading thread
  void stopReading();   // Stop the thread

  float getMultipler(float readSensorTemp);

 private:
  int file;
  std::thread readThread;
  bool isStopThread;
  void readTemperatureLoop();  // Thread function
};
