#pragma once

#include "global.hpp"

// RESTful
void handle_RESTful(std::vector<std::string> segments);

// mqtt
void thread_mqtt_start();
void thread_mqtt_stop();
void handle_mqtt(std::string payload);


