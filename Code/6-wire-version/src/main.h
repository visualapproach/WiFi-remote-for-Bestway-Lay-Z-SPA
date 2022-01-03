#include <Arduino.h>
#include "BWC_8266.h"
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>         //** Requires library 2.8.0 or higher ** https://github.com/knolleary/pubsubclient
#include "credentials.h"

bool checkMqttConnection = false;
ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81
bool portalRunning = false;
File fsUploadFile;                 // a File variable to temporarily store the received file
WiFiClient My_WiFi_Client;
PubSubClient MQTTclient(My_WiFi_Client);
int mqtt_connect_count;                // Count of how may times we've connected to the MQTT server since booting (should always be 1 or more)

WiFiManager wm;
Ticker updateMqttTimer;
Ticker updateWSTimer;
Ticker periodicTimer;

BWC bwc;
bool sendWSFlag = false;
bool sendMQTTFlag = false;
bool periodicTimerFlag = false;
String prevButtonName = "";
const int solarpin = D0;    //no interrupt or PWM
const int myoutputpin = D8; //pulled to GND. Boot fails if pulled HIGH.
bool runonce = true;

void handleAUX();
void sendMQTTsetFlag();
void sendWSsetFlag();
void periodicTimerCallback();
void sendWS();
void sendMQTT();
String getContentType(String filename);
bool handleFileRead(String path);
void handleNotFound();
void handleFileUpload();
void handleFileRemove();
void startWebSocket();
void startServer();
void handleDir();
void startNTP();
void startOTA();
void startWiFi();
void handleGetConfig();
void handleSetConfig();
void handleGetMQTT();
void handleSetMQTT();
void saveMQTT();
void loadMQTT();
void handleResetWifi();
void handleGetCommandQueue();
void handleAddCommand();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len);

/*
   MQTT functions
*/
void startMQTT();
void MQTTcallback(char* topic, byte* payload, unsigned int length);
void MQTT_Connect();