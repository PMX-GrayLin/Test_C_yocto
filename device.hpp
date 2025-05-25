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
  diod_in,
  diod_out,
} DIO_Direction;

// PWM
const std::string path_pwm = "/sys/devices/platform/soc/10048000.pwm/pwm/pwmchip0";
const std::string pwmTarget = path_pwm + "/pwm1";
const std::string path_pwmExport = path_pwm + "/export";
const int pwmPeriod = 200000;   // 5 kHz

// DI
int DI_GPIOs[NUM_DI] = {0, 1};  // DI GPIO
std::thread t_aicamera_monitorDI;
bool isMonitorDI = false;

// Triger
int Triger_GPIOs[NUM_Triger] = {17, 70};  // Triger GPIO
std::thread t_aicamera_monitorTriger;
bool isMonitorTriger = false;

// DO
int DO_GPIOs[NUM_DO] = {3, 7};  // DO GPIO

// DIO
int DIO_IN_GPIOs[NUM_DIO] = {2, 6, 12, 13};   // DI GPIO
int DIO_OUT_GPIOs[NUM_DIO] = {8, 9, 11, 5};  // DO GPIO
DIO_Direction dioDirection[NUM_DIO] = {diod_in};
std::thread t_aicamera_monitorDIO[NUM_DIO];
bool isMonitorDIO[NUM_DIO] = {false};

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

