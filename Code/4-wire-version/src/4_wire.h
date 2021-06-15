#include "Arduino.h"
#include <credentials.h>
#include <BWC_8266_4w.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>         //** Requires library 2.8.0 or higher ** https://github.com/knolleary/pubsubclient


ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81
bool portalRunning = false;
File fsUploadFile;                 // a File variable to temporarily store the received file
bool checkMqttConnection = false;
int mqtt_connect_count;                    // Count of how may times we've connected to the MQTT server since booting (should always be 1 or more)
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