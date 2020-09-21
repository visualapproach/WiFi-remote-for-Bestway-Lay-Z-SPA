//ArduinoOTA password
#define myOTApassword "esp8266"

//MQTT Credentials
#define myMqttIP (192, 168, 1, 100)
//#define myMqttName "mqtt.eclipse.org" //only for testing purpose
#define myMqttPort 1883
#define myMqttUser ""
#define myMqttPassword ""

//comment out what you don't need below

//load and use MQTT library
#define USE_MQTT

//Send information to http clients via websocket
//pages will still load but no SPA data will be sent TO the page (save bandwidth)         
#define USE_WEBINTERFACE

//change MQTT credentials via web page. You will still be able to load/save the page but it is ignored.
#define USE_MQTT_WEBINTERFACE

//store history in "eventlog.csv", and reboot time and reason in "tmp.txt"
//#define USE_LOGS
