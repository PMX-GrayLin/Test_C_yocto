
// OST
// THERMOPILE ARRAY
// OTPA-8 series

#pragma once

#include "global.hpp"

#include <linux/i2c-dev.h>

#define OTPA8_I2C_BUS "/dev/i2c-2"
#define OTPA8_I2C_ADDR 0x68

float temperatureSensor[96] = { 
  18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 
  18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 
  18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 
  18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 18.4, 
  21.7, 22.3, 22.7, 24.5, 24.5, 25.5, 26.5, 27.0,
  28.0, 28.8, 29.3, 31.0, 32.0, 32.5, 33.3, 34.8,
  36.5, 37.0, 38.0, 38.8, 40.3, 40.7, 41.0, 43.0,
  43.2, 43.5, 45.0, 46.0, 47.0, 47.2, 47.5, 49.0,
  50.0, 52.0, 52.5, 53.0, 54.0, 55.0, 56.0, 57.0,
  57.5, 58.0, 59.0, 59.5, 60.0, 61.0, 62.0, 62.3,
  62.6, 63.0, 63.0, 63.0, 63.0, 63.0, 63.0, 63.0, 
  63.0, 63.0, 63.0, 63.0, 63.0, 63.0, 63.0, 63.0
 };

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
