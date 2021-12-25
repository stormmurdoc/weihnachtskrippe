/* mosquitto related stuff */

#define CONFIG_MQTT_PAYLOAD_ON "on"
#define CONFIG_MQTT_PAYLOAD_OFF "off"
#define CONFIG_MQTT_TOPIC_GET "/get"
#define CONFIG_MQTT_TOPIC_SET "/set"
#define CONFIG_MQTT_TOPIC_SET_RESET "/reset"
#define CONFIG_MQTT_TOPIC_SET_UPDATE "/update"
#define CONFIG_MQTT_TOPIC_SET_PING "/ping"
#define CONFIG_MQTT_TOPIC_SET_PONG "/pong"
#define CONFIG_MQTT_TOPIC_STATUS "/status"
#define CONFIG_MQTT_TOPIC_STATUS_ONLINE "/online"
#define CONFIG_MQTT_TOPIC_STATUS_HARDWARE "/hardware"
#define CONFIG_MQTT_TOPIC_STATUS_VERSION "/version"
#define CONFIG_MQTT_TOPIC_STATUS_INTERVAL "/statsinterval"
#define CONFIG_MQTT_TOPIC_STATUS_IP "/ip"
#define CONFIG_MQTT_TOPIC_STATUS_MAC "/mac"
#define CONFIG_MQTT_TOPIC_STATUS_UPTIME "/uptime"
#define CONFIG_MQTT_TOPIC_STATUS_SIGNAL "/rssi"
#define CONFIG_MQTT_TOPIC_STATUS_FIRESIMULATION "/firesimulation"
#define CONFIG_MQTT_TOPIC_STATUS_LAMPS "/lamps"
#define CONFIG_MQTT_TOPIC_STATUS_STAR "/star"




class PubSubClientWrapper : public PubSubClient{
  private:
  public:
    PubSubClientWrapper(Client& espc);
    bool publish(StringSumHelper topic, String str);
    bool publish(StringSumHelper topic, unsigned int num);
    bool publish(const char* topic, String str);
    bool publish(const char* topic, unsigned int num);

    bool publish(StringSumHelper topic, String str, bool retain);
    bool publish(StringSumHelper topic, unsigned int num, bool retain);
    bool publish(const char* topic, String str, bool retain);
    bool publish(const char* topic, unsigned int num, bool retain);
};

PubSubClientWrapper::PubSubClientWrapper(Client& espc) : PubSubClient(espc){

}

bool PubSubClientWrapper::publish(StringSumHelper topic, String str) {
  return publish(topic.c_str(), str);
}

bool PubSubClientWrapper::publish(StringSumHelper topic, unsigned int num) {
  return publish(topic.c_str(), num);
}

bool PubSubClientWrapper::publish(const char* topic, String str) {
  return publish(topic, str, false);
}

bool PubSubClientWrapper::publish(const char* topic, unsigned int num) {
  return publish(topic, num, false);
}

bool PubSubClientWrapper::publish(StringSumHelper topic, String str, bool retain) {
  return publish(topic.c_str(), str, retain);
}

bool PubSubClientWrapper::publish(StringSumHelper topic, unsigned int num, bool retain) {
  return publish(topic.c_str(), num, retain);
}

bool PubSubClientWrapper::publish(const char* topic, String str, bool retain) {
  char buf[128];

  if(str.length() >= 128) return false;

  str.toCharArray(buf, 128);
  return PubSubClient::publish(topic, buf, retain);
}

bool PubSubClientWrapper::publish(const char* topic, unsigned int num, bool retain) {
  char buf[6];

  dtostrf(num, 0, 0, buf);
  return PubSubClient::publish(topic, buf, retain);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  String topicStr = topic;
  String check = ((String)mqtt_root + CONFIG_MQTT_TOPIC_SET);

  if(topicStr == check) {
    Serial.println("MQTT SET!");
    Serial.flush();
    String strMsg = (String)message;
    /*if((strMsg.startsWith(CONFIG_MQTT_PAYLOAD_ON)) || (strMsg.startsWith(CONFIG_MQTT_PAYLOAD_OFF))) {
        cmdSwitch((strMsg.startsWith(CONFIG_MQTT_PAYLOAD_ON)) ? true : false);
    }*/
  }

   if(topicStr.startsWith((String)check + "/led01")){
    String strMsg = (String)message;
    Serial.println("LED01 SET!" + strMsg);

    if (strMsg == "on" ){
      fire_simulation=true;
      lamps=true;
    }
    else {
        fire_simulation=false;
        lamps=false;
    }
    client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_GET + "/led01" ), (fire_simulation ? CONFIG_MQTT_PAYLOAD_ON : CONFIG_MQTT_PAYLOAD_OFF), true);
      Serial.flush();
   }

   if(topicStr.startsWith((String)check + "/led05")){
    String strMsg = (String)message;
    Serial.println("LED05 SET: " + strMsg);
    if (strMsg == "on" ){
      lamps=true;
    }
    else {
      lamps=false;
    }
    client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_GET + "/led05" ), (lamps ? CONFIG_MQTT_PAYLOAD_ON : CONFIG_MQTT_PAYLOAD_OFF), true);
    Serial.flush();
   }

   if(topicStr.startsWith((String)check + "/led07")){
    String strMsg = (String)message;
    Serial.println("LED07 SET: " + strMsg);
    if (strMsg == "on" ){
      star=true;
    }
    else {
      star=false;
    }
    client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_GET + "/led07" ), (lamps ? CONFIG_MQTT_PAYLOAD_ON : CONFIG_MQTT_PAYLOAD_OFF), true);
    Serial.flush();
   }


  if(topicStr.startsWith((String)check + CONFIG_MQTT_TOPIC_SET_RESET)) {
    Serial.println("MQTT RESET!");
    Serial.flush();
    ESP.restart();
  }

  if(topicStr.startsWith((String)check + CONFIG_MQTT_TOPIC_SET_PING)) {
    Serial.println("PING");
    client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_GET + CONFIG_MQTT_TOPIC_SET_PONG), message, false);
    return;
  }

  if(topicStr.startsWith((String)check + CONFIG_MQTT_TOPIC_SET_UPDATE)) {
    Serial.println("OTA REQUESTED!");
    Serial.flush();
    ArduinoOTA.begin();

    unsigned long timeout = millis() + (120*1000); // Max 2 minutes
    os_timer_disarm(&Timer1);

    while(true) {
      yield();
      ArduinoOTA.handle();
      if(millis() > timeout) break;
    }

    os_timer_arm(&Timer1, stats_interval*1000, true);
    return;
  }

  return;
}
