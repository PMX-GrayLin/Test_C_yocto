#pragma once

#include "global.hpp"
#include <mosquittopp.h>


class MQTTClient : public mosqpp::mosquittopp {
 public:
  MQTTClient(const std::string& client_id);
  void on_connect(int rc) override;
  void on_disconnect(int rc) override;
  void send_message(const std::string& topic, const std::string& message);
  void on_message(const struct mosquitto_message* message) override;

  // Static helper function
  static void send_message_static(const std::string& topic, const std::string& message);

 private:
  bool connected = false;
  static MQTTClient* instance;
};

void mqtt_start();
void mqtt_stop();
void mqtt_send(string topic, string message);
