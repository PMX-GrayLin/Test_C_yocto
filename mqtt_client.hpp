#pragma once

#include "common.hpp"
#include "global.hpp"

extern int32_t Thread_MQTT();
extern int32_t mqtt_publish(char *jString, const bool bCameID);  // dual camera
extern void mqtt_disconnect();

