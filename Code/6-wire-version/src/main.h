#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "model.h"
#include "BWC_common.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <PubSubClient.h> // ** Requires library 2.8.0 or higher ** https://github.com/knolleary/pubsubclient
#include <Ticker.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager



/** The Answer to the Ultimate Question of Life, the Universe, and Everything. */
BWC bwc;

/**  */
Ticker bootlogTimer;
/**  */
Ticker periodicTimer;
/**  */
bool periodicTimerFlag = false;
/**  */
int periodicTimerInterval = 60;
/** get or set the state of the network beeing connected */
bool wifiConnected = false;

/** a WiFi Manager for configurations via access point */
WiFiManager wm;

/** a webserver object that listens on port 80 */
ESP8266WebServer server(80);
/** a file variable to temporarily store the received file */
File fsUploadFile;

/** a websocket object that listens on port 81 */
WebSocketsServer webSocket(81);
/**  */
Ticker updateWSTimer;
/**  */
bool sendWSFlag = false;

/** a WiFi client beeing used by the MQTT client */
WiFiClient aWifiClient;
/** a MQTT client */
PubSubClient mqttClient(aWifiClient);
/**  */
bool checkMqttConnection = false;
/** Count of how may times we've connected to the MQTT server since booting (should always be 1 or more) */
int mqtt_connect_count;
/**  */
String prevButtonName = "";
/**  */
bool prevunit = 1;
/**  */
Ticker updateMqttTimer;
/**  */
bool sendMQTTFlag = false;

/** used for handleAUX() */
bool runonce = true;
/** no interrupt or PWM */
const int solarpin = D0;
/** pulled to GND. Boot fails if pulled HIGH. */
const int myoutputpin = D8;



void handleAUX();

void sendWS();
String getOtherInfo();
void sendMQTT();

void startWiFi();
void startWiFiConfigPortal();

void startNTP();

void startOTA();
void stopall();

void startWebSocket();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len);

void startHttpServer();
void handleHWtest();
void handleNotFound();
String getContentType(String filename);
bool handleFileRead(String path);
void handleGetConfig();
void handleSetConfig();
void handleGetCommandQueue();
void handleAddCommand();
void loadWifi();
void saveWifi();
void handleGetWifi();
void handleSetWifi();
void handleResetWifi();
void resetWiFi();
void loadMqtt();
void saveMqtt();
void handleGetMqtt();
void handleSetMqtt();
void handleDir();
void handleFileUpload();
void handleFileRemove();
void handleRestart();

void startMqtt();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttConnect();
void setupHA();
void setupClimate();
void printStackSize();

#warning "Don't forget to upload file system also"