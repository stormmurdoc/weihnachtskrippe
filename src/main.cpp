/***************************************************************************
  Wemos D1 - Weihnachtsbeleuchtung für die Krippe

  This script is written by Patrick Kirchhoff (patrick@kirchhoffs.de)
 ***************************************************************************/

/* include stuff */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

//periodic status reports
const unsigned int stats_interval = 60;  // Update statistics every 60 seconds

extern "C" {
  #include "user_interface.h" //os_timer
}

// WiFi Configuration
const char* SSID = "MURDOC1";
const char* SSID_PASS = "geometrie";

// MQTT Configuration
#define USE_MQTT
const char* mqtt_server = "192.168.1.119";
const unsigned int mqtt_port = 1883;
const char* mqtt_user =   "murdoc";
const char* mqtt_pass =   "geometrie";
const char* mqtt_root = "/wohnzimmer/krippe";

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
bool fire_simulation = true;
bool lamps = true;
bool star = true;
   int brightness = 0;    // how bright the LED is
  int fadeAmount = 5;    // how many points to fade the LED by
#define MQJ_HARDWARE "wemos6"
#define MQJ_VERSION  "0.0.3"

/* init some vars */
/*int led01=2;  // D4 -- Builtin LED
int led02=0;  // D3
int led03=4;  // D2
int led04=5;  // D1
int led05=14; // D5
int led06=12; // D6
*/
int led01=0;  // D3
int led02=4;  // D2
int led03=5;  // D1
int led04=14; // D5
int led05=12; // D6
int led06=13; // D7
int led07=15; // D8
int r=0;


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

os_timer_t Timer1;
bool sendStats = true;

//WiFiClientSecure espClient;
WiFiClient espClient;
PubSubClientWrapper client(espClient);

void timerCallback(void *arg) {
  sendStats = true;
}

uint8_t rssiToPercentage(int32_t rssi) {
  //@author Marvin Roger - https://github.com/marvinroger/homie-esp8266/blob/ad876b2cd0aaddc7bc30f1c76bfc22cd815730d9/src/Homie/Utils/Helpers.cpp#L12
  uint8_t quality;
  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }

  return quality;
}

void ipToString(const IPAddress& ip, char * str) {
  //@author Marvin Roger - https://github.com/marvinroger/homie-esp8266/blob/ad876b2cd0aaddc7bc30f1c76bfc22cd815730d9/src/Homie/Utils/Helpers.cpp#L82
  snprintf(str, 16, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void sendStatsBoot(void) {
  client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS_HARDWARE), MQJ_HARDWARE, true);
  client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS_VERSION), MQJ_VERSION, true);
  char buf[5];
  sprintf(buf, "%d", stats_interval);
  client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS_INTERVAL), buf, true);
  client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS_MAC), WiFi.macAddress(), true);
  client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_GET), CONFIG_MQTT_PAYLOAD_OFF, true);
}

void sendStatsInterval(void) {
  char buf[16]; //v4 only atm
  ipToString(WiFi.localIP(), buf);
  client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS_IP), buf);
  client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS_UPTIME), (uint32_t)(millis()/1000));
  client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS_SIGNAL), rssiToPercentage(WiFi.RSSI()));
  client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS_FIRESIMULATION), fire_simulation);
  client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS_LAMPS), lamps);
}

/*
void cmdSwitch(bool state) {
    digitalWrite(RELAY, state);
    digitalWrite(LED, !state);
    relay_state = state;
    Serial.print("Switched output to ");
    Serial.println(state);
    client.publish(((String)mqtt_root + CONFIG_MQTT_TOPIC_GET), (relay_state ? CONFIG_MQTT_PAYLOAD_ON : CONFIG_MQTT_PAYLOAD_OFF), true);
}
*/

/*
void verifyFingerprint() {
  if(client.connected() || espClient.connected()) return; //Already connected

  Serial.print("Checking TLS @ ");
  Serial.print(mqtt_server);
  Serial.print("...");

  if (!espClient.connect(mqtt_server, mqtt_port)) {
    Serial.println("Connection failed. Rebooting.");
    Serial.flush();
    ESP.restart();
  }
  if (espClient.verify(mqtt_fprint, mqtt_server)) {
    Serial.print("Connection secure -> .");
  } else {
    Serial.println("Connection insecure! Rebooting.");
    Serial.flush();
    ESP.restart();
  }

  espClient.stop();

  delay(100);
}
*/

/*
 * Function Randomize with range
 */
unsigned int Randomize(unsigned int min, unsigned int max)
{
  double scaled = (double)rand() / RAND_MAX;
  return (max - min + 1) * scaled + min;
}

/*
 *  Function: Fire Simulation
 */
void fireSimulation(bool state) {

  if (state == true) {
    r = Randomize(10,255);
    analogWrite(led01, r);

    r = Randomize(10,255);
    analogWrite(led02, r);

    r = Randomize(10,255);
    analogWrite(led03, r);

    r = Randomize(10,255);
    analogWrite(led04, r);

    //delay(random(80));
    delay(80);
  }
  else {
    // disable LEDs
    digitalWrite(led01,LOW);
    digitalWrite(led02,LOW);
    digitalWrite(led03,LOW);
    digitalWrite(led04,LOW);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    /*
    Serial.print("Attempting MQTT connection...");
    //verifyFingerprint();
    String clientId = "NurEineID";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if (client.connect(WiFi.macAddress().c_str(), mqtt_user, mqtt_pass, ((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS + CONFIG_MQTT_TOPIC_STATUS_ONLINE).c_str(), 0, 1, "0")) {
    if (client.connect(WiFi.macAddress().c_str())) {
    // if (client.connect(WiFi.macAddress().c_str())) {
      Serial.println("connected");
      client.subscribe(((String)mqtt_root + CONFIG_MQTT_TOPIC_SET + "/#").c_str());
      client.publish((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS + CONFIG_MQTT_TOPIC_STATUS_ONLINE, "1");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      */

    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = MQJ_HARDWARE;
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
      client.subscribe(((String)mqtt_root + CONFIG_MQTT_TOPIC_SET + "/#").c_str());
      client.publish((String)mqtt_root + CONFIG_MQTT_TOPIC_STATUS + CONFIG_MQTT_TOPIC_STATUS_ONLINE, "1");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(5000);
    }
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.mode(WIFI_STA); // Disable the built-in WiFi access point.
  WiFi.begin(SSID, SSID_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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



void setup() {
  // put your setup code here, to run once:
  pinMode(led01, OUTPUT);
  pinMode(led02, OUTPUT);
  pinMode(led03, OUTPUT);
  pinMode(led04, OUTPUT);
  pinMode(led05, OUTPUT);
  pinMode(led06, OUTPUT);
  pinMode(led07, OUTPUT);

   // start serial port
  Serial.begin(115200);
  Serial.println("ESP8266 MQTT start...");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  ArduinoOTA.setHostname(mqtt_user);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  if (!client.connected()) {
    reconnect();
  }

  // Send boot info
  Serial.println(F("Announcing boot..."));
  sendStatsBoot();

  os_timer_setfn(&Timer1, timerCallback, (void *)0);
  os_timer_arm(&Timer1, stats_interval*1000, true);

  digitalWrite(led01,1);
  delay(300);
  digitalWrite(led02,1);
  delay(300);
  digitalWrite(led03,1);
  delay(300);
  digitalWrite(led04,1);
  delay(300);
  digitalWrite(led05,1);
  delay(300);
  digitalWrite(led07,1);
  delay(300);
  digitalWrite(led01,0);
  delay(300);
  digitalWrite(led02,0);
  delay(300);
  digitalWrite(led03,0);
  delay(300);
  digitalWrite(led04,0);
  delay(300);
  digitalWrite(led05,0);
  delay(300);
  digitalWrite(led07,0);
  delay(300);


}
/*
 * Main Loop
 */
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  fireSimulation(fire_simulation);

  if (lamps == true) {

         // set the brightness of pin 9:
    analogWrite(led05, brightness);

    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;

    // reverse the direction of the fading at the ends of the fade:
    if (brightness == 0 || brightness == 255) {
      fadeAmount = -fadeAmount ;
    }
    // wait for 30 milliseconds to see the dimming effect
    delay(30);
  }
  else {
      digitalWrite(led05,LOW);
  }


  if (star == true) {

      digitalWrite(led07,HIGH);
  }
  else {
      digitalWrite(led07,LOW);
  }
  analogWrite(led06, 125);
  if(sendStats) {
    sendStatsInterval();
    sendStats=false;
  }
}


