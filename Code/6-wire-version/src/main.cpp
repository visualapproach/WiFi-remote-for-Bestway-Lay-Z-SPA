#include "main.h"

WiFiManager wm;
Ticker updateMqttTimer;
Ticker updateWSTimer;
BWC bwc;
bool sendWSFlag = false;
bool sendMQTTFlag = false;
String prevButtonName = "";
const int solarpin = D0;    //no interrupt or PWM
const int myoutputpin = D8; //pulled to GND. Boot fails if pulled HIGH.
bool runonce = true;

void setup() {
  // put your setup code here, to run once:
  pinMode(solarpin, INPUT_PULLUP);
  pinMode(myoutputpin, OUTPUT);
  digitalWrite(myoutputpin, LOW);
  Serial.begin(115200);		//As if you connected serial to your pump...
  LittleFS.begin(); // needs to be loaded here for reading the wifi.json
  loadWifi();
  startWiFi();
  startOTA();
  startServer();
  startWebSocket();
  bwc.begin(); //no params = default pins
  //Default pins:
  // bwc.begin(			
			// int cio_cs_pin 		= D1, 
			// int cio_data_pin 	= D7, 
			// int cio_clk_pin 		= D2, 
			// int dsp_cs_pin 		= D3, 
			// int dsp_data_pin 	= D5, 
			// int dsp_clk_pin 		= D4, 
			// int dsp_audio_pin 	= D6 
			// );
	//example: bwc.begin(D1, D2, D3, D4, D5, D6, D7);
  startMQTT();
  updateMqttTimer.attach(600, sendMQTTsetFlag); //update mqtt every 10 minutes. Mqtt will also be updated on every state change
  updateWSTimer.attach(2.0, sendWSsetFlag);     //update webpage every 2 secs plus state changes
  bwc.print(WiFi.localIP().toString());
}

void loop() {
  webSocket.loop();             // constantly check for websocket events
  server.handleClient();        // run the server
  ArduinoOTA.handle();          // listen for OTA events
  //wm.process();
  if(enableMqtt){
    if (!MQTTclient.loop()) MQTT_Connect();           // Do MQTT magic
  }
  
  bwc.loop();                   // Fiddle with the pump computer
  if (bwc.newData()) {
    sendMessage(1);//ws
    if(enableMqtt) sendMessage(0);//mqtt
    //bwc.saveEventlog();       //will only fill up and wear flash memory eventually
  }
  if (sendWSFlag) {
    sendWSFlag = false;
    sendMessage(1);//ws
  }
  if (sendMQTTFlag && enableMqtt) {
    sendMQTTFlag = false;
    sendMessage(0);//MQTT
  }
  if(enableMqtt){
    String msg = bwc.getButtonName();
    //publish pretty button name if display button is pressed (or NOBTN if released)
    if(!msg.equals(prevButtonName)) {
      MQTTclient.publish((String(mqttBaseTopic) + "/button").c_str(), String(msg).c_str(), true);
      prevButtonName = msg;
    }
  }
  //handleAUX();

  //You can add own code here, but don't stall! If CPU is choking you can try to run @ 160 MHz, but that's cheating!
}

void handleAUX() {
   //Usage example. Solar panels are giving a (3.3V) signal to start dumping electricity into the pool heater.
   //Rapid changes on this pin will fill up the command queue and stop you from adding other commands
   //It will also cause the heater to turn on and off as fast as it can
   //I have not tested this feature, but what can go wrong ;-)
   if (digitalRead(solarpin) == HIGH && runonce == true) {
     bwc.qCommand(SETHEATER, 1, 0, 0);       // cmd:set heater, to ON (1), immidiately, no repeat
     runonce = false;                        // to stop queing commands every loop. We only want to trigger once
   }
   if (digitalRead(solarpin) == LOW && runonce == false) {
     bwc.qCommand(SETPUMP, 0, 0, 0);         // change to SETHEATER if you want the pump to continue filtering
     runonce = true;
   }
  
   //Switch the output pin when temperature is below or above 30
   //Don't load the pin above specs (a few mA)
   if (bwc.getState(TEMPERATURE) < 30){
     digitalWrite(myoutputpin, HIGH);
   } else {
     digitalWrite(myoutputpin, LOW);
   }
}

// This function is called by the mqtt timer
void sendMQTTsetFlag() {
  sendMQTTFlag = true;
}
void sendWSsetFlag() {
  sendWSFlag = true;
}

// Send status data to web client in JSON format (because it is easy to decode on the other side)
void sendMessage(int msgtype) {

  //send states to web sockets
  if (msgtype == 1) {
    String jsonmsg = bwc.getJSONStates();
    webSocket.broadcastTXT(jsonmsg);
    jsonmsg = bwc.getJSONTimes();
    webSocket.broadcastTXT(jsonmsg);
    String mqttJSONstatus = String("{\"CONTENT\":\"OTHER\",\"MQTT\":") + String(MQTTclient.state()) + 
                            String(",\"PressedButton\":\"") + bwc.getPressedButton() +
                            String("\",\"HASJETS\":") + String(HASJETS) +
                            String("}");
    webSocket.broadcastTXT(mqttJSONstatus);
  }

  //Send STATES and TIMES to MQTT - 877dev
  //It would be more elegant to send both states and times on the "message" topic
  //and use the "CONTENT" field to distinguish between them
  //but it might break peoples home automation setups, so to keep it backwards
  //compatible I choose to start a new topic "/times"
  if (msgtype == 0) {
    String jsonmsg = bwc.getJSONStates();
    if (MQTTclient.publish((String(mqttBaseTopic) + "/message").c_str(), String(jsonmsg).c_str(), true))
    {
      //Serial.println(F("MQTT published"));
    }
    else
    {
      //Serial.println(F("MQTT not published"));
    }
    jsonmsg = bwc.getJSONTimes();
    if (MQTTclient.publish((String(mqttBaseTopic) + "/times").c_str(), String(jsonmsg).c_str(), true))
    {
      //Serial.println(F("MQTT published"));
    }
    else
    {
      //Serial.println(F("MQTT not published"));
    }
  }
}



void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println(F("OTA Start"));
    bwc.stop();
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\r\nOTA End"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
  });
  ArduinoOTA.begin();
  Serial.println(F("OTA ready\r\n"));
}



/**
 * Start a Wi-Fi access point, and try to connect to some given access points.
 * Then wait for either an AP or STA connection
 */
void startWiFi()
{
  WiFi.mode(WIFI_STA);
  
  if (enableStaticIp4)
  {
    WiFi.config(ip4Address, ip4Gateway, ip4Subnet, ip4DnsPrimary, ip4DnsSecondary);
  }

  if (enableAp)
  {
    Serial.println("WiFi > using WiFi configuration with SSID \"" + apSsid + "\"");
    WiFi.begin(apSsid, apPwd);

    Serial.print("WiFi > Trying to connect ...");
    int maxTries = 5;
    int tryCount = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.print(".");
      tryCount++;

      if (tryCount >= maxTries)
      {
        Serial.println("");
        Serial.println("WiFi > NOT Connected!");
        // disable specific WiFi config
        enableAp = false;
        enableStaticIp4 = false;
        // fallback to WiFi config portal
        Serial.println("WiFi > Using WiFiManager Config Portal");
        startWiFiConfigPortal();
        break;
      }
    }
  }
  else
  {
    Serial.println("WiFi > Using WiFiManager Config Portal");
    startWiFiConfigPortal();
  }

  apSsid = WiFi.SSID();
  apPwd = WiFi.psk();
  saveWifi();

  Serial.println("");
  Serial.println("WiFi > Connected");
  Serial.println(" SSID: \"" + WiFi.SSID() + "\"");
  Serial.println(" IP: \"" + WiFi.localIP().toString() + "\"");
}

/**
 * start WiFiManager configuration portal
 */
void startWiFiConfigPortal()
{
  wm.autoConnect("Lay-Z-Spa WiFi Config");

  Serial.print("WiFi > Trying to connect ...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
}



/**
 * start a web socket server
 */
void startWebSocket()
{
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println(F("WebSocket server started."));
}

/**
 * handle web socket events
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len)
{
  // When a WebSocket message is received
  switch (type)
  {
    // if the websocket is disconnected
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    // if a new websocket connection is established
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        sendMessage(1);
      }
      break;

    // if new text data is received
    case WStype_TEXT:
      {
        Serial.printf("[%u] get Text: %s\n", num, payload);
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
          Serial.println(F("JSON command failed"));
          return;
        }

        // Copy values from the JsonDocument to the Config
        uint32_t command = doc["CMD"];
        uint32_t value = doc["VALUE"];
        uint32_t xtime = doc["XTIME"];
        uint32_t interval = doc["INTERVAL"];
        //add command to the command queue
        bwc.qCommand(command, value, xtime, interval);
      }
      break;
      
    default:
      break;
  }
}



/**
 * start a HTTP server with a file read and upload handler
 */
void startServer()
{
  server.on(F("/getconfig/"), handleGetConfig);
  server.on(F("/setconfig/"), handleSetConfig);
  server.on(F("/getcommands/"), handleGetCommandQueue);
  server.on(F("/addcommand/"), handleAddCommand);
  server.on(F("/getwifi/"), handleGetWifi);
  server.on(F("/setwifi/"), handleSetWifi);
  server.on(F("/resetwifi/"), handleResetWifi);
  server.on(F("/getmqtt/"), handleGetMqtt);
  server.on(F("/setmqtt/"), handleSetMqtt);
  server.on(F("/dir/"), handleDir);
  server.on(F("/upload.html"), HTTP_POST, [](){
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  server.on(F("/remove.html"), HTTP_POST, handleFileRemove);
  server.on(F("/restart/"), handleRestart);
  // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists
  server.onNotFound(handleNotFound);
  // start the HTTP server
  server.begin();
  Serial.println(F("HTTP server started."));
}

/**
 * if the requested file or page doesn't exist, return a 404 not found error
 */
void handleNotFound()
{
  // check if the file exists in the flash memory (LittleFS), if so, send it
  if (!handleFileRead(server.uri()))
  {
    server.send(404, "text/plain", "404: File Not Found");
  }
}

/**
 * determine the filetype of a given filename, based on the extension
 */
String getContentType(String filename)
{
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".json")) return "application/json";
  return "text/plain";
}

/**
 * send the right file to the client (if it exists)
 */
bool handleFileRead(String path)
{
  // ask for web authentication
  if (enableWebAuth)
  {
    if (!server.authenticate(authUsername, authPassword))
    {
      server.requestAuthentication();
    }
  }
  
  Serial.println("handleFileRead: " + path);
  // If a folder is requested, send the index file
  if (path.endsWith("/"))
  {
    path += F("index.html");
  }
  // deny reading credentials
  //if (path.equalsIgnoreCase("/mqtt.json") || path.equalsIgnoreCase("/wifi.json"))
  //{
  //  Serial.println(String("\tFile reading denied (credentials)."));
  //  return false;
  //}
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (LittleFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed version
    File file = LittleFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    Serial.println(String("\tFile size: ") + String(sent));
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

/**
 * checks the method to be a POST
 */
bool checkHttpPost(HTTPMethod method)
{
  if (method != HTTP_POST)
  {
    server.send(405, "text/plain", "Method Not Allowed");
    return false;
  }
  return true;
}

/**
 * response for /getconfig/
 * web server prints a json document
 */
void handleGetConfig()
{
  if (!checkHttpPost(server.method())) return;

  String json = bwc.getJSONSettings();
  server.send(200, "text/plain", json);
}

/**
 * response for /setconfig/
 * save spa config
 */
void handleSetConfig()
{
  if (!checkHttpPost(server.method())) return;

  String message = server.arg(0);
  bwc.setJSONSettings(message);

  server.send(200, "plain/text", "");
}

/**
 * response for /getcommands/
 * web server prints a json document
 */
void handleGetCommandQueue()
{
  if (!checkHttpPost(server.method())) return;

  String json = bwc.getJSONCommandQueue();
  server.send(200, "text/plain", json);
}

/**
 * response for /addcommand/
 * add a command to the queue
 */
void handleAddCommand()
{
  if (!checkHttpPost(server.method())) return;

  DynamicJsonDocument doc(256);
  String message = server.arg(0);
  DeserializationError error = deserializeJson(doc, message);
  if (error)
  {
    server.send(400, "plain/text", "Error deserializing message");
    return;
  }

  uint32_t command = doc["CMD"];
  uint32_t value = doc["VALUE"];
  uint32_t xtime = doc["XTIME"];
  uint32_t interval = doc["INTERVAL"];

  bwc.qCommand(command, value, xtime, interval);

  server.send(200, "text/plain", "");
}

/**
 * load WiFi json configuration from "wifi.json"
 */
void loadWifi()
{
  File file = LittleFS.open("wifi.json", "r");
  if (!file)
  {
    Serial.println(F("Failed to read wifi.json. Using defaults."));
    return;
  }

  DynamicJsonDocument doc(1024);

  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    Serial.println(F("Failed to deserialize wifi.json"));
    file.close();
    return;
  }

  enableAp = doc["enableAp"];
  apSsid = doc["apSsid"].as<String>();
  apPwd = doc["apPwd"].as<String>();

  enableStaticIp4 = doc["enableStaticIp4"];
  ip4Address[0] = doc["ip4Address"][0];
  ip4Address[1] = doc["ip4Address"][1];
  ip4Address[2] = doc["ip4Address"][2];
  ip4Address[3] = doc["ip4Address"][3];
  ip4Gateway[0] = doc["ip4Gateway"][0];
  ip4Gateway[1] = doc["ip4Gateway"][1];
  ip4Gateway[2] = doc["ip4Gateway"][2];
  ip4Gateway[3] = doc["ip4Gateway"][3];
  ip4Subnet[0] = doc["ip4Subnet"][0];
  ip4Subnet[1] = doc["ip4Subnet"][1];
  ip4Subnet[2] = doc["ip4Subnet"][2];
  ip4Subnet[3] = doc["ip4Subnet"][3];
  ip4DnsPrimary[0] = doc["ip4DnsPrimary"][0];
  ip4DnsPrimary[1] = doc["ip4DnsPrimary"][1];
  ip4DnsPrimary[2] = doc["ip4DnsPrimary"][2];
  ip4DnsPrimary[3] = doc["ip4DnsPrimary"][3];
  ip4DnsSecondary[0] = doc["ip4DnsSecondary"][0];
  ip4DnsSecondary[1] = doc["ip4DnsSecondary"][1];
  ip4DnsSecondary[2] = doc["ip4DnsSecondary"][2];
  ip4DnsSecondary[3] = doc["ip4DnsSecondary"][3];
}

/**
 * save WiFi json configuration to "wifi.json"
 */
void saveWifi()
{
  File file = LittleFS.open("wifi.json", "w");
  if (!file)
  {
    Serial.println(F("Failed to save wifi.json"));
    return;
  }

  DynamicJsonDocument doc(1024);

  doc["enableAp"] = enableAp;
  doc["apSsid"] = apSsid;
  doc["apPwd"] = apPwd;

  doc["enableStaticIp4"] = enableStaticIp4;
  doc["ip4Address"][0] = ip4Address[0];
  doc["ip4Address"][1] = ip4Address[1];
  doc["ip4Address"][2] = ip4Address[2];
  doc["ip4Address"][3] = ip4Address[3];
  doc["ip4Gateway"][0] = ip4Gateway[0];
  doc["ip4Gateway"][1] = ip4Gateway[1];
  doc["ip4Gateway"][2] = ip4Gateway[2];
  doc["ip4Gateway"][3] = ip4Gateway[3];
  doc["ip4Subnet"][0] = ip4Subnet[0];
  doc["ip4Subnet"][1] = ip4Subnet[1];
  doc["ip4Subnet"][2] = ip4Subnet[2];
  doc["ip4Subnet"][3] = ip4Subnet[3];
  doc["ip4DnsPrimary"][0] = ip4DnsPrimary[0];
  doc["ip4DnsPrimary"][1] = ip4DnsPrimary[1];
  doc["ip4DnsPrimary"][2] = ip4DnsPrimary[2];
  doc["ip4DnsPrimary"][3] = ip4DnsPrimary[3];
  doc["ip4DnsSecondary"][0] = ip4DnsSecondary[0];
  doc["ip4DnsSecondary"][1] = ip4DnsSecondary[1];
  doc["ip4DnsSecondary"][2] = ip4DnsSecondary[2];
  doc["ip4DnsSecondary"][3] = ip4DnsSecondary[3];

  if (serializeJson(doc, file) == 0)
  {
    Serial.println("{\"error\": \"Failed to serialize file\"}");
  }
  file.close();
}

/**
 * response for /getwifi/
 * web server prints a json document
 */
void handleGetWifi()
{
  if (!checkHttpPost(server.method())) return;
  
  DynamicJsonDocument doc(1024);

  doc["enableAp"] = enableAp;
  doc["apSsid"] = apSsid;
  doc["apPwd"] = "<enter password>";
  // print credentials
  if (true)
  {
    doc["apPwd"] = apPwd;
  }

  doc["enableStaticIp4"] = enableStaticIp4;
  doc["ip4Address"][0] = ip4Address[0];
  doc["ip4Address"][1] = ip4Address[1];
  doc["ip4Address"][2] = ip4Address[2];
  doc["ip4Address"][3] = ip4Address[3];
  doc["ip4Gateway"][0] = ip4Gateway[0];
  doc["ip4Gateway"][1] = ip4Gateway[1];
  doc["ip4Gateway"][2] = ip4Gateway[2];
  doc["ip4Gateway"][3] = ip4Gateway[3];
  doc["ip4Subnet"][0] = ip4Subnet[0];
  doc["ip4Subnet"][1] = ip4Subnet[1];
  doc["ip4Subnet"][2] = ip4Subnet[2];
  doc["ip4Subnet"][3] = ip4Subnet[3];
  doc["ip4DnsPrimary"][0] = ip4DnsPrimary[0];
  doc["ip4DnsPrimary"][1] = ip4DnsPrimary[1];
  doc["ip4DnsPrimary"][2] = ip4DnsPrimary[2];
  doc["ip4DnsPrimary"][3] = ip4DnsPrimary[3];
  doc["ip4DnsSecondary"][0] = ip4DnsSecondary[0];
  doc["ip4DnsSecondary"][1] = ip4DnsSecondary[1];
  doc["ip4DnsSecondary"][2] = ip4DnsSecondary[2];
  doc["ip4DnsSecondary"][3] = ip4DnsSecondary[3];

  String json;
  if (serializeJson(doc, json) == 0)
  {
    json = "{\"error\": \"Failed to serialize message\"}";
  }
  server.send(200, "text/plain", json);
}

/**
 * response for /setwifi/
 * web server prints a json document
 */
void handleSetWifi()
{
  if (!checkHttpPost(server.method())) return;

  DynamicJsonDocument doc(1024);
  String message = server.arg(0);
  DeserializationError error = deserializeJson(doc, message);
  if (error)
  {
    Serial.println(F("Failed to read config file"));
    server.send(400, "plain/text", "Error deserializing message");
    return;
  }

  enableAp = doc["enableAp"];
  apSsid = doc["apSsid"].as<String>();
  apPwd = doc["apPwd"].as<String>();

  enableStaticIp4 = doc["enableStaticIp4"];
  ip4Address[0] = doc["ip4Address"][0];
  ip4Address[1] = doc["ip4Address"][1];
  ip4Address[2] = doc["ip4Address"][2];
  ip4Address[3] = doc["ip4Address"][3];
  ip4Gateway[0] = doc["ip4Gateway"][0];
  ip4Gateway[1] = doc["ip4Gateway"][1];
  ip4Gateway[2] = doc["ip4Gateway"][2];
  ip4Gateway[3] = doc["ip4Gateway"][3];
  ip4Subnet[0] = doc["ip4Subnet"][0];
  ip4Subnet[1] = doc["ip4Subnet"][1];
  ip4Subnet[2] = doc["ip4Subnet"][2];
  ip4Subnet[3] = doc["ip4Subnet"][3];
  ip4DnsPrimary[0] = doc["ip4DnsPrimary"][0];
  ip4DnsPrimary[1] = doc["ip4DnsPrimary"][1];
  ip4DnsPrimary[2] = doc["ip4DnsPrimary"][2];
  ip4DnsPrimary[3] = doc["ip4DnsPrimary"][3];
  ip4DnsSecondary[0] = doc["ip4DnsSecondary"][0];
  ip4DnsSecondary[1] = doc["ip4DnsSecondary"][1];
  ip4DnsSecondary[2] = doc["ip4DnsSecondary"][2];
  ip4DnsSecondary[3] = doc["ip4DnsSecondary"][3];

  saveWifi();

  server.send(200, "plain/text", "");
}

/**
 * response for /resetwifi/
 * do this before giving away the device
 */
void handleResetWifi()
{
  Serial.println("WiFi connection reset (erase) ...");
  WiFi.disconnect();
  delay(3000);
  Serial.println("ESP reset ...");
  ESP.reset();
}

/**
 * load MQTT json configuration from "mqtt.json"
 */
void loadMqtt()
{
  File file = LittleFS.open("mqtt.json", "r");
  if (!file)
  {
    Serial.println(F("Failed to read mqtt.json. Using defaults."));
    return;
  }

  DynamicJsonDocument doc(1024);

  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    Serial.println(F("Failed to deserialize mqtt.json."));
    file.close();
    return;
  }
  
  enableMqtt = doc["enableMqtt"];
  mqttIpAddress[0] = doc["mqttIpAddress"][0];
  mqttIpAddress[1] = doc["mqttIpAddress"][1];
  mqttIpAddress[2] = doc["mqttIpAddress"][2];
  mqttIpAddress[3] = doc["mqttIpAddress"][3];
  mqttPort = doc["mqttPort"];
	mqttUsername = doc["mqttUsername"].as<String>();
	mqttPassword = doc["mqttPassword"].as<String>();
	mqttClientId = doc["mqttClientId"].as<String>();
	mqttBaseTopic = doc["mqttBaseTopic"].as<String>();
}

/**
 * save MQTT json configuration to "mqtt.json"
 */
void saveMqtt()
{
  File file = LittleFS.open("mqtt.json", "w");
  if (!file)
  {
    Serial.println(F("Failed to save mqtt.json"));
    return;
  }

  DynamicJsonDocument doc(1024);

  doc["enableMqtt"] = enableMqtt;
  doc["mqttIpAddress"][0] = mqttIpAddress[0];
  doc["mqttIpAddress"][1] = mqttIpAddress[1];
  doc["mqttIpAddress"][2] = mqttIpAddress[2];
  doc["mqttIpAddress"][3] = mqttIpAddress[3];
  doc["mqttPort"] = mqttPort;
  doc["mqttUsername"] = mqttUsername;
  doc["mqttPassword"] = mqttPassword;
  doc["mqttClientId"] = mqttClientId;
  doc["mqttBaseTopic"] = mqttBaseTopic;

  if (serializeJson(doc, file) == 0)
  {
    Serial.println("{\"error\": \"Failed to serialize file\"}");
  }
  file.close();
}

/**
 * response for /getmqtt/
 * web server prints a json document
 */
void handleGetMqtt()
{
  if (!checkHttpPost(server.method())) return;
  
  DynamicJsonDocument doc(1024);

  doc["enableMqtt"] = enableMqtt;
  doc["mqttIpAddress"][0] = mqttIpAddress[0];
  doc["mqttIpAddress"][1] = mqttIpAddress[1];
  doc["mqttIpAddress"][2] = mqttIpAddress[2];
  doc["mqttIpAddress"][3] = mqttIpAddress[3];
  doc["mqttPort"] = mqttPort;
  doc["mqttUsername"] = "<enter username>";
  doc["mqttPassword"] = "<enter password>";
  // print credentials
  if (true)
  {
    doc["mqttUsername"] = mqttUsername;
    doc["mqttPassword"] = mqttPassword;
  }
  doc["mqttClientId"] = mqttClientId;
  doc["mqttBaseTopic"] = mqttBaseTopic;

  String json;
  if (serializeJson(doc, json) == 0)
  {
    json = "{\"error\": \"Failed to serialize message\"}";
  }
  server.send(200, "text/plain", json);
}

/**
 * response for /setmqtt/
 * web server prints a json document
 */
void handleSetMqtt()
{
  if (!checkHttpPost(server.method())) return;

  DynamicJsonDocument doc(1024);
  String message = server.arg(0);
  DeserializationError error = deserializeJson(doc, message);
  if (error)
  {
    Serial.println(F("Failed to read config file"));
    server.send(400, "plain/text", "Error deserializing message");
    return;
  }

  enableMqtt = doc["enableMqtt"];
  mqttIpAddress[0] = doc["mqttIpAddress"][0];
  mqttIpAddress[1] = doc["mqttIpAddress"][1];
  mqttIpAddress[2] = doc["mqttIpAddress"][2];
  mqttIpAddress[3] = doc["mqttIpAddress"][3];
  mqttPort = doc["mqttPort"];
  mqttUsername = doc["mqttUsername"].as<String>();
  mqttPassword = doc["mqttPassword"].as<String>();
  mqttClientId = doc["mqttClientId"].as<String>();
  mqttBaseTopic = doc["mqttBaseTopic"].as<String>();
	
  saveMqtt();

  server.send(200, "plain/text", "");
}

/**
 * response for /dir/
 * web server prints a list of files
 */
void handleDir()
{
  String mydir;
  Dir root = LittleFS.openDir("/");
  while (root.next())
  {
    Serial.println(root.fileName());
    mydir += root.fileName() + F(" \t Size: ");
    mydir += String(root.fileSize()) + F(" Bytes\n");
  }
  server.send(200, "text/plain", mydir);
}

/**
 * response for /upload.html
 * upload a new file to the LittleFS
 */
void handleFileUpload()
{
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START)
  {
    path = upload.filename;
    if (!path.startsWith("/"))
    {
      path = "/" + path;
    }

    // The file server always prefers a compressed version of a file
    if (!path.endsWith(".gz"))
    {
      // So if an uploaded file is not compressed, the existing compressed
      String pathWithGz = path + ".gz";
      // version of that file must be deleted (if it exists)
      if (LittleFS.exists(pathWithGz))
      {
        LittleFS.remove(pathWithGz);
      }
    }

    Serial.print(F("handleFileUpload Name: "));
    Serial.println(path);

    // Open the file for writing in LittleFS (create if it doesn't exist)
    fsUploadFile = LittleFS.open(path, "w");
    path = String();
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (fsUploadFile)
    {
      // Write the received bytes to the file
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
    {
      fsUploadFile.close();
      Serial.print(F("handleFileUpload Size: "));
      Serial.println(upload.totalSize);

      server.sendHeader("Location", "/success.html");
      server.send(303);

	    if (upload.filename == "cmdq.txt")
      {
        bwc.reloadCommandQueue();
      }
	    if (upload.filename == "settings.txt")
      {
        bwc.reloadSettings();
      }
    }
    else
    {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

/**
 * response for /remove.html
 * delete a file from the LittleFS
 */
void handleFileRemove()
{
  String path;
  path = server.arg("FileToRemove");
  if (!path.startsWith("/"))
  {
    path = "/" + path;
  }
  
  Serial.print(F("handleFileRemove Name: "));
  Serial.println(path);
  
  if (LittleFS.exists(path) && LittleFS.remove(path))
  {
    Serial.print(F("handleFileRemove success: "));
    Serial.println(path);
    server.sendHeader("Location", "/success.html");
    server.send(303);
  }
  else
  {
    Serial.print(F("handleFileRemove error: "));
    Serial.println(path);
    server.send(500, "text/plain", "500: couldn't delete file");
  }
}

/**
 * response for /restart/
 */
void handleRestart()
{
  // TODO: browser tab must move from /restart/ otherwise its an endless restart loop
  server.sendHeader("Location", "/"); // this does not work..
  server.send(303);

  Serial.println("ESP restart ...");
  ESP.restart();
}












/*
   MQTT functions
*/

void startMQTT() { //MQTT setup and connect - 877dev
  //load mqtt credential file if it exists, and update default strings  ********************
  loadMqtt();

  MQTTclient.setServer(mqttIpAddress, mqttPort); //setup MQTT broker information as defined earlier
  if (MQTTclient.setBufferSize (1024))      //set buffer for larger messages, new to library 2.8.0
  {
    Serial.println(F("MQTT buffer size successfully increased"));
  }
  MQTTclient.setKeepAlive(60);
  MQTTclient.setSocketTimeout(30);
  MQTTclient.setCallback(MQTTcallback);          // set callback details - this function is called automatically whenever a message arrives on a subscribed topic.
  //MQTT_Connect();                                //Connect to MQTT broker, publish Status/MAC/count, and subscribe to keypad topic.
}


void MQTTcallback(char* topic, byte* payload, unsigned int length) {  //877dev

  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (String(topic).equals(String(mqttBaseTopic) + "/command")) {
    DynamicJsonDocument doc(256);
    // Deserialize the JSON document
    String message = (const char *) &payload[0];
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
      return;
    }

    // Copy values from the JsonDocument to the Config
    uint32_t command = doc["CMD"];
    uint32_t value = doc["VALUE"];
    uint32_t xtime = doc["XTIME"];
    uint32_t interval = doc["INTERVAL"];
    bwc.qCommand(command, value, xtime, interval);
  }
} // End of void MQTTcallback

void MQTT_Connect()
{
  Serial.print(F("Connecting to MQTT...  "));
  // We'll connect with a Retained Last Will that updates the '.../Status' topic with "Dead" when the device goes offline...
  // Attempt to connect...
  /*
    MQTT Connection syntax:
    boolean connect (client_id, username, password, willTopic, willQoS, willRetain, willMessage)
    Connects the client with a Will message, username and password specified.
    Parameters
    client_id : the client ID to use when connecting to the server.
    username : the username to use. If NULL, no username or password is used (const char[])
    password : the password to use. If NULL, no password is used (const char[])
    willTopic : the topic to be used by the will message (const char[])
    willQoS : the quality of service to be used by the will message (int : 0,1 or 2)
    willRetain : whether the will should be published with the retain flag (int : 0 or 1)
    willMessage : the payload of the will message (const char[])
    Returns
    false - connection failed.
    true - connection succeeded
  */
  if (MQTTclient.connect(mqttClientId.c_str(), mqttUsername.c_str(), mqttPassword.c_str(), (String(mqttBaseTopic) + "/Status").c_str(), 0, 1, "Dead"))
  {
    // We get here if the connection was successful...
    mqtt_connect_count++;
    Serial.println(F("CONNECTED!"));
    // Once connected, publish some announcements...
    // These all have the Retained flag set to true, so that the value is stored on the server and can be retrieved at any point
    // Check the .../Status topic to see that the device is still online before relying on the data from these retained topics
    MQTTclient.publish((String(mqttBaseTopic) + "/Status").c_str(), "Alive", true);
    MQTTclient.publish((String(mqttBaseTopic) + "/MAC_Address").c_str(), WiFi.macAddress().c_str(), true);                 // Device MAC Address
    MQTTclient.publish((String(mqttBaseTopic) + "/MQTT_Connect_Count").c_str(), String(mqtt_connect_count).c_str(), true); // MQTT Connect Count
    MQTTclient.loop();


    // ... and then re/subscribe to the watched topics
    MQTTclient.subscribe((String(mqttBaseTopic) + "/command").c_str());   // Watch the .../command topic for incoming MQTT messages
    MQTTclient.loop();
    // Add other watched topics in here...
  }
  else
  {
    // We get here if the connection failed...
    Serial.print(F("MQTT Connection FAILED, Return Code = "));
    Serial.println(MQTTclient.state());
    Serial.println();
    /*
      MQTTclient.state return code meanings...
      -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
      -3 : MQTT_CONNECTION_LOST - the network connection was broken
      -2 : MQTT_CONNECT_FAILED - the network connection failed
      -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
      0 : MQTT_CONNECTED - the client is connected
      1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
      2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
      3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
      4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
      5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect *
    */
  }
} // End of void MQTT_Connect