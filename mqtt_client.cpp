
#include "mqtt_client.hpp"

#include "iosCtl/IOS_CompFunction.hpp"
#include "ipsCtl/IPS_CompAlgorithm.hpp"
#include "ipsCtl/IPS_CompFunction.hpp"
#include "mainCtl.hpp"

#if defined(AICAMERAG2)
static const char *publish_topic[] = {"PX/VBS/Resp", "PX/VBS/Resp/Cam2"};
static const char *subscribe_topic[] = {"PX/VBS/Cmd", "PX/VBS/Cmd/Cam2"};
#else
static const char *publish_topic[] = {"PX/VBS/Resp/Cam1", "PX/VBS/Resp/Cam2"};
static const char *subscribe_topic[] = {"PX/VBS/Cmd/Cam1", "PX/VBS/Cmd/Cam2"};
#endif  // AICAMERAG2

static struct mosquitto *mqtt_instance = nullptr;
static struct mosq_config mqtt_cfg = {};
extern unsigned char ios_cameraid;
extern char trig_DinMode;
extern char trig_DinMode_Dual;

pthread_mutex_t mutex_mqtt = PTHREAD_MUTEX_INITIALIZER;

int json_parse_IOS(struct json_object *root, char *setFunc, const int iID) {
  xlog("");
  JsonInfoIO jsonInfo = {};

  struct json_object *j_subsystem;
  int ret = -1;

  // look for cmd
  j_subsystem = (struct json_object *)json_object_object_get(root, "cmd");
  if (j_subsystem) {
    std::string jsonString = json_object_get_string(j_subsystem);
    ret = searchHandleFunction_IOS(jsonString.c_str(), (struct json_object *)root, &jsonInfo);

    if (ret >= 0) {
      JsonQ_EnQ_IOS(jsonInfo);
    }
  } else {
    xlog("json_object_object_get fail");
    ret = -1;
  }

  return ret;
}

int json_parse_IPS(struct json_object *root, char *setFunc, const int iID) {
  xlog("");
  JsonInfo jsonInfo = {};

  struct json_object *j_subsystem;
  int ret = -1;

  // look for cmd
  j_subsystem = (struct json_object *)json_object_object_get(root, "cmd");
  if (j_subsystem) {
    std::string jsonString = json_object_get_string(j_subsystem);
    ret = searchHandleFunction_IPSDual(jsonString.c_str(), (struct json_object *)root, &jsonInfo, iID);
    if (ret >= 0) {
      JsonQ_EnQ_IPS(jsonInfo, iID);
    }
  } else {
    xlog("json_object_object_get fail");
    ret = -1;
  }
  return ret;
}

int mqtt_parse_json(const char *payload, char *setFunc, const int iID) {
  struct json_object *root;
  struct json_object *j_subsystem;
  JsonInfo jsonInfo = {} ;

  if (setFunc == nullptr)
    return -1;

  root = (struct json_object *)json_tokener_parse((const char *)payload);
  struct json_object *tmp_obj_array = nullptr;
  const char *mqtt_cmd_header[] = {"CMD_START", "ROI_", "CMD_END"};
  const char *mqtt_type_header[] = {"Normal", "ARRAY"};
  int iSetlection = mht_normal;
  tmp_obj_array = json_object_object_get(root, mqtt_type_header[1]);

  // Selection the MQTT type. Identity is 0:Noraml, 1:Array < AutoRun Mode >
  if (!tmp_obj_array) {
    iSetlection = mht_normal;
  } else {
    iSetlection = mht_array;
  }

  if (mht_normal == iSetlection) {
    
    j_subsystem = (struct json_object *)json_object_object_get(root, "cmd");
    if (j_subsystem) {
      strcpy(setFunc, json_object_get_string(j_subsystem));
      xlog("setFunc:%s", setFunc);

      if (j_subsystem != nullptr) {
        int ret = 0;

        // look for handle function of IOS
        ret = json_parse_IOS(root, setFunc, iID);
        if (ret != 0) {
          // look for handle function of IPS
          ret = json_parse_IPS(root, setFunc, iID);
        }

        std::string str(reinterpret_cast<char *>(setFunc));
        InnerQ_EnQ_Main(str);
      }
    }

  } else if (mht_array == iSetlection) {
    int array_mqttlength = json_object_array_length(tmp_obj_array);
    xlog("ARRAY size:%d", array_mqttlength);

    // get each item from the array.
    for (int i = 0; i < array_mqttlength; i++) {
      std::string strKey;
      if (i == 0 || i == (array_mqttlength - 1)) {  // CMD_START, CMD_END
        if (i == 0) {
          strKey = mqtt_cmd_header[0];  // CMD_START
        } else {
          strKey = mqtt_cmd_header[2];  // CMD_END
        }
      } else {  // ROI_x [x:1 ~ n]
        string strTmp = mqtt_cmd_header[1];
        strKey = strTmp + std::to_string(i);
      }

      xlog("strKey:%s", strKey.c_str());

      struct json_object *item_obj_id = json_object_array_get_idx(tmp_obj_array, i);
      struct json_object *tmp_obj_item = (struct json_object *)json_object_object_get(item_obj_id, strKey.c_str());

      if (!tmp_obj_item) {
        xlog("ignore...");
        break;
      }

      int item_mqttlength = json_object_array_length(tmp_obj_item);
      xlog("item_mqttlength:%d", item_mqttlength);

      for (int x = 0; x < item_mqttlength; x++) {
        std::string strElementKey = "cmd";

        struct json_object *elem_obj_id = json_object_array_get_idx(tmp_obj_item, x);
        struct json_object *tmp_obj_element = (struct json_object *)json_object_object_get(elem_obj_id, strElementKey.c_str());

        if (tmp_obj_element != nullptr) {
          std::string jsonString = json_object_get_string(tmp_obj_element);
          xlog("strKey.c_str():%s, json_object_get_string(tmp_obj_element):%s", strKey.c_str(), json_object_get_string(tmp_obj_element));

          searchHandleFunction_IPSDual(jsonString.c_str(), (struct json_object *)elem_obj_id, &jsonInfo, iID);

          memcpy(setFunc, jsonInfo.szCmd, strlen(jsonInfo.szCmd));

          JsonQ_EnQ_IPS(jsonInfo, iID);
          InnerQ_EnQ_Main(jsonInfo.szCmd);

        } else {
          xlog("ignore...");
        }
      }
    }
  } else {
    xlog("not match...");
  }

  json_object_put(root);
  return 0;
}

static char setFunc[128] = "";
void mqtt_message_callback(struct mosquitto *mqtt, void *obj, const struct mosquitto_message *message) {
  struct mosq_config *mqcfg;
  int32_t i;
  bool res;

  if (obj == nullptr) {
    xlog("obj null");
    return;
  }
  mqcfg = (struct mosq_config *)obj;

  if (message->retain && mqcfg->no_retain)
    return;
  if (message->payloadlen == 0)
    return;
  if (mqcfg->filter_outs) {
    for (i = 0; i < mqcfg->filter_out_count; i++) {
      mosquitto_topic_matches_sub(mqcfg->filter_outs[i], message->topic, &res);
      if (res) {
        xlog("filter_outs, message->topic:%s", message->topic);
      }
    }
  } else {
    xlog("topic:%s, payloadlen:%d, payload:%s", message->topic, message->payloadlen, (char*)message->payload);

    if (message->payloadlen == (int)strlen((const char *)message->payload)) {
      memset(setFunc, '\0', sizeof(setFunc));

      // if (!strcmp(message->topic, subscribe_topic[0])) {
      //   ios_cameraid = 0;
      // } else if (!strcmp(message->topic, subscribe_topic[1])) {
      //   ios_cameraid = 1;
      // }

      if (strcmp(message->topic, subscribe_topic[0]) == 0) {
        ios_cameraid = 0;
        if (mqtt_parse_json((char*)message->payload, setFunc, 0) != 0) {
          xlog("mqtt_parse_json error");
          sprintf((char *)setFunc, "%s", "MQTT_PARSER_ERROR");
        }
      } else if (strcmp(message->topic, subscribe_topic[1]) == 0) {
        ios_cameraid = 0;
        if (mqtt_parse_json((char*)message->payload, setFunc, 1) != 0) {
          xlog("mqtt_parse_json error");
          sprintf((char *)setFunc, "%s", "MQTT_PARSER_ERROR");
        }
      }

    } else {
      xlog("MQTT payload length error...");
    }
  }
}

static int32_t mid;
void mqtt_connect_callback(struct mosquitto *mqtt, void *obj, int32_t result) {
  struct mosq_config *mqcfg;
  int32_t i;

  if (obj == nullptr) {
    xlog("obj null");
    return;
  }
  mqcfg = (struct mosq_config *)obj;

  for (i = 0; i < mqcfg->topic_count; i++) {
    xlog("mosquitto_subscribe topics:%s", mqcfg->topics_sub[i]);
    mosquitto_subscribe(mqtt, &mid, mqcfg->topics_sub[i], mqcfg->qos);
  }
}

void mqtt_config(struct mosq_config *mqttConfig) {
  mqttConfig->host = "localhost";
  mqttConfig->port = 1883;
  mqttConfig->debug = false;
  mqttConfig->max_inflight = 20;
  mqttConfig->keepalive = 60;
  mqttConfig->clean_session = true;
  mqttConfig->eol = true;
  mqttConfig->qos = 0;
  mqttConfig->retain = 0;
  mqttConfig->protocol_version = MQTT_PROTOCOL_V311;

  /* for subscription */
  mqttConfig->topic_count = 1;
  mqttConfig->topics_sub = (char **)realloc(mqttConfig->topics_sub, mqttConfig->topic_count * sizeof(char *));
  mqttConfig->topics_sub[0] = strdup(subscribe_topic[0]);
  mqttConfig->topics_sub[1] = strdup(subscribe_topic[1]);
  
  /* for publishing */
  mqttConfig->topics_pub = (char **)realloc(mqttConfig->topics_pub, mqttConfig->topic_count * sizeof(char *));
  mqttConfig->topics_pub[0] = strdup(publish_topic[0]);
  mqttConfig->topics_pub[1] = strdup(publish_topic[1]);
}

void mqtt_disconnect() {
  mosquitto_disconnect(mqtt_instance);
}

int32_t mqtt_gen_clientID(struct mosq_config *mqttConfig, const char *id_base) {
  int32_t len;
  char hostname[256];

  if (mqttConfig->id_prefix) {
    mqttConfig->id = (char *)malloc(strlen(mqttConfig->id_prefix) + 10);
    if (!mqttConfig->id) {
      if (!mqttConfig->quiet) {
        xlog("error... Out of memory");
      }
      mosquitto_lib_cleanup();
      return 1;
    }
    snprintf(mqttConfig->id, strlen(mqttConfig->id_prefix) + 10, "%s%d", mqttConfig->id_prefix, getpid());
  } else if (!mqttConfig->id) {
    hostname[0] = '\0';
    gethostname(hostname, 256);
    hostname[255] = '\0';
    len = strlen(id_base) + strlen("/-") + 6 + strlen(hostname);
    mqttConfig->id = (char *)malloc(len);
    if (!mqttConfig->id) {
      if (!mqttConfig->quiet) {
        xlog("error... Out of memory");
      }
      mosquitto_lib_cleanup();
      return 1;
    }
    snprintf(mqttConfig->id, len, "%s/%d-%s", id_base, getpid(), hostname);
    if (strlen(mqttConfig->id) > MOSQ_MQTT_ID_MAX_LENGTH) {
      mqttConfig->id[MOSQ_MQTT_ID_MAX_LENGTH] = '\0';
    }
  }
  return MOSQ_ERR_SUCCESS;
}

int32_t Thread_MQTT() {
  int32_t rc;
  xlog("$$$$ Start $$$$");

  // init
  mqtt_config(&mqtt_cfg);
  mosquitto_lib_init();

  if (mqtt_gen_clientID(&mqtt_cfg, "vbox_id_")) {
    xlog("mqtt_gen_clientID fail...");
    return 1;
  }

  mqtt_instance = mosquitto_new(mqtt_cfg.id, mqtt_cfg.clean_session, &mqtt_cfg);
  if (!mqtt_instance) {
    xlog("fail to init MQTT client");
    return -1;
    // switch (errno) {
    //   case ENOMEM:
    //     if (!mqtt_cfg.quiet)
    //       xlog("Error: Out of memory");
    //     break;
    //   case EINVAL:
    //     if (!mqtt_cfg.quiet)
    //       xlog("Error: Invalid id and/or clean_session");
    //     break;
    // }
    // mosquitto_lib_cleanup();
    // xlog("mosquitto_new fail...");
  }

  mosquitto_connect_callback_set(mqtt_instance, mqtt_connect_callback);
  mosquitto_message_callback_set(mqtt_instance, mqtt_message_callback);
  rc = mosquitto_connect(mqtt_instance, mqtt_cfg.host, mqtt_cfg.port, mqtt_cfg.keepalive);
  if (rc) {
    xlog("mosquitto_connect fail...");
    return rc;
  }

  rc = mosquitto_loop_forever(mqtt_instance, -1, 1);
  xlog("after mosquitto_loop_forever <<<<");

  mosquitto_destroy(mqtt_instance);
  mosquitto_lib_cleanup();

  if (mqtt_cfg.msg_count > 0 && rc == MOSQ_ERR_NO_CONN) {
    rc = 0;
  }
  if (rc) {
    xlog("error:%s", mosquitto_strerror(rc));
  }

  xlog("$$$$ Stop $$$$");
  return rc;
}

int32_t mqtt_publish(char *jString, const bool bCameID) {
  pthread_mutex_lock(&mutex_mqtt);
  std::string strText(jString);

  int ret = 0;
  xlog("topic:%s, json:%s", mqtt_cfg.topics_pub[bCameID], jString);

  ret = mosquitto_publish(mqtt_instance, &mid, mqtt_cfg.topics_pub[bCameID], strlen(jString), jString, mqtt_cfg.qos, mqtt_cfg.retain);
  if (ret) {
    xlog("mosquitto_publish fail");
  }
  pthread_mutex_unlock(&mutex_mqtt);
  return ret;
}

