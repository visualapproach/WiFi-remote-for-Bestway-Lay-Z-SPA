
#define LEGACY_NAME "BW_2.0.0"

#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
//#include <LittleFS.h>
#include <WebSocketsServer.h>
//used by ap config portal
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
//#include <Ticker.h>
#include <PubSubClient.h>         //** Requires library 2.8.0 or higher ** https://github.com/knolleary/pubsubclient
bool checkMqttConnection = false;

ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81
bool portalRunning = false;
File fsUploadFile;                 // a File variable to temporarily store the received file

//OTA credentials
const char *OTAName = LEGACY_NAME;          // A name and a password for the OTA service
const char *OTAPassword = "esp8266";        //myOTApassword;

//MQTT Credentials
IPAddress myMqttIP(192,168,4,126);
int myMqttPort = 1883;
String myMqttUser = "admin";
String myMqttPassword = "admin";
int mqtt_connect_count;                // Count of how may times we've connected to the MQTT server since booting (should always be 1 or more)
String mqtt_client_id = LEGACY_NAME;   // Used for unique MQTT Client ID
String base_mqtt_topic = LEGACY_NAME;  // Start of the MQTT Topic name used by this device
bool enableMQTT = false;
WiFiClient My_WiFi_Client;
PubSubClient MQTTclient(My_WiFi_Client);
