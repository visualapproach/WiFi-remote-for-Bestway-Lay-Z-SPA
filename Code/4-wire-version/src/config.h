#include <Arduino.h>
#include <ESP8266WiFi.h>

#define LEGACY_NAME "layzspa"
#define FW_VERSION "4W_2022-05-18"

/*
 * Miscellaneous
 */
/** get the state of password visibility */
const bool hidePasswords = true;
/** get the network hostname of the device (max. length 26) */
const char *netHostname = LEGACY_NAME;

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
 *
 * A fresh/clean ESP needs WiFi credentials to be connected to a network.
 * This manager creates an access point when there is no persistent data set yet.
 * Persistent data means, the data the ESP writes to it's internal memory,
 *  when a connection was established successfully.
 * Means not the data we write with the "WiFi Access Point" configuration below.
 *  (wifi.json on flash memory)
 * 
 * NOTICE: If you want your ESP running continuously without creating an access point
 *  when having WiFi issues, set 'enableWmApFallback=false', otherwise we could fallback
 *  to this 'AP mode' on the upstart setup() job.
 * 
 * WARNING: For the case you set 'enableWmApFallback=false' you could lock out
 *  yourself when loosing your home network. You would have to "Reset WiFi" but
 *  you are not able to connect to the Web GUI without a connection.
 * 
 * TODO: create a hardware based key to "Reset WiFi"
 */
/** get the state of the WiFi configuration manager fallback on wifi failures */
const bool enableWmApFallback = true;
/** get the name for the WiFi configuration manager access point */
const char *wmApName = "Lay-Z-Spa Module";
/** get the password for the WiFi configuration manager (min. 8, max. 63 chars; NULL to disable) */
const char *wmApPassword = "layzspam0dule";
//const char *wmApPassword = NULL;

/*
 * WiFi Access Point
 *
 * When a connection was established successfully, the 'enableAp' get automatically
 *  the state 'true' including writing credentials to the "wifi.json".
 * 
 * You can modify this via Web GUI.
 */
/** get or set the state of the specific access point configuration */
bool enableAp = false;
/** get or set the name of the SSID */
String apSsid = "ssid";
/** get or set the password for the SSID */
String apPwd = "pwd";

/*
 * WiFi Static IP
 *
 * You can modify this via Web GUI.
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
 *
 * You can modify this via Web GUI.
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
/** get or set the MQTT telemetry interval */
int mqttTelemetryInterval = 600;

/* only enable this when debugging communication
 * This feature has not been tested yet
 */
bool DEBUGSERIAL = false;