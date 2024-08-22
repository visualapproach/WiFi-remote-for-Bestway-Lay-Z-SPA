#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include "FW_VERSION.h"
#define DEVICE_NAME "layzspa"
#define DEVICE_NAME_F F(DEVICE_NAME)
// #define HA_PREFIX "homeassistant"
#define HA_PREFIX_F F("homeassistant")
#define PROM_NAMESPACE "layzspa"
// #define PROM_NAMESPACE_F F("layzspa")
#define OTA_PSWD_F F("esp8266")
#define WM_AP_NAME_F String(F("Lay-Z-Spa-"))+String(ESP.getChipId())
#define WM_AP_PASSWORD_F F("layzspam0dule")
#define MQTT_USER_F F("username")
#define MQTT_PASSWORD_F F("password")
#define MQTT_CLIENT_ID_F DEVICE_NAME_F
#define MQTT_BASE_TOPIC_F DEVICE_NAME_F
/*
 * Miscellaneous
 */
/** get the state of password visibility */
const bool hidePasswords = true;
/** get the network hostname of the device (max. length 26) */
// const char *netHostname = DEVICE_NAME;
/** no comment :-) */
bool notify = false;
/** no comment :-) */
int notification_time = 32;

/*
 * Web Server Authentication (not used right now) TODO: implement authentication again
 */
/** get or set the state of the web authentication */
// bool enableWebAuth = false;
/** get or set the username for web authentication */
// String authUsername = "username";
/** get or set the password for web authentication */
// String authPassword = "password";

/*
 * OTA Service Credentials
 */
/** get the name for the OTA service */
// const char *OTAName = DEVICE_NAME;
/** get the password for the OTA service  */
// const char *OTAPassword = "esp8266";

/*
 * Web UI Configuration
 *
 * You can modify this via Web UI.
 */
/** get or set the state of displaying the "Temperature" section */
bool showSectionTemperature = true;
/** get or set the state of displaying the "Display" section */
bool showSectionDisplay = true;
/** get or set the state of displaying the "Control" section */
bool showSectionControl = true;
/** get or set the state of displaying the "Buttons" section */
bool showSectionButtons = true;
/** get or set the state of displaying the "Timer" section */
bool showSectionTimer = true;
/** get or set the state of displaying the "Totals" section */
bool showSectionTotals = true;
/** get or set the state of displaying slider or selector */
bool useControlSelector = false;

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
 *  to this 'AP mode' on the upstart setup() job. This setting can be changed from web ui.
 *
 * WARNING: For the case you set 'enableWmApFallback=false' you could lock out
 *  yourself when loosing your home network. You would have to "Reset WiFi" but
 *  you are not able to connect to the web ui without a connection.
 */
/** get the state of the WiFi configuration manager fallback on wifi failures */
// bool enableWmApFallback = true;
/** get the name for the WiFi configuration manager access point */
// const char *wmApName = "Lay-Z-Spa Module";
/** get the password for the WiFi configuration manager (min. 8, max. 63 chars; NULL to disable) */
// const char *wmApPassword = "layzspam0dule";
//const char *wmApPassword = NULL;

        /*
        * WiFi Access Point (deprecated)
        *
        * When a connection was established successfully, the 'enableAp' get automatically
        *  the state 'true' including writing credentials to the "wifi.json".
        *
        * You can modify this via Web UI.
        */
        // /** get or set the state of the specific access point configuration */
        // bool enableAp = false;
        // /** get or set the name of the SSID */
        // String apSsid = "ssid";
        // /** get or set the password for the SSID */
        // String apPwd = "pwd";

        // /*
        //  * WiFi Static IP
        //  *
        //  * You can modify this via Web UI.
        //  */
        // /** get or set the state of the static IP setup */
        // bool enableStaticIp4 = false;
        // /** get or set the IP address */
        // IPAddress ip4Address(192,168,0,30);
        // /** get or set the gateway address */
        // IPAddress ip4Gateway(192,168,0,1);
        // /** get or set the subnet mask */
        // IPAddress ip4Subnet(255,255,255,0);
        // /** get or set the primary DNS IP */
        // IPAddress ip4DnsPrimary(8,8,8,8);
        // /** get or set the secondary DNS IP */
        // IPAddress ip4DnsSecondary(8,8,4,4);

/*
 * MQTT Server
 *
 * You can modify this via Web UI.
 */
/** get or set the state of the MQTT server connection */
// bool useMqtt = false;
// /** get or set the MQTT server IP address */
// IPAddress mqttIpAddress(192,168,0,20);
// /** get or set the MQTT server port */
// int mqttPort = 1883;
// /** get or set the MQTT server username */
// String mqttUsername = "username";
// /** get or set the MQTT server password */
// String mqttPassword = "password";
// /** get or set the unique MQTT client ID */
// String mqttClientId = DEVICE_NAME;
// /** get or set the MQTT topic name */
// String mqttBaseTopic = DEVICE_NAME;
// /** get or set the MQTT telemetry interval */
// int mqttTelemetryInterval = 600;

struct sMQTT_info
{
    String mqttHost;
    int mqttTelemetryInterval;
    int mqttPort;
    String mqttUsername;
    String mqttPassword;
    String mqttClientId;
    String mqttBaseTopic;
    bool useMqtt = false;
};

sMQTT_info* mqtt_info;

#endif