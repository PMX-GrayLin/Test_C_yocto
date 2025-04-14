#include "otpa8.hpp"

float temperatureSensor[96] = { 
  14.7, 14.7, 14.7, 14.7, 14.7, 14.7, 14.7, 14.7, 
  14.7, 14.7, 14.7, 14.7, 14.7, 14.7, 14.7, 14.7, 
  14.7, 14.7, 14.7, 14.7, 14.7, 14.7, 14.7, 14.7, 
  14.7, 15.9, 16.8, 17.8, 18.6, 19.8, 20.5, 21.0, 
  21.7, 22.3, 22.7, 24.5, 24.5, 25.5, 26.5, 27.0,
  28.0, 28.8, 29.3, 29.7, 32.0, 33.0, 33.4, 34.7,
  34.8, 35.3, 35.6, 37.0, 37.5, 38.5, 40.2, 40.7,
  41.0, 42.3, 42.4, 43.3, 44.2, 45.4, 46.3, 47.1,
  47.5, 48.2, 48.4, 50.5, 50.8, 51.2, 51.8, 53.0,
  53.3, 54.7, 55.0, 55.5, 56.0, 56.5, 57.0, 59.0,
  59.5, 60.0, 60.0, 60.0, 60.0, 60.0, 60.0, 60.0, 
  60.0, 60.0, 60.0, 60.0, 60.0, 60.0, 60.0, 60.0
 };


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
  //   uint8_t command[1] = {0x4E};               // OK
  uint8_t command[2] = {0x4E, 0x00};            // OK

  // Send the readout command
  if (write(file, command, sizeof(command)) != sizeof(command)) {
    xlog("Failed to send readout command");
    return false;
  }

  // Read 141 bytes of response from sensor
  uint8_t buffer[141] = {0};
  if (read(file, buffer, sizeof(buffer)) != sizeof(buffer)) {
    xlog("Failed to read temperature data");
    return false;
  }

  // check
//   xlog("read...");
  // printBuffer(buffer, 141);

  // Parse ambient temperature (bytes 10-13)
  uint8_t ambHigh = buffer[9];  // Byte 10: AMB_H
  uint8_t ambLow = buffer[10];  // Byte 11: AMB_L
  uint16_t ambientRaw = (ambHigh << 8) | ambLow;
  ambientTemp = (static_cast<float>(ambientRaw) - 27315.0f) / 100.0f;

  printf("\033[3J\033[H\033[2J");
  // Parse object temperature (bytes 14-141)
  // find Max
  float tempArray[64] = { 0.0 };
  float tempMax = 0.0;
  // float multipler = 1.0;

  for (int i = 0; i < 64; ++i) {
    uint8_t objHigh = buffer[13 + 2 * i];  // High byte of pixel i
    uint8_t objLow = buffer[14 + 2 * i];   // Low byte of pixel i
    uint16_t objectRaw = (objHigh << 8) | objLow;

    tempArray[i] = (static_cast<float>(objectRaw) - 27315.0f) / 100.0f;

    if (i % 8 == 0) {
      printf("\n\n");
    }
    printf("%.2f [%02X%02X]\t", tempArray[i], objHigh, objLow);

    if (tempArray[i] > tempMax) {
      tempMax = tempArray[i];
    }
  }

  float multipler = getMultipler(tempMax);
  objectTemp = tempMax * multipler;

  // printf("\n\n");
  // printf("multipler:%.2f", multipler);
  // printf("\n\n");

  // check
  // printArray_float(tempArray, 64);
  // printArray_forUI(tempArray, 64);

  // Log the results
  // printf("ambientTemp: %.2f, maxTemp: %.2f, multipler:%.2f, originTemp:%.2f\n", ambientTemp, objectTemp, multipler, tempMax);
  // xlog("ambientTemp: %.2f, objectTemp: %.2f", ambientTemp, objectTemp);
  return true;
}

bool OTPA8::readTemperature_array(float& ambientTemp, float* objectTemp) {
  // Readout command for OTPA-8 (3 bytes: ADR, CMD, NUL)
  uint8_t command[2] = {0x4E, 0x00};            // OK

  // Send the readout command
  if (write(file, command, sizeof(command)) != sizeof(command)) {
    xlog("Failed to send readout command");
    return false;
  }

  // Read 525 bytes of response from sensor
  uint8_t buffer[525] = {0};
  if (read(file, buffer, sizeof(buffer)) != sizeof(buffer)) {
    xlog("Failed to read temperature data");
    return false;
  }

  // Parse ambient temperature (bytes 10-13)
  uint8_t ambHigh = buffer[9];  // Byte 10: AMB_H
  uint8_t ambLow = buffer[10];  // Byte 11: AMB_L
  uint16_t ambientRaw = (ambHigh << 8) | ambLow;
  ambientTemp = (static_cast<float>(ambientRaw) - 27315.0f) / 100.0f;

  printf("\033[3J\033[H\033[2J");
  
  // Parse object temperature (bytes 14-141)
  float tempArray[256] = { 0.0 };
  float tempMax = 0.0;
  float multipler = 1.0;

  for (int i = 0; i < 256; ++i) {
    uint8_t objHigh = buffer[13 + 2 * i];  // High byte of pixel i
    uint8_t objLow = buffer[14 + 2 * i];   // Low byte of pixel i
    uint16_t objectRaw = (objHigh << 8) | objLow;

    tempArray[i] = (static_cast<float>(objectRaw) - 27315.0f) / 100.0f;

    multipler = getMultipler(tempArray[i]);
    objectTemp[i] = tempArray[i] * multipler;
    
    if (i % 16 == 0) {
      printf("\n\n");
    }
    // printf("%.2f [%02X%02X]\t", objectTemp[i], objHigh, objLow);
    // printf("[%02X%02X]\t", objHigh, objLow);
    printf("%.1f\t", objectTemp[i]);
  }
  
  // printf("\n\n");
  // printf("multipler:%.2f", multipler);
  // printf("\n\n");

  // check
  // printArray_float(tempArray, 64);
  // printArray_forUI(tempArray, 64);

  // Log the results
  // printf("ambientTemp: %.2f, maxTemp: %.2f, multipler:%.2f, originTemp:%.2f\n", ambientTemp, objectTemp, multipler, tempMax);
  // xlog("ambientTemp: %.2f, objectTemp: %.2f", ambientTemp, objectTemp);
  return true;
}

void OTPA8::startReading() {
  isStopThread = false;
  readThread = std::thread(&OTPA8::readTemperatureLoop, this);
}

void OTPA8::stopReading() {
  isStopThread = true;
  if (readThread.joinable()) {
    readThread.join();
  }
}

float OTPA8::getMultipler(float readSensorTemp) {
  int index = 0;

  if (readSensorTemp < temperatureSensor[0]) {
    index = 25;
  } else if (readSensorTemp > temperatureSensor[95]) {
    index = 82;
  } else {
    for (int i = 0; i < 96; i++) {
      if (temperatureSensor[i] < readSensorTemp && readSensorTemp < temperatureSensor[i + 1]) {
        index = i;
        break;
      }
    }
  }

  return (float)index / temperatureSensor[index];
}

void OTPA8::readTemperatureLoop() {
  while (!isStopThread) {
    float ambientTemp, objectTemp;
    if (readTemperature_max(ambientTemp, objectTemp)) {
      // Successfully read temperature, printed inside readTemperature()
    }
    usleep(1000000);  // Sleep for 1 second (1,000,000 microseconds)
  }
}
