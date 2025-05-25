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
  ledgp_2_red = 79,
  ledgp_2_green = 81,
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
  diod_in,
  diod_out,
} DIO_Direction;


// PWM 
void AICamera_writePWMFile(const std::string &path, const std::string &value);
extern void AICamera_setPWM(string sPercent);

// led
void AICamera_setGPIO(int gpio_num, int value);
void AICamera_setLED(string led_index, string led_color);

// DI
void ThreadAICameraMonitorDI();
extern void AICamera_MonitorDIStart();
extern void AICamera_MonitorDIStop();

// Triger
void ThreadAICameraMonitorTriger();
extern void AICamera_MonitorTrigerStart();
extern void AICamera_MonitorTrigerStop();

// DO
extern void AICamera_setDO(string index_do, string on_off);

// DIO
void ThreadAICameraMonitorDIOIn(int index_dio);
extern void AICamera_MonitorDIOInStart(int index_dio);
extern void AICamera_MonitorDIOInStop(int index_dio);
extern void AICamera_setDIODirection(string index_dio, string di_do);
extern void AICamera_setDIOOut(string index_dio, string on_off);

