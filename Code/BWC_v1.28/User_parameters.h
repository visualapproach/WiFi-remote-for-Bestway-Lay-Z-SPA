//web sockets and mqtt sending interval (seconds)
const float WS_INTERVAL = 1.4;
const float MQTT_INTERVAL = 60.0;


//ArduinoOTA password
#define myOTApassword "esp8266"


//MQTT Credentials
#define myMqttIP (192, 168, 4, 126)
//#define myMqttName "mqtt.eclipse.org" //only for testing purpose
#define myMqttPort 1883
#define myMqttUser ""
#define myMqttPassword ""

/*
 * this device will publish MQTT messages to topic: "MY_VERSION/message" where MY_VERSION is defined in a_globals.h
 * Commands should be sent to this device in topic "MY_VERSION/command"
 * E.g topic:"BW_1.28/command" payload:"e" will lock/unlock the SPA
 * If using USE_MQTT_WEBINTERFACE be sure to save your config from the web interface. The hardcoded parameters above will only be used if there
 * is no file "mqtt.txt" on the file system.
 * 
 */


//comment out what you don't need below

//load and use MQTT library
#define USE_MQTT

//Send information to http clients via websocket
//pages will still load but no SPA data will be sent TO the page (save bandwidth)         
#define USE_WEBINTERFACE

//change MQTT credentials via web page. You will still be able to load/save the page if commented, but it is ignored.
#define USE_MQTT_WEBINTERFACE

//store history in "eventlog.csv", and reboot time and reason in "tmp.txt"
//#define USE_LOGS
