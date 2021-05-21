#include <Arduino.h>
#include "BWC_8266.h"
#include "globals.h"

void sendMQTTsetFlag();
void sendWSsetFlag();
void sendMessage(int msgtype);
String getContentType(String filename);
bool handleFileRead(String path);
void handleNotFound();
void handleFileUpload();
void handleFileRemove();
void startWebSocket();
void startServer();
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
