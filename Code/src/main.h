#include <Arduino.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>

#ifdef ESP8266

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include "certs.h"
#include <CertStoreBearSSL.h>
BearSSL::CertStore certStore;
#include <time.h>

#else

#include <WebServer.h>
#include <WiFi.h>

#endif

#include <LittleFS.h>
#include <PubSubClient.h> // ** Requires library 2.8.0 or higher ** https://github.com/knolleary/pubsubclient
#include <Ticker.h>
#include <WebSocketsServer.h>
#include <ESP_WiFiManager.h>
#include "bwc.h"

/**  */
Ticker bootlogTimer;
/**  */
Ticker periodicTimer;
Ticker startComplete;
/**  */
bool periodicTimerFlag = false;
/**  */
int periodicTimerInterval = 60;
/** get or set the state of the network beeing connected */
bool wifiConnected = false;

/** a WiFi Manager for configurations via access point */
ESP_WiFiManager wm;

/** a webserver object that listens on port 80 */
#if defined(ESP8266)
ESP8266WebServer server(80);
#elif defined(ESP32)
WebServer server(80);
#endif
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
bool enableMqtt = false;

/** used for handleAUX() */
bool runonce = true;

void setClock();

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
void handleGetVersions();
void handleGetHardware();
void handleSetHardware();
void handleHWtest();
void handleNotFound();
String getContentType(const String& filename);
bool handleFileRead(String path);
bool checkHttpPost(HTTPMethod method);
void handleGetConfig();
void handleSetConfig();
void handleGetCommandQueue();
void handleAddCommand();
void loadWebConfig();
void saveWebConfig();
void handleGetWebConfig();
void handleSetWebConfig();
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
String checkFirmwareUpdate();
void handleUpdate();
void updateFiles();
void updateStart();
void updateEnd();
void udpateProgress(int cur, int total);
void updateError(int err);
void startMqtt();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttConnect();
void handleESPInfo();

void setupHA();
void handlePrometheusMetrics();
