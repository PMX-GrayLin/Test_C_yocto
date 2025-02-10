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
  xlog("");
  if (file >= 0) {
    close(file);
  }
}

bool OTPA8::readTemperature_avg(float& ambientTemp, float& objectTemp) {
  // Readout command for OTPA-8 (3 bytes: ADR, CMD, NUL)
  //   uint8_t command[3] = {0xD0, 0x4E, 0x00};   // NG
  //   uint8_t command[3] = {0x68, 0x4E, 0x00};   // NG
  //   uint8_t command[2] = {0x4E, 0x00};         // OK
  uint8_t command[1] = {0x4E};

  // Send the readout command
  if (write(file, command, sizeof(command)) != sizeof(command)) {
    xlog("Failed to send readout command");
    return false;
  }

  usleep(10000);

  // Read 141 bytes of response from sensor
  uint8_t buffer[141] = {0};
  if (read(file, buffer, sizeof(buffer)) != sizeof(buffer)) {
    xlog("Failed to read temperature data");
    return false;
  }

  // check
  xlog("read...");
  printBuffer(buffer, 141);

  // Parse ambient temperature (bytes 10-13)
  uint8_t ambHigh = buffer[9];  // Byte 10: AMB_H
  uint8_t ambLow = buffer[10];  // Byte 11: AMB_L
  xlog("buffer[9]:0x%x, buffer[10]:0x%x", buffer[9], buffer[10]);
  int16_t ambientRaw = (ambHigh << 8) | ambLow;
  ambientTemp = (static_cast<float>(ambientRaw) - 27315) / 100.0f;

  // Parse object temperature (bytes 14-141)
  // For simplicity, we'll average all 64 object temperature readings
  float objectSum = 0.0f;
  for (int i = 0; i < 64; ++i) {
    uint8_t objHigh = buffer[13 + 2 * i];  // High byte of pixel i
    uint8_t objLow = buffer[14 + 2 * i];   // Low byte of pixel i
    int16_t objectRaw = (objHigh << 8) | objLow;
    objectSum += (static_cast<float>(objectRaw) - 27315) / 100.0f;
  }
  objectTemp = objectSum / 64.0f;  // Average temperature

  // Log the results
  xlog("ambientTemp: %f, objectTemp: %f", ambientTemp, objectTemp);
  return true;
}

bool OTPA8::readTemperature_max(float& ambientTemp, float& objectTemp) {
  // Readout command for OTPA-8 (3 bytes: ADR, CMD, NUL)
  //   uint8_t command[3] = {0xD0, 0x4E, 0x00};   // NG
  //   uint8_t command[3] = {0x68, 0x4E, 0x00};   // NG
  //   uint8_t command[2] = {0x4E, 0x00};         // OK
  uint8_t command[1] = {0x4E};

  // Send the readout command
  if (write(file, command, sizeof(command)) != sizeof(command)) {
    xlog("Failed to send readout command");
    return false;
  }

  usleep(10000);

  // Read 141 bytes of response from sensor
  uint8_t buffer[141] = {0};
  if (read(file, buffer, sizeof(buffer)) != sizeof(buffer)) {
    xlog("Failed to read temperature data");
    return false;
  }

  // check
  xlog("read...");
  printBuffer(buffer, 141);

  // Parse ambient temperature (bytes 10-13)
  uint8_t ambHigh = buffer[9];  // Byte 10: AMB_H
  uint8_t ambLow = buffer[10];  // Byte 11: AMB_L
  int16_t ambientRaw = (ambHigh << 8) | ambLow;
  ambientTemp = (static_cast<float>(ambientRaw) - 27315) / 100.0f;

  // Parse object temperature (bytes 14-141)
  // find Max
  int16_t objectMax = 0;
  for (int i = 0; i < 64; ++i) {
    uint8_t objHigh = buffer[13 + 2 * i];  // High byte of pixel i
    uint8_t objLow = buffer[14 + 2 * i];   // Low byte of pixel i
    int16_t objectRaw = (objHigh << 8) | objLow;
    if (objectRaw > objectMax) {
      objectMax = objectRaw;
    }
  }
  objectTemp += (static_cast<float>(objectMax) - 27315) / 100.0f;

  // Log the results
  xlog("ambientTemp: %f, objectTemp: %f", ambientTemp, objectTemp);
  return true;
}
