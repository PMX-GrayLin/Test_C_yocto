#pragma once

#include "global.hpp"

// RESTful
void handle_RESTful(std::vector<std::string> segments);

class MQTTClient : public mosqpp::mosquittopp {
 public:
  explicit MQTTClient(const char* id);

 protected:
  void on_message(const struct mosquitto_message* message) override;
};

// mqtt
void thread_mqtt_start();
void thread_mqtt_stop();
void handle_mqtt(std::string payload);


