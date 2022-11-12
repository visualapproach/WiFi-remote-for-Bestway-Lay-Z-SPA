#include "main.h"

// initial stack
char *stack_start;

void setup()
{
  // init record of stack
  char stack;
  stack_start = &stack;

  pinMode(solarpin, INPUT_PULLUP);
  pinMode(myoutputpin, OUTPUT);
  digitalWrite(myoutputpin, LOW);
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  bwc.begin(); //no params = default pins

  // check things in a cycle
  periodicTimer.attach(periodicTimerInterval, []{ periodicTimerFlag = true; });

  // run once after setup (should be) is done
  startTimer.attach(120, []{ if(useMqtt) enableMqtt = true; startTimer.detach(); });

  // update webpage every 2 seconds. (will also be updated on state changes)
  updateWSTimer.attach(2.0, []{ sendWSFlag = true; });

  // needs to be loaded here for reading the wifi.json
  LittleFS.begin();
  loadWifi();

  startWiFi();
  startNTP();
  startOTA();
  startHttpServer();
  startWebSocket();
  startMqtt();

  /*
    Blinking LED feedback removed.
    See JSON "BCC" (bad cio checksum) and
    "BDC" (display) messages instead
  */

  Serial.println(F("End of setup()"));
}

void loop()
{
  // calc looptime
  static bool firstloopdone = false;
  unsigned long ms = millis();
  unsigned long looptime;
  if(ms>prevlooptime && firstloopdone){
    looptime = ms - prevlooptime;
    looptime_min = min(looptime, looptime_min);
    looptime_max = max(looptime, looptime_max);
  }
  prevlooptime = ms;
  firstloopdone = true;
  // We need this self-destructing info several times, so save it locally
  bool newData = bwc.newData();
  // Fiddle with the pump computer
  bwc.loop();

  // run only when a wifi connection is established
  if (WiFi.status() == WL_CONNECTED)
  {
    // listen for websocket events
    // webSocket.loop();
    // listen for webserver events
    server.handleClient();
    // listen for OTA events
    ArduinoOTA.handle();

    // MQTT
    if (enableMqtt && mqttClient.loop())
    {
      if (newData)
      {
        sendMQTT();
      }
      else if (sendMQTTFlag)
      {
        sendMQTTFlag = false;
        sendMQTT();
      }
    }

    // web socket
    if (newData)
    {
      sendWS();
    }
    else if (sendWSFlag)
    {
      sendWSFlag = false;
      sendWS();
    }

    // run once after connection was established
    if (!wifiConnected)
    {
      Serial.println(F("WiFi > Connected"));
      Serial.println(" SSID: \"" + WiFi.SSID() + "\"");
      Serial.println(" IP: \"" + WiFi.localIP().toString() + "\"");
      startOTA();
      startHttpServer();
      startWebSocket();
    }
    // reset marker
    wifiConnected = true;
  }

  // run only when the wifi connection got lost
  if (WiFi.status() != WL_CONNECTED)
  {
    // run once after connection was lost
    if (wifiConnected)
    {
      Serial.println(F("WiFi > Lost connection. Trying to reconnect ..."));
    }
    // set marker
    wifiConnected = false;
  }

  // run every X seconds
  if (periodicTimerFlag)
  {
    periodicTimerFlag = false;

    if (WiFi.status() != WL_CONNECTED)
    {
      bwc.print(F("check network"));
      Serial.println(F("WiFi > Trying to reconnect ..."));
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      if (!DateTime.isTimeValid())
      {
        Serial.println(F("NTP > Start synchronisation"));
        DateTime.begin(5000);
      }

      if (enableMqtt && !mqttClient.loop())
      {
        Serial.println(F("MQTT > Not connected"));
        mqttConnect();
      }
    }
  }

  //handleAUX();

  // You can add own code here, but don't stall!
  // If CPU is choking you can try to run @ 160 MHz, but that's cheating!
}



void handleAUX()
{
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



/**
 * Send status data to web client in JSON format (because it is easy to decode on the other side)
 */
void sendWS()
{
  String json;

  // send states
  json = bwc.getJSONStates();
  webSocket.broadcastTXT(json);

  // send times
  json = bwc.getJSONTimes();
  webSocket.broadcastTXT(json);

  if(DEBUGSERIAL)
  {
    json = bwc.getSerialBuffers();
    webSocket.broadcastTXT(json);
  }

  // send other info
  json = getOtherInfo();
  webSocket.broadcastTXT(json);
}

String getOtherInfo()
{
  DynamicJsonDocument doc(1024);
  String json = "";
  // Set the values in the document
  doc["CONTENT"] = "OTHER";
  doc["MQTT"] = mqttClient.state();
  doc["CIOTX"] = bwc.cio_tx_ok;
  doc["DSPTX"] = bwc.dsp_tx_ok;
  doc["HASJETS"] = HASJETS;
  doc["HASAIR"] = HASAIR;
  doc["RSSI"] = WiFi.RSSI();
  doc["IP"] = WiFi.localIP().toString();
  doc["SSID"] = WiFi.SSID();
  doc["FW"] = FW_VERSION;
  doc["MODEL"] = MYMODEL;
  doc["LOOPMAX"] = looptime_max;
  doc["LOOPMIN"] = looptime_min;

  // Serialize JSON to string
  if (serializeJson(doc, json) == 0)
  {
    json = "{\"error\": \"Failed to serialize message\"}";
  }
  return json;
}

/**
 * Send STATES and TIMES to MQTT
 * It would be more elegant to send both states and times on the "message" topic
 * and use the "CONTENT" field to distinguish between them
 * but it might break peoples home automation setups, so to keep it backwards
 * compatible I choose to start a new topic "/times"
 * @author 877dev
 */
void sendMQTT()
{
  String json;

  // send states
  json = bwc.getJSONStates();
  if (mqttClient.publish((String(mqttBaseTopic) + "/message").c_str(), String(json).c_str(), true))
  {
    //Serial.println(F("MQTT > message published"));
  }
  else
  {
    //Serial.println(F("MQTT > message not published"));
  }

  // send times
  json = bwc.getJSONTimes();
  if (mqttClient.publish((String(mqttBaseTopic) + "/times").c_str(), String(json).c_str(), true))
  {
    //Serial.println(F("MQTT > times published"));
  }
  else
  {
    //Serial.println(F("MQTT > times not published"));
  }


  //send other info
  json = getOtherInfo();
  if (mqttClient.publish((String(mqttBaseTopic) + "/other").c_str(), String(json).c_str(), true))
  {
    //Serial.println(F("MQTT > other published"));
  }
  else
  {
    //Serial.println(F("MQTT > other not published"));
  }
}



/**
 * Start a Wi-Fi access point, and try to connect to some given access points.
 * Then wait for either an AP or STA connection
 */
void startWiFi()
{
  //WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.hostname(netHostname);

  if (enableStaticIp4)
  {
    Serial.println("WiFi > using static IP \"" + ip4Address.toString() + "\" on gateway \"" + ip4Gateway.toString() + "\"");
    WiFi.config(ip4Address, ip4Gateway, ip4Subnet, ip4DnsPrimary, ip4DnsSecondary);
  }

  if (enableAp)
  {
    Serial.println("WiFi > using WiFi configuration with SSID \"" + apSsid + "\"");

    WiFi.begin(apSsid, apPwd);

    Serial.print(F("WiFi > Trying to connect ..."));
    int maxTries = 10;
    int tryCount = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.print(".");
      tryCount++;

      if (tryCount >= maxTries)
      {
        Serial.println("");
        Serial.println(F("WiFi > NOT connected!"));
        if (enableWmApFallback)
        {
          // disable specific WiFi config
          enableAp = false;
          enableStaticIp4 = false;
          // fallback to WiFi config portal
          startWiFiConfigPortal();
        }
        break;
      }
      Serial.println("");
    }
  }
  else
  {
    startWiFiConfigPortal();
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    enableAp = true;
    apSsid = WiFi.SSID();
    apPwd = WiFi.psk();
    saveWifi();

    wifiConnected = true;

    Serial.println(F("WiFi > Connected."));
    Serial.println(" SSID: \"" + WiFi.SSID() + "\"");
    Serial.println(" IP: \"" + WiFi.localIP().toString() + "\"");
  }
  else
  {
    Serial.println("WiFi > Connection failed. Retrying in a while ...");
  }
}

/**
 * start WiFiManager configuration portal
 */
void startWiFiConfigPortal()
{
  Serial.println(F("WiFi > Using WiFiManager Config Portal"));
  wm.autoConnect(wmApName, wmApPassword);
  Serial.print(F("WiFi > Trying to connect ..."));
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
}



/**
 * start NTP sync
 */
void startNTP()
{
  DateTime.setServer("pool.ntp.org");
  DateTime.begin(5000);
}



/**
 * Start the OTA service
 */
void startOTA()
{
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println(F("OTA > Start"));
    //bwc.stop(); // TODO: 6-wire only(?)
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("OTA > End"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA > Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA > Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
  });
  ArduinoOTA.begin();
  Serial.println(F("OTA > ready"));
}



/**
 * start a web socket server
 */
void startWebSocket()
{
  // In case we are already running
  webSocket.close();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println(F("WebSocket > server started"));
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
      Serial.printf("WebSocket > [%u] Disconnected!\r\n", num);
      break;

    // if a new websocket connection is established
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("WebSocket > [%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        sendWS();
      }
      break;

    // if new text data is received
    case WStype_TEXT:
      {
        Serial.printf("WebSocket > [%u] get Text: %s\r\n", num, payload);
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
          Serial.println(F("WebSocket > JSON command failed"));
          return;
        }

        // Copy values from the JsonDocument to the Config
        int64_t command = doc["CMD"];
        int64_t value = doc["VALUE"];
        int64_t xtime = doc["XTIME"];
        int64_t interval = doc["INTERVAL"];
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
void startHttpServer()
{
  // In case we are already running
  server.stop();
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
  server.on(F("/info/"), handleESPInfo);
  // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists
  server.onNotFound(handleNotFound);
  // start the HTTP server
  server.begin();
  Serial.println(F("HTTP > server started"));
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
    if (!server.authenticate(authUsername.c_str(), authPassword.c_str()))
    {
      server.requestAuthentication();
    }
  }

  Serial.println("HTTP > request: " + path);
  // If a folder is requested, send the index file
  if (path.endsWith("/"))
  {
    path += F("index.html");
  }
  // deny reading credentials
  if (path.equalsIgnoreCase("/mqtt.json") || path.equalsIgnoreCase("/wifi.json"))
  {
    server.send(403, "text/plain", "Permission denied.");
    Serial.println(F("HTTP > file reading denied (credentials)."));
    return false;
  }
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (LittleFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed version
    File file = LittleFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println("HTTP > file sent: " + path + " (" + sent + " bytes)");
    return true;
  }
  Serial.println("HTTP > file not found: " + path);   // If the file doesn't exist, return false
  return false;
}

/**
 * checks the method to be a POST
 */
bool checkHttpPost(HTTPMethod method)
{
  if (method != HTTP_POST)
  {
    server.send(405, "text/plain", "Method not allowed.");
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

  server.send(200, "text/plain", "");
}

/**
 * response for /getcommands/
 * web server prints a json document
 */
void handleGetCommandQueue()
{
  if (!checkHttpPost(server.method())) return;

  String json = bwc.getJSONCommandQueue();
  server.send(200, "application/json", json);
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
    server.send(400, "text/plain", "Error deserializing message");
    return;
  }

  int64_t command = doc["CMD"];
  int64_t value = doc["VALUE"];
  int64_t xtime = doc["XTIME"];
  int64_t interval = doc["INTERVAL"];

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
    Serial.println(F("{\"error\": \"Failed to serialize file\"}"));
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
  if (!hidePasswords)
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
  server.send(200, "application/json", json);
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
    server.send(400, "text/plain", "Error deserializing message");
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

  server.send(200, "text/plain", "");
}

/**
 * response for /resetwifi/
 * do this before giving away the device (be aware of other credentials e.g. MQTT)
 * a complete flash erase should do the job but remember to upload the filesystem as well.
 */
void handleResetWifi()
{
  server.send(200, F("text/html"), F("WiFi connection reset (erase) ..."));
  Serial.println(F("WiFi connection reset (erase) ..."));

  periodicTimer.detach();
  updateMqttTimer.detach();
  updateWSTimer.detach();
  //bwc.stop(); // TODO: 6-wire only(?)
  bwc.saveSettings();
  delay(1000);

  ESP.eraseConfig();
  delay(1000);

  enableAp = false;
  apSsid = "empty";
  apPwd = "empty";
  saveWifi();
  delay(1000);

  wm.resetSettings();
  //WiFi.disconnect();
  delay(1000);

  server.send(200, F("text/html"), F("WiFi connection reset (erase) ... done."));
  Serial.println(F("WiFi connection reset (erase) ... done."));
  Serial.println(F("ESP reset ..."));
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

  useMqtt = doc["enableMqtt"];
  mqttIpAddress[0] = doc["mqttIpAddress"][0];
  mqttIpAddress[1] = doc["mqttIpAddress"][1];
  mqttIpAddress[2] = doc["mqttIpAddress"][2];
  mqttIpAddress[3] = doc["mqttIpAddress"][3];
  mqttPort = doc["mqttPort"];
	mqttUsername = doc["mqttUsername"].as<String>();
	mqttPassword = doc["mqttPassword"].as<String>();
	mqttClientId = doc["mqttClientId"].as<String>();
	mqttBaseTopic = doc["mqttBaseTopic"].as<String>();
  mqttTelemetryInterval = doc["mqttTelemetryInterval"];
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

  doc["enableMqtt"] = useMqtt;
  doc["mqttIpAddress"][0] = mqttIpAddress[0];
  doc["mqttIpAddress"][1] = mqttIpAddress[1];
  doc["mqttIpAddress"][2] = mqttIpAddress[2];
  doc["mqttIpAddress"][3] = mqttIpAddress[3];
  doc["mqttPort"] = mqttPort;
  doc["mqttUsername"] = mqttUsername;
  doc["mqttPassword"] = mqttPassword;
  doc["mqttClientId"] = mqttClientId;
  doc["mqttBaseTopic"] = mqttBaseTopic;
  doc["mqttTelemetryInterval"] = mqttTelemetryInterval;

  if (serializeJson(doc, file) == 0)
  {
    Serial.println(F("{\"error\": \"Failed to serialize file\"}"));
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

  doc["enableMqtt"] = useMqtt;
  doc["mqttIpAddress"][0] = mqttIpAddress[0];
  doc["mqttIpAddress"][1] = mqttIpAddress[1];
  doc["mqttIpAddress"][2] = mqttIpAddress[2];
  doc["mqttIpAddress"][3] = mqttIpAddress[3];
  doc["mqttPort"] = mqttPort;
  doc["mqttUsername"] = mqttUsername;
  doc["mqttPassword"] = "<enter password>";
  if (!hidePasswords)
  {
    doc["mqttPassword"] = mqttPassword;
  }
  doc["mqttClientId"] = mqttClientId;
  doc["mqttBaseTopic"] = mqttBaseTopic;
  doc["mqttTelemetryInterval"] = mqttTelemetryInterval;

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
    server.send(400, "text/plain", "Error deserializing message");
    return;
  }

  useMqtt = doc["enableMqtt"];
  mqttIpAddress[0] = doc["mqttIpAddress"][0];
  mqttIpAddress[1] = doc["mqttIpAddress"][1];
  mqttIpAddress[2] = doc["mqttIpAddress"][2];
  mqttIpAddress[3] = doc["mqttIpAddress"][3];
  mqttPort = doc["mqttPort"];
  mqttUsername = doc["mqttUsername"].as<String>();
  mqttPassword = doc["mqttPassword"].as<String>();
  mqttClientId = doc["mqttClientId"].as<String>();
  mqttBaseTopic = doc["mqttBaseTopic"].as<String>();
  mqttTelemetryInterval = doc["mqttTelemetryInterval"];

  server.send(200, "text/plain", "");

  saveMqtt();
  startMqtt();

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
    mydir += "<a href=\"/" + root.fileName() + "\">" + root.fileName() + "</a>" + F(" \t Size: ");
    mydir += String(root.fileSize()) + F(" Bytes<br>");
  }
  server.send(200, "text/html", mydir);
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
	    // TODO: this is 6-wire only(?)
      //if (upload.filename == "settings.txt")
      //{
      //  bwc.reloadSettings();
      //}
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
  server.send(200, F("text/html"), F("ESP restart ..."));

  server.sendHeader("Location", "/");
  server.send(303);

  delay(1000);
  periodicTimer.detach();
  updateMqttTimer.detach();
  updateWSTimer.detach();
  //bwc.stop(); // TODO: 6-wire only(?)
  bwc.saveSettings();
  delay(1000);
  Serial.println(F("ESP restart ..."));
  ESP.restart();
}

void handleESPInfo()
{
  char stack;
  uint32_t stacksize = stack_start - &stack;
  size_t const BUFSIZE = 1024;
  char response[BUFSIZE];
  char const *response_template =
  "Stack size:          %u \n"
  "Free Heap:           %u \n"
  "Core version:        %s \n"
  "CPU fq:              %u MHz\n"
  "Cycle count:         %u \n"
  "Free cont stack:     %u \n"
  "Sketch size:         %u \n"
  "Free sketch space:   %u \n"
  "Max free block size: %u \n";

  snprintf(response, BUFSIZE, response_template,
    stacksize,
    ESP.getFreeHeap(),
    ESP.getCoreVersion(),
    ESP.getCpuFreqMHz(),
    ESP.getCycleCount(),
    ESP.getFreeContStack(),
    ESP.getSketchSize(),
    ESP.getFreeSketchSpace(),
    ESP.getMaxFreeBlockSize() );
    server.send(200, "text/plain; charset=utf-8", response);

}

/**
 * MQTT setup and connect
 * @author 877dev
 */
void startMqtt()
{
  // load mqtt credential file if it exists, and update default strings
  loadMqtt();

  // disconnect in case we are already connected
  mqttClient.disconnect();
  // setup MQTT broker information as defined earlier
  mqttClient.setServer(mqttIpAddress, mqttPort);
  // set buffer for larger messages, new to library 2.8.0
  if (mqttClient.setBufferSize(1536))
  {
    Serial.println(F("MQTT > Buffer size successfully increased"));
  }
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(30);
  // set callback details
  // this function is called automatically whenever a message arrives on a subscribed topic.
  mqttClient.setCallback(mqttCallback);
  // Connect to MQTT broker, publish Status/MAC/count, and subscribe to keypad topic.
  mqttConnect();
}

/**
 * MQTT callback function
 * @author 877dev
 */
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print(F("MQTT > Message arrived ["));
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (String(topic).equals(String(mqttBaseTopic) + "/command"))
  {
    DynamicJsonDocument doc(256);
    String message = (const char *) &payload[0];
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
      return;
    }

    int64_t command = doc["CMD"];
    int64_t value = doc["VALUE"];
    int64_t xtime = doc["XTIME"];
    int64_t interval = doc["INTERVAL"];
    bwc.qCommand(command, value, xtime, interval);
  }
}

/**
 * Connect to MQTT broker, publish Status/MAC/count, and subscribe to keypad topic.
 */
void mqttConnect()
{
  // do not connect if MQTT is not enabled
  if (!enableMqtt)
  {
    return;
  }

  Serial.print(F("MQTT > Connecting ... "));
  // We'll connect with a Retained Last Will that updates the 'Status' topic with "Dead" when the device goes offline...
  if (mqttClient.connect(
    mqttClientId.c_str(), // client_id : the client ID to use when connecting to the server.
    mqttUsername.c_str(), // username : the username to use. If NULL, no username or password is used (const char[])
    mqttPassword.c_str(), // password : the password to use. If NULL, no password is used (const char[])
    (String(mqttBaseTopic) + "/Status").c_str(), // willTopic : the topic to be used by the will message (const char[])
    0, // willQoS : the quality of service to be used by the will message (int : 0,1 or 2)
    1, // willRetain : whether the will should be published with the retain flag (int : 0 or 1)
    "Dead")) // willMessage : the payload of the will message (const char[])
  {
    Serial.println(F("success!"));
    mqtt_connect_count++;

    // update MQTT every X seconds. (will also be updated on state changes)
    updateMqttTimer.attach(mqttTelemetryInterval, []{ sendMQTTFlag = true; });

    // These all have the Retained flag set to true, so that the value is stored on the server and can be retrieved at any point
    // Check the 'Status' topic to see that the device is still online before relying on the data from these retained topics
    mqttClient.publish((String(mqttBaseTopic) + "/Status").c_str(), "Alive", true);
    mqttClient.publish((String(mqttBaseTopic) + "/MAC_Address").c_str(), WiFi.macAddress().c_str(), true);                 // Device MAC Address
    mqttClient.publish((String(mqttBaseTopic) + "/MQTT_Connect_Count").c_str(), String(mqtt_connect_count).c_str(), true); // MQTT Connect Count
    mqttClient.loop();

    // Watch the 'command' topic for incoming MQTT messages
    mqttClient.subscribe((String(mqttBaseTopic) + "/command").c_str());
    mqttClient.loop();

    mqttClient.publish((String(mqttBaseTopic) + "/reboot_time").c_str(), reboottime.c_str(), true);
    mqttClient.publish((String(mqttBaseTopic) + "/reboot_reason").c_str(), ESP.getResetReason().c_str(), true);
    mqttClient.loop();
    sendMQTT();
    setupHA(); //Setup MQTT Device
  }
  else
  {
    Serial.print(F("failed, Return Code = "));
    Serial.println(mqttClient.state()); // states explained in WebSocket.js
  }
}

void setupHA()
{
  String topic;
  String payload;
  String mychipid = String((unsigned int)ESP.getChipId());
  int maxtemp, mintemp;
  maxtemp = 40;
  mintemp = 20;
  DynamicJsonDocument devicedoc(512);
  DynamicJsonDocument doc(2048);
  devicedoc["dev"]["configuration_url"] = "http://" + WiFi.localIP().toString();
  devicedoc["dev"]["connections"].add(serialized("[\"mac\",\"" + WiFi.macAddress()+"\"]" ));
  devicedoc["dev"]["identifiers"] = mychipid;
  devicedoc["dev"]["manufacturer"] = F("Visualapproach");
  devicedoc["dev"]["model"] = MYMODEL;
  devicedoc["dev"]["name"] = F("Layzspa WiFi controller 4w");
  devicedoc["dev"]["sw_version"] = FW_VERSION;

  // reboot time sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_reboot_time/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa reboot time");
  doc["uniq_id"] = "sensor.layzspa_4w_reboot_time"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/reboot_time");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // reboot reason sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_reboot_reason/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa reboot reason");
  doc["uniq_id"] = "sensor.layzspa_4w_reboot_reason"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/reboot_reason");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = "Alive";
  doc["pl_not_avail"] = "Dead";
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // WiFi SSID sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_ssid/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa ssid");
  doc["uniq_id"] = "sensor.layzspa_4w_ssid"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/other");
  doc["val_tpl"] = F("{{ value_json.SSID }}");
  doc["exp_aft"] = 700;
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // WiFi RSSI sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_rssi/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa rssi");
  doc["uniq_id"] = "sensor.layzspa_4w_rssi"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/other");
  doc["val_tpl"] = F("{{ value_json.RSSI }}");
  doc["unit_of_meas"] = F("dBm");
  doc["exp_aft"] = 700;
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // WiFi local ip sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_ip/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa ip");
  doc["uniq_id"] = "sensor.layzspa_4w_ip"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/other");
  doc["val_tpl"] = F("{{ value_json.IP }}");
  doc["exp_aft"] = 700;
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();


  // connect count sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_connect_count/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa connect count");
  doc["uniq_id"] = "sensor.layzspa_4w_connect_count"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/MQTT_Connect_Count");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // // spa time to ready sensor
  // doc["dev"] = devicedoc["dev"];
  // payload = "";
  // topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_time_to_ready/config");
  // Serial.println(topic);
  // doc["name"] = F("Layzspa time to ready");
  // doc["uniq_id"] = "sensor.layzspa_4w_time_to_ready"+mychipid;
  // doc["stat_t"] = mqttBaseTopic+F("/times");
  // doc["unit_of_meas"] = F("hours");
  // doc["val_tpl"] = F("{{ value_json.T2R }}");
  // doc["exp_aft"] = 700;
  // doc["icon"] = F("mdi:clock");
  // doc["avty_t"] = mqttBaseTopic+F("/Status");
  // doc["pl_avail"] = F("Alive");
  // doc["pl_not_avail"] = F("Dead");
  // if (serializeJson(doc, payload) == 0)
  // {
  //   Serial.println(F("Failed to serialize HA message!"));
  //   return;
  // }
  // mqttClient.publish(topic.c_str(), payload.c_str(), true);
  // mqttClient.loop();
  // Serial.println(payload);
  // doc.clear();
  // doc.garbageCollect();


  // spa energy sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_energy/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa energy");
  doc["uniq_id"] = "sensor.layzspa_4w_energy"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/times");
  doc["unit_of_meas"] = F("kWh");
  doc["val_tpl"] = F("{{ value_json.KWH | round(3) }}");
  doc["dev_cla"] = F("energy");
  doc["state_class"] = F("total_increasing");
  doc["expire_after"] = 700;
  doc["icon"] = F("mdi:flash");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa daily energy sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_today/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa today");
  doc["uniq_id"] = "sensor.layzspa_4w_today"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/times");
  doc["unit_of_meas"] = F("kWh");
  doc["val_tpl"] = F("{{ value_json.KWHD | round(3) }}");
  doc["dev_cla"] = F("energy");
  doc["state_class"] = F("total_increasing");
  doc["expire_after"] = 700;
  doc["icon"] = F("mdi:flash");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa power sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_power/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa power");
  doc["uniq_id"] = "sensor.layzspa_4w_power"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/times");
  doc["unit_of_meas"] = F("W");
  doc["val_tpl"] = F("{{ value_json.WATT | int }}");
  doc["dev_cla"] = F("power");
  doc["state_class"] = F("measurement");
  doc["expire_after"] = 700;
  doc["icon"] = F("mdi:flash");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa chlorine age sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_chlorine_age/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa chlorine age");
  doc["uniq_id"] = "sensor.layzspa_4w_chlorine_age"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/times");
  doc["unit_of_meas"] = F("days");
  doc["val_tpl"] = F("{{ ( ( (now().timestamp()|int) - value_json.CLTIME|int)/3600/24) | round(2) }}");
  doc["exp_aft"] = 700;
  doc["icon"] = F("hass:hand-coin-outline");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa filter age sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_filter_age/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa filter age");
  doc["uniq_id"] = "sensor.layzspa_4w_filter_age"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/times");
  doc["unit_of_meas"] = F("days");
  doc["val_tpl"] = F("{{ ( ( (now().timestamp()|int) - value_json.FTIME|int)/3600/24) | round(2) }}");
  doc["exp_aft"] = 700;
  doc["icon"] = F("hass:air-filter");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa uptime sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_uptime/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa uptime");
  doc["uniq_id"] = "sensor.layzspa_4w_uptime"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/times");
  doc["unit_of_meas"] = F("days");
  doc["val_tpl"] = F("{{ ( (value_json.UPTIME|int)/3600/24) | round(2) }}");
  doc["exp_aft"] = 700;
  doc["icon"] = F("mdi:clock-outline");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa pump time sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_pumptime/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa pump time");
  doc["uniq_id"] = "sensor.layzspa_4w_pumptime"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/times");
  doc["unit_of_meas"] = F("hours");
  doc["val_tpl"] = F("{{ ( (value_json.PUMPTIME|int)/3600) | round(2) }}");
  doc["exp_aft"] = 700;
  doc["icon"] = F("mdi:clock-outline");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa heater time sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_heatertime/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa heater time");
  doc["uniq_id"] = "sensor.layzspa_4w_heatertime"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/times");
  doc["unit_of_meas"] = F("hours");
  doc["val_tpl"] = F("{{ ( (value_json.HEATINGTIME|int)/3600) | round(2) }}");
  doc["exp_aft"] = 700;
  doc["icon"] = F("mdi:clock-outline");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa air time sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_airtime/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa air time");
  doc["uniq_id"] = "sensor.layzspa_4w_airtime"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/times");
  doc["unit_of_meas"] = F("hours");
  doc["val_tpl"] = F("{{ ( (value_json.AIRTIME|int)/3600) | round(2) }}");
  doc["exp_aft"] = 700;
  doc["icon"] = F("mdi:clock-outline");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa heater binary_sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/binary_sensor/layzspa_4w_heater/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa heater");
  doc["uniq_id"] = "binary_sensor.layzspa_4w_heater"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/message");
  doc["val_tpl"] = F("{% if value_json.RED == 1 %}ON{% else %}OFF{% endif %}");
  doc["dev_cla"] = F("heat");
  doc["exp_aft"] = 700;
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa ready binary_sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/binary_sensor/layzspa_4w_ready/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa ready");
  doc["uniq_id"] = "binary_sensor.layzspa_4w_ready"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/message");
  doc["val_tpl"] = F("{% if value_json.TMP > 30 %}{% if value_json.TMP >= value_json.TGT-1 %}ON{% else %}OFF{% endif %}{% else %}OFF{% endif %}");
  doc["exp_aft"] = 700;
  doc["icon"] = F("mdi:hot-tub");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa connection status binary_sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/binary_sensor/layzspa_4w_connection/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa connection");
  doc["uniq_id"] = "binary_sensor.layzspa_4w_connection"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/Status");
  doc["dev_cla"] = F("connectivity");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  doc["payload_on"] = "Alive";
  doc["payload_off"] = "Dead";
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa 4 wire Error status binary_sensor
  doc["device"] = devicedoc["device"];
  payload = "";
  topic = String(HA_PREFIX) + F("/binary_sensor/layzspa_4w_error/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa Error");
  doc["uniq_id"] = "binary_sensor.layzspa_4w_error"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/message");
  doc["val_tpl"] = F("{{ value_json.ERR }}");
  doc["dev_cla"] = F("problem");
  doc["ent_cat"] = F("diagnostic");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  doc["payload_on"] = 1;
  doc["payload_off"] = 0;
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa heat regulation switch
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/switch/layzspa_4w_heat_regulation/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa heat regulation");
  doc["uniq_id"] = "switch.layzspa_4w_heat_regulation"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/message");
  doc["command_topic"] = mqttBaseTopic+F("/command");
  doc["val_tpl"] = F("{% if value_json.RED == 1 %}1{% elif value_json.GRN == 1 %}1{% else %}0{% endif %}");
  doc["exp_aft"] = 700;
  doc["icon"] = F("mdi:radiator");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  doc["payload_on"] = F("{CMD:3,VALUE:true,XTIME:0,INTERVAL:0}");
  doc["payload_off"] = F("{CMD:3,VALUE:false,XTIME:0,INTERVAL:0}");
  doc["state_on"] = 1;
  doc["state_off"] = 0;
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa waterjets switch
  if(HASJETS)
  {
    doc["dev"] = devicedoc["dev"];
    payload = "";
    topic = String(HA_PREFIX) + F("/switch/layzspa_4w_jets/config");
    Serial.println(topic);
    doc["name"] = F("Layzspa jets");
    doc["uniq_id"] = "switch.layzspa_4w_jets"+mychipid;
    doc["stat_t"] = mqttBaseTopic+F("/message");
    doc["command_topic"] = mqttBaseTopic+F("/command");
    doc["val_tpl"] = F("{{ value_json.HJT }}");
    doc["exp_aft"] = 700;
    doc["icon"] = F("mdi:hydro-power");
    doc["avty_t"] = mqttBaseTopic+F("/Status");
    doc["pl_avail"] = F("Alive");
    doc["pl_not_avail"] = F("Dead");
    doc["payload_on"] = F("{CMD:11,VALUE:true,XTIME:0,INTERVAL:0}");
    doc["payload_off"] = F("{CMD:11,VALUE:false,XTIME:0,INTERVAL:0}");
    doc["state_on"] = 1;
    doc["state_off"] = 0;
    if (serializeJson(doc, payload) == 0)
    {
      Serial.println(F("Failed to serialize HA message!"));
      return;
    }
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
    mqttClient.loop();
    Serial.println(payload);
    doc.clear();
    doc.garbageCollect();
  }

  // spa airbubbles switch
  if(HASAIR)
  {
    doc["dev"] = devicedoc["dev"];
    payload = "";
    topic = String(HA_PREFIX) + F("/switch/layzspa_4w_airbubbles/config");
    Serial.println(topic);
    doc["name"] = F("Layzspa airbubbles");
    doc["uniq_id"] = "switch.layzspa_4w_airbubbles"+mychipid;
    doc["stat_t"] = mqttBaseTopic+F("/message");
    doc["command_topic"] = mqttBaseTopic+F("/command");
    doc["val_tpl"] = F("{{ value_json.AIR }}");
    doc["exp_aft"] = 700;
    doc["icon"] = F("mdi:chart-bubble");
    doc["avty_t"] = mqttBaseTopic+F("/Status");
    doc["pl_avail"] = F("Alive");
    doc["pl_not_avail"] = F("Dead");
    doc["payload_on"] = F("{CMD:2,VALUE:true,XTIME:0,INTERVAL:0}");
    doc["payload_off"] = F("{CMD:2,VALUE:false,XTIME:0,INTERVAL:0}");
    doc["state_on"] = 1;
    doc["state_off"] = 0;
    if (serializeJson(doc, payload) == 0)
    {
      Serial.println(F("Failed to serialize HA message!"));
      return;
    }
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
    mqttClient.loop();
    Serial.println(payload);
    doc.clear();
    doc.garbageCollect();
  }

  // spa pump switch
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/switch/layzspa_4w_pump/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa pump");
  doc["uniq_id"] = "switch.layzspa_4w_pump"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/message");
  doc["command_topic"] = mqttBaseTopic+F("/command");
  doc["val_tpl"] = F("{{ value_json.FLT }}");
  doc["exp_aft"] = 700;
  doc["icon"] = F("mdi:pump");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  doc["payload_on"] = F("{CMD:4,VALUE:true,XTIME:0,INTERVAL:0}");
  doc["payload_off"] = F("{CMD:4,VALUE:false,XTIME:0,INTERVAL:0}");
  doc["state_on"] = 1;
  doc["state_off"] = 0;
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // god mode switch
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/switch/layzspa_ctrl/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa take control");
  doc["uniq_id"] = "switch.layzspa_4w_ctrl"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/message");
  doc["command_topic"] = mqttBaseTopic+F("/command");
  doc["val_tpl"] = F("{{ value_json.GOD }}");
  doc["exp_aft"] = 700;
  doc["icon"] = F("mdi:camera-control");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  doc["payload_on"] = F("{CMD:12,VALUE:true,XTIME:0,INTERVAL:0}");
  doc["payload_off"] = F("{CMD:12,VALUE:false,XTIME:0,INTERVAL:0}");
  doc["state_on"] = true;
  doc["state_off"] = false;
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();


  // // spa temperature unit switch
  // doc["dev"] = devicedoc["dev"];
  // payload = "";
  // topic = String(HA_PREFIX) + F("/switch/layzspa_4w_temperature_unit/config");
  // Serial.println(topic);
  // doc["name"] = F("Layzspa temperature unit F-C");
  // doc["uniq_id"] = "switch.layzspa_4w_unit"+mychipid;
  // doc["stat_t"] = mqttBaseTopic+F("/message");
  // doc["command_topic"] = mqttBaseTopic+F("/command");
  // doc["val_tpl"] = F("{{ value_json.UNT }}");
  // doc["exp_aft"] = 700;
  // doc["icon"] = F("mdi:circle-outline");
  // doc["avty_t"] = mqttBaseTopic+F("/Status");
  // doc["pl_avail"] = F("Alive");
  // doc["pl_not_avail"] = F("Dead");
  // doc["payload_on"] = F("{CMD:1,VALUE:true,XTIME:0,INTERVAL:0}");
  // doc["payload_off"] = F("{CMD:1,VALUE:false,XTIME:0,INTERVAL:0}");
  // doc["state_on"] = 1;
  // doc["state_off"] = 0;
  // if (serializeJson(doc, payload) == 0)
  // {
  //   Serial.println(F("Failed to serialize HA message!"));
  //   return;
  // }
  // mqttClient.publish(topic.c_str(), payload.c_str(), true);
  // mqttClient.loop();
  // Serial.println(payload);
  // doc.clear();
  // doc.garbageCollect();

  // spa reset chlorine timer button
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/button/layzspa_4w_reset_chlorine/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa reset chlorine timer");
  doc["uniq_id"] = "button.layzspa_4w_reset_chlorine"+mychipid;
  doc["command_topic"] = mqttBaseTopic+F("/command");
  doc["payload_press"] = F("{CMD:9,VALUE:true,XTIME:0,INTERVAL:0}");
  doc["icon"] = F("mdi:restart");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa reset filter timer button
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/button/layzspa_4w_reset_filter/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa reset filter timer");
  doc["uniq_id"] = "button.layzspa_4w_reset_filter"+mychipid;
  doc["command_topic"] = mqttBaseTopic+F("/command");
  doc["payload_press"] = F("{CMD:10,VALUE:true,XTIME:0,INTERVAL:0}");
  doc["icon"] = F("mdi:restart");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // spa restart esp button
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/button/layzspa_4w_restart_esp/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa restart esp");
  doc["uniq_id"] = "button.layzspa_4w_restart_esp"+mychipid;
  doc["command_topic"] = mqttBaseTopic+F("/command");
  doc["payload_press"] = F("{CMD:6,VALUE:true,XTIME:0,INTERVAL:0}");
  doc["icon"] = F("mdi:restart");
  doc["dev_cla"] = F("restart");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  // // spa temperature sensor f
  // doc["dev"] = devicedoc["dev"];
  // payload = "";
  // topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_temperature_f/config");
  // Serial.println(topic);
  // doc["name"] = F("Layzspa temp (F)");
  // doc["uniq_id"] = "sensor.layzspa_4w_temp_f"+mychipid;
  // doc["stat_t"] = mqttBaseTopic+F("/message");
  // doc["unit_of_meas"] = "°F";
  // doc["val_tpl"] = F("{{ value_json.TMPF }}");
  // doc["exp_aft"] = 700;
  // doc["avty_t"] = mqttBaseTopic+F("/Status");
  // doc["pl_avail"] = F("Alive");
  // doc["pl_not_avail"] = F("Dead");
  // doc["dev_cla"] = F("temperature");
  // if (serializeJson(doc, payload) == 0)
  // {
  //   Serial.println(F("Failed to serialize HA message!"));
  //   return;
  // }
  // mqttClient.publish(topic.c_str(), payload.c_str(), true);
  // mqttClient.loop();
  // Serial.println(payload);
  // doc.clear();
  // doc.garbageCollect();

  // spa temperature sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_temp/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa temp");
  doc["uniq_id"] = "sensor.layzspa_4w_temp"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/message");
  doc["unit_of_meas"] = "°C";
  doc["val_tpl"] = F("{{ value_json.TMP }}");
  doc["exp_aft"] = 700;
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  doc["dev_cla"] = F("temperature");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

  //   // spa virtual temperature sensor f
  // doc["dev"] = devicedoc["dev"];
  // payload = "";
  // topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_virtualtemp_f/config");
  // Serial.println(topic);
  // doc["name"] = F("Layzspa virtual temp (F)");
  // doc["uniq_id"] = "sensor.layzspa_4w_virtual_temp_f"+mychipid;
  // doc["stat_t"] = mqttBaseTopic+F("/message");
  // doc["unit_of_meas"] = "°F";
  // doc["val_tpl"] = F("{{ value_json.VTMF | round(2) }}");
  // doc["exp_aft"] = 700;
  // doc["avty_t"] = mqttBaseTopic+F("/Status");
  // doc["pl_avail"] = F("Alive");
  // doc["pl_not_avail"] = F("Dead");
  // doc["dev_cla"] = F("temperature");
  // if (serializeJson(doc, payload) == 0)
  // {
  //   Serial.println(F("Failed to serialize HA message!"));
  //   return;
  // }
  // mqttClient.publish(topic.c_str(), payload.c_str(), true);
  // mqttClient.loop();
  // Serial.println(payload);
  // doc.clear();
  // doc.garbageCollect();

//     // spa virtual temperature sensor c
//   doc["dev"] = devicedoc["dev"];
//   payload = "";
//   topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_virtualtemp_c/config");
//   Serial.println(topic);
//   doc["name"] = F("Layzspa virtual temp (C)");
//   doc["uniq_id"] = "sensor.layzspa_4w_virtual_temp_c"+mychipid;
//   doc["stat_t"] = mqttBaseTopic+F("/message");
//   doc["unit_of_meas"] = "°C";
//   doc["val_tpl"] = F("{{ value_json.VTMC | round(2) }}");
//   doc["exp_aft"] = 700;
//   doc["avty_t"] = mqttBaseTopic+F("/Status");
//   doc["pl_avail"] = F("Alive");
//   doc["pl_not_avail"] = F("Dead");
//   doc["dev_cla"] = F("temperature");
//   if (serializeJson(doc, payload) == 0)
//   {
//     Serial.println(F("Failed to serialize HA message!"));
//     return;
//   }
//   mqttClient.publish(topic.c_str(), payload.c_str(), true);
//   mqttClient.loop();
//   Serial.println(payload);
//   doc.clear();
//   doc.garbageCollect();

// // spa target temperature sensor f
//   doc["dev"] = devicedoc["dev"];
//   payload = "";
//   topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_target_temp_f/config");
//   Serial.println(topic);
//   doc["name"] = F("Layzspa target temp (F)");
//   doc["uniq_id"] = "sensor.layzspa_4w_target_temp_f"+mychipid;
//   doc["stat_t"] = mqttBaseTopic+F("/message");
//   doc["unit_of_meas"] = "°F";
//   doc["val_tpl"] = F("{{ value_json.TGTF }}");
//   doc["exp_aft"] = 700;
//   doc["avty_t"] = mqttBaseTopic+F("/Status");
//   doc["pl_avail"] = F("Alive");
//   doc["pl_not_avail"] = F("Dead");
//   doc["dev_cla"] = F("temperature");
//   if (serializeJson(doc, payload) == 0)
//   {
//     Serial.println(F("Failed to serialize HA message!"));
//     return;
//   }
//   mqttClient.publish(topic.c_str(), payload.c_str(), true);
//   mqttClient.loop();
//   Serial.println(payload);
//   doc.clear();
//   doc.garbageCollect();

  // spa target temperature sensor
  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_target_temp/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa target temp");
  doc["uniq_id"] = "sensor.layzspa_4w_target_temp"+mychipid;
  doc["stat_t"] = mqttBaseTopic+F("/message");
  doc["unit_of_meas"] = "°C";
  doc["val_tpl"] = F("{{ value_json.TGT }}");
  doc["exp_aft"] = 700;
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  doc["dev_cla"] = F("temperature");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  mqttClient.loop();
  Serial.println(payload);
  doc.clear();
  doc.garbageCollect();

//   // spa ambient temperature sensor c
//   doc["dev"] = devicedoc["dev"];
//   payload = "";
//   topic = String(HA_PREFIX) + F("/sensor/layzspa_4w_amb_temp_c/config");
//   Serial.println(topic);
//   doc["name"] = F("Layzspa ambient temp (C)");
//   doc["uniq_id"] = "sensor.layzspa_4w_amb_temp_c"+mychipid;
//   doc["stat_t"] = mqttBaseTopic+F("/message");
//   doc["unit_of_meas"] = "°C";
//   doc["val_tpl"] = F("{{ value_json.AMBC }}");
//   doc["exp_aft"] = 700;
//   doc["avty_t"] = mqttBaseTopic+F("/Status");
//   doc["pl_avail"] = F("Alive");
//   doc["pl_not_avail"] = F("Dead");
//   doc["dev_cla"] = F("temperature");
//   if (serializeJson(doc, payload) == 0)
//   {
//     Serial.println(F("Failed to serialize HA message!"));
//     return;
//   }
//   mqttClient.publish(topic.c_str(), payload.c_str(), true);
//   mqttClient.loop();
//   Serial.println(payload);
//   doc.clear();
//   doc.garbageCollect();


  // spa climate control

  doc["dev"] = devicedoc["dev"];
  payload = "";
  topic = String(HA_PREFIX) + F("/climate/layzspa_4w_clim/config");
  Serial.println(topic);
  doc["name"] = F("Layzspa 4w temp ctrl");
  doc["uniq_id"] = "climate.layzspa4wclim"+mychipid;
  doc["max_temp"] = maxtemp;
  doc["min_temp"] = mintemp;
  doc["precision"] = 1.0;
  doc["temp_unit"] = "C";
  doc["modes"].add(serialized("\"fan_only\", \"off\", \"heat\""));
  doc["mode_cmd_t"] = mqttBaseTopic+F("/command");
  doc["mode_cmd_tpl"] = F("{CMD:3,VALUE:{%if value == \"heat\" %}1{% else %}0{% endif %},XTIME:0,INTERVAL:0}");
  doc["mode_stat_t"] = mqttBaseTopic+F("/message");
  doc["mode_stat_tpl"] = F("{% if value_json.RED == 1 %}heat{% elif value_json.GRN == 1 %}heat{% else %}off{% endif %}");
  doc["act_t"] = mqttBaseTopic+F("/message");
  doc["act_tpl"] = F("{% if value_json.RED == 1 %}heating{% elif value_json.GRN == 1 %}idle{% elif value_json.FLT == 1 %}fan{% else %}off{% endif %}");
  doc["temp_stat_t"] = mqttBaseTopic+F("/message");
  doc["temp_stat_tpl"] = F("{{ value_json.TGT }}");
  doc["curr_temp_t"] = mqttBaseTopic+F("/message");
  doc["curr_temp_tpl"] = F("{{ value_json.TMP }}");
  doc["temp_cmd_t"] = mqttBaseTopic+F("/command");
  doc["temp_cmd_tpl"] = F("{CMD:0,VALUE:{{ value|int }},XTIME:0,INTERVAL:0}");
  doc["pow_cmd_t"] = mqttBaseTopic+F("/command");
  doc["pl_on"] = F("{CMD:4,VALUE:1,XTIME:0,INTERVAL:0}");
  doc["pl_off"] = F("{CMD:4,VALUE:0,XTIME:0,INTERVAL:0}");
  doc["avty_t"] = mqttBaseTopic+F("/Status");
  doc["pl_avail"] = F("Alive");
  doc["pl_not_avail"] = F("Dead");
  if (serializeJson(doc, payload) == 0)
  {
    Serial.println(F("Failed to serialize HA message!"));
    return;
  }
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  mqttClient.loop();
  mqttClient.loop();
  doc.clear();
  doc.garbageCollect();
  Serial.println(payload);

}
