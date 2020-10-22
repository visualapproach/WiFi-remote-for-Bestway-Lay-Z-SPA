
#define MY_VERSION "BW_2.0.0"

#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
//#include <LittleFS.h>
#include <WebSocketsServer.h>
//used by ap config portal
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81
bool portalRunning = false;
File fsUploadFile;                 // a File variable to temporarily store the received file

//OTA credentials
const char *OTAName = MY_VERSION;           // A name and a password for the OTA service
const char *OTAPassword = "esp8266";        //myOTApassword;
