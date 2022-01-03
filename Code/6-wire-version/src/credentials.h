#include <Arduino.h>
#include <ESP8266WiFi.h>

#define LEGACY_NAME "BW_2.0.0"

//OTA credentials
const char *OTAName = LEGACY_NAME;          // A name and a password for the OTA service
const char *OTAPassword = "esp8266";        //myOTApassword;
const char *www_username = "admin";         //web gui username
const char *www_password = "esp8266";       //web gui pswd
const char *wm_password = "esp8266";        //autoportal pswd

//MQTT Credentials
IPAddress myMqttIP(192,168,4,126);
int myMqttPort = 1883;
String myMqttUser = "admin";
String myMqttPassword = "admin";
String mqtt_client_id = LEGACY_NAME;   // Used for unique MQTT Client ID
String base_mqtt_topic = LEGACY_NAME;  // Start of the MQTT Topic name used by this device
bool enableMQTT = false;

