#define MY_VERSION "BW4W_1.0"

#include "Arduino.h"
#include "BWC_8266_4w.h"
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
const char *OTAName = MY_VERSION;           // A name and a password for the OTA service
const char *OTAPassword = "esp8266";        //myOTApassword;

//MQTT Credentials
IPAddress myMqttIP(192,168,4,126);
const int myMqttPort = 1883;
const char *myMqttUser = "admin";
const char *myMqttPassword = "admin";
int mqtt_connect_count;                    // Count of how may times we've connected to the MQTT server since booting (should always be 1 or more)
const char *mqtt_client_id = MY_VERSION;   // Used for unique MQTT Client ID
const char *base_mqtt_topic = MY_VERSION;  // Start of the MQTT Topic name used by this device
WiFiClient My_WiFi_Client;
PubSubClient MQTTclient(My_WiFi_Client);
// If failing to connect at all, this will stall the loop = Erratic behavior may occur if called to often.
// Default frequency of once every 10 min shouldn't be a problem though.
// This function is called by the mqtt timer
void sendMQTTsetFlag();
void sendWSsetFlag();
// Send status data to web client in JSON format (because it is easy to decode on the other side)
void sendMessage(int msgtype);
/*
   File handlers below. Most users can stop reading here.
*/
String getContentType(String filename);
bool handleFileRead(String path);
void handleNotFound();
void handleFileUpload(); 
/*
   Starters - bon apetit
*/
void startWebSocket();
void startServer();
void startOTA();
void startWiFi();
/*
   Web server functions to exchange data between server and web client
*/
//response to /getconfig/
void handleGetConfig();
//response to /setconfig/
void handleSetConfig();
//response to /getcommands/
void handleGetCommandQueue();
//respone to /addcommand/
void handleAddCommand();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len);
/*
   MQTT functions
*/
void startMQTT();
void MQTTcallback(char* topic, byte* payload, unsigned int length);
void MQTT_Connect();