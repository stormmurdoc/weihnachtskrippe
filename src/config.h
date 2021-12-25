/* Config vars for Weihnachtskrippe */

/* WiFi Configuration */
const char* cfg_wifi_ssid = "MURDOC1";
const char* cfg_wifi_password = "geometrie";

/* build vars */
#define MQJ_HARDWARE "weihnachtskrippe"
#define MQJ_VERSION  "0.0.3"


/* MQTT Configuration */
#define USE_MQTT
const char* mqtt_server = "192.168.1.119";
const unsigned int mqtt_port = 1883;
const char* mqtt_user =   "murdoc";
const char* mqtt_pass =   "geometrie";
const char* mqtt_root = "/wohnzimmer/krippe";


/* Update telemetrie statistics every 60 seconds */
const unsigned int stats_interval = 60;
