#include "mqttx.hpp"

bool isMQTTRunning = false;

void thread_mqtt_start() {
  if (!isMQTTRunning) {
    std::thread([] {
      xlog("thread_mqtt_start start >>>>");
      isMQTTRunning = true;

      mosqpp::lib_init();
      MQTTClient client("my_client");

      client.connect("localhost", 1883);
      client.subscribe(nullptr, "PX/VBS/Cmd");

      while (isMQTTRunning) {
        client.loop();  // This triggers on_message()
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      mosqpp::lib_cleanup();

      xlog("thread_mqtt_start stop >>>>");
    }).detach();  // Detach to run in the background

  } else {
    xlog("thread_mqtt_start already running...");
  }
}

void thread_mqtt_stop() {
  isMQTTRunning = false;
}

void handle_mqtt(std::string payload) {
  xlog("MQTT payload:%s", payload.c_str());
}

MQTTClient::MQTTClient(const char* id)
    : mosqpp::mosquittopp(id) {}

void MQTTClient::on_message(const struct mosquitto_message* message) {
  std::string payload(static_cast<char*>(message->payload), message->payloadlen);

  handle_mqtt(payload);
}
