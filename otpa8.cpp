#include "otpa8.hpp"

OTPA8::OTPA8() {
  file = open(OTPA8_I2C_BUS, O_RDWR);
  if (file < 0) {
    xlog("Unable to open I2C bus:%s", OTPA8_I2C_BUS);
  } else if (ioctl(file, I2C_SLAVE, OTPA8_I2C_ADDR) < 0) {
    xlog("Failed to set I2C address 0x%x", OTPA8_I2C_ADDR);
    close(file);
    file = -1;
  }
}

OTPA8::~OTPA8() {
  if (file >= 0) {
    close(file);
  }
}
