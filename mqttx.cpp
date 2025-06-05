#include "mqttx.hpp"

bool isMQTTRunning = false;

MQTTClient::MQTTClient(const std::string &client_id)
    : mosquittopp(client_id.c_str()) {
  // You can also set options here (e.g., username/password)
}

void MQTTClient::on_connect(int rc) {
  if (rc == 0) {
    xlog("MQTT connected");
    connected = true;
  } else {
    xlog("MQTT connection failed, rc:%d", rc);
  }
}

void MQTTClient::on_disconnect(int rc) {
  xlog("MQTT disconnected, rc:%d", rc);
  connected = false;
}

void MQTTClient::send_message(const std::string &topic, const std::string &message) {
  if (!connected) {
    xlog("not connected...");
    return;
  }

  int ret = publish(nullptr, topic.c_str(), message.length(), message.c_str(), 1, false);
  if (ret != MOSQ_ERR_SUCCESS) {
    xlog("Failed to publish, ret:%d", ret);
  } else {
    xlog("topic:%s, message:%s", topic, message);
  }
}

void MQTTClient::on_message(const struct mosquitto_message *message) {
  std::string payload(static_cast<char *>(message->payload), message->payloadlen);
  xlog("MQTT payload:%s", payload.c_str());
}

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
