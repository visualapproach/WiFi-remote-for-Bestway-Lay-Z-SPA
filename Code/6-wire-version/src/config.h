#include <Arduino.h>
#include <ESP8266WiFi.h>

#define LEGACY_NAME "layzspa"

/*
 * Web Authentication
 */
/** get or set the state of the web authentication */
bool enableWebAuth = false;
/** get or set the username for web authentication */
const char *authUsername = "username";
/** get or set the password for web authentication */
const char *authPassword = "password";

/*
 * OTA Service Credentials
 */
/** get the name for the OTA service */
const char *OTAName = LEGACY_NAME;
/** get the password for the OTA service  */
const char *OTAPassword = "esp8266";

/*
 * Access Point Configuration
 */
/** get or set the state of the specific access point configuration */
bool enableAp = false;
/** get or set the name of the SSID */
String apSsid = "ssid";
/** get or set the password for the SSID */
String apPwd = "pwd";
/** autoportal pswd */
const char *wm_password = "esp8266";

/*
 * Static IP Configuration
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
 * MQTT Server Configuration
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
