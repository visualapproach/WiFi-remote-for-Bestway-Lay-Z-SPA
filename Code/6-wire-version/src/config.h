#include <Arduino.h>
#include <ESP8266WiFi.h>

#define LEGACY_NAME "layzspa"

/*
 * Web Server Authentication
 */
/** get or set the state of the web authentication */
bool enableWebAuth = false;
/** get or set the username for web authentication */
String authUsername = "username";
/** get or set the password for web authentication */
String authPassword = "password";

/*
 * OTA Service Credentials
 */
/** get the name for the OTA service */
const char *OTAName = LEGACY_NAME;
/** get the password for the OTA service  */
const char *OTAPassword = "esp8266";

/*
 * WiFi Configuration Manager
 */
/** get the state of the WiFi configuration manager */
const bool enableWmAp = true;
/** get the name for the WiFi configuration manager access point */
const char *wmApName = "Lay-Z-Spa Module";
/** get the password for the WiFi configuration manager (min. 8, max. 63 chars) */
const char *wmApPassword = "layzspam0dule";

/*
 * WiFi Access Point
 */
/** get or set the state of the specific access point configuration */
bool enableAp = false;
/** get or set the name of the SSID */
String apSsid = "ssid";
/** get or set the password for the SSID */
String apPwd = "pwd";

/*
 * WiFi Static IP
 */
/** get or set the state of the static IP setup */
bool enableStaticIp4 = false;
/** get or set the IP address */
IPAddress ip4Address(192,168,0,30);
/** get or set the gateway address */
IPAddress ip4Gateway(192,168,0,1);
/** get or set the subnet mask */
IPAddress ip4Subnet(255,255,255,0);
/** get or set the primary DNS IP */
IPAddress ip4DnsPrimary(8,8,8,8);
/** get or set the secondary DNS IP */
IPAddress ip4DnsSecondary(8,8,4,4);

/*
 * MQTT Server
 */
/** get or set the state of the MQTT server connection */
bool enableMqtt = false;
/** get or set the MQTT server IP address */
IPAddress mqttIpAddress(192,168,0,20);
/** get or set the MQTT server port */
int mqttPort = 1883;
/** get or set the MQTT server username */
String mqttUsername = "username";
/** get or set the MQTT server password */
String mqttPassword = "password";
/** get or set the unique MQTT client ID */
String mqttClientId = LEGACY_NAME;
/** get or set the MQTT topic name */
String mqttBaseTopic = LEGACY_NAME;
