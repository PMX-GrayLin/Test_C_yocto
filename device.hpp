#pragma once

#include "global.hpp"

#define GPIO_CHIP       "/dev/gpiochip0"
#define NUM_DI          2                 // Number of DI
#define NUM_Triger      2                 // Number of Triger, treat as DI
#define NUM_DO          2                 // Number of DO

// ? NUM_DIOm aicamera = 2, visionhb = 4
#define NUM_DIO         4                 // Number of DIO


typedef enum {
  lc_red,
  lc_green,
  lc_orange,
  lc_off,
} LEDColor;

typedef enum {
  ledgp_1_red = 79,
  ledgp_1_green = 80,
  ledgp_2_red = 81,
  ledgp_2_green = 82,
  ledgp_3_red = 114,
  ledgp_3_green = 115,
  ledgp_4_red = 116,
  ledgp_4_green = 117,
  ledgp_5_red = 119,
  ledgp_5_green = 120,
} LED_GPIO_PIN;

typedef enum {
  digp_1 = 0,
  digp_2 = 1,
} DI_GPIO_PIN;

typedef enum {
  tgp_1 = 17,
  tgp_2 = 70,
} Triger_GPIO_PIN;

typedef enum {
  dogp_1 = 3,
  dogp_2 = 7,
} DO_GPIO_PIN;

typedef enum {
  diod_in,
  diod_out,
} DIO_Direction;

typedef enum {
  diodigp_1 = 2,
  diodigp_2 = 6,
  diodigp_3 = 12,
  diodigp_4 = 13,
} DIO_DI_GPIO_PIN;
typedef enum {
  diodogp_1 = 8,
  diodogp_2 = 9,
  diodogp_3 = 11,
  diodogp_4 = 5,
} DIO_DO_GPIO_PIN;

typedef enum {
    gpiol_unknown,
    gpiol_low,
    gpiol_high
} GPIO_LEVEl;

extern std::string product;
extern std::string hostname_prefix;

void FW_getProduct();
void FW_getHostnamePrefix();
void FW_getDeviceIndo();

bool FW_isDeviceAICamera();
bool FW_isDeviceVisionHub();

// PWM 
void FW_writePWMFile(const std::string &path, const std::string &value);
extern void FW_setPWM(const std::string &pwmIndex, const std::string &sPercent);

// GPIO ops
int FW_getGPIO(int gpio_num);
void FW_setGPIO(int gpio_num, int value);
void FW_toggleGPIO(int gpio_num);

// led
void FW_setLED(string led_index, string led_color);
void FW_toggleLED(string led_index, string led_color);
void FW_setCameraLED();

// DI
void Thread_FWMonitorDI();
extern void FW_MonitorDIStart();
extern void FW_MonitorDIStop();

// Triger
void Thread_FWMonitorTriger();
extern void FW_MonitorTrigerStart();
extern void FW_MonitorTrigerStop();

// DO
extern void FW_setDO(string index_do, string on_off);

// DIO
void Thread_FWMonitorDIOIn(int index_dio);
extern void FW_MonitorDIOInStart(int index_dio);
extern void FW_MonitorDIOInStop(int index_dio);
extern void FW_setDIODirection(string index_dio, string di_do);
extern void FW_setDIOOut(string index_dio, string on_off);

