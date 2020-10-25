/*

  Bestway Spa Controller Library by Thomas Landahl (Visualapproach)

  https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
   
  Installation and first run:
  
  Copy files to documents/Arduino/Libraries/
  Start Arduino IDE and open File/examples/BWC_v2.0.0/[choose one]
  Select the right board, and set "FS 2MB/OTA 1MB", speed 80 MHz.

  Compile and upload sketch. 
  **Upload LittleFS files.** 
    
  An Access Point is created called "Auto portal". Log in and enter wifi credentials.

  Visit IP/ and click "Go to config page"
  Enter your settings, click SAVE.
   
  This version is using a command queue with repeatable commands. 
  11 commands can be queued, of which 10 should be used by the user. 
  Please leave 1 for internal use.
  The library will unlock the device automatically if necessary.
  Just send what you want to happen when, and the library will take care of it.
  You can even directly set desired target temperature!!
  
 LittleFS files saved from this program:
 
 settings.txt   - stores JSON formatted running times and configurations. Updates every 2 hrs.
 cmdq.txt       - stores JSON formatted command queue.
 eventlog.txt   - stores JSON formatted states and a timestamp (Mostly for debugging. Not human friendly as is)
 bootlog.txt    - stores JSON formatted boot times and reasons.

 These files can be read by visiting the address "IP/filename.txt"
 You can if you want edit the file and upload it by visiting "IP/upload.html"
 
 To remove the files you need to re-upload LittleFS from Arduino IDE. Or write a function to do so :-)

  
-------------------Info to coders below--------------------  

  Public functions in BWC class:

  void begin(void);       //Must be run once in setup.
  void loop(void);        //As always, don't use blocking code. This needs to be run fairly frequently.
  void print(String txt); //Send text to display. Not all characters can be shown on a 7 segment display and will be replaced by spaces.
  bool qCommand(uint32_t cmd, uint32_t val, uint32_t xtime, uint32_t interval);  


  bool newData();                         //true when a state has changed
  void saveEventlog();                    //save states to LittleFS (flash) (I don't recommend using this unless you are really a log freak)
  String getJSONStates();                 //get a JSON formatted string to send to clients
  String getJSONTimes();                  //get a JSON formattes string to send to clients
  String getJSONSettings();               //get a JSON formattes string to send to clients (internal use)
  void setJSONSettings(String message);   //save settings to flash. Must be formatted correctly. (internal use)
  String getJSONCommandQueue();           //get a JSON formatted string to send to clients

States index constants, for use in bwc.getState(int state):
  0  LOCKEDSTATE
  1  POWERSTATE
  2  UNITSTATE
  3  BUBBLESSTATE
  4  HEATGRNSTATE
  5  HEATREDSTATE
  6  HEATSTATE
  7  PUMPSTATE
  8  TARGET
  9  TEMPERATURE
  10 CHAR1
  11 CHAR2
  12 CHAR3

  These are defined in the library so you can call bwc.getState(TEMPERATURE) for instance.
 
  Commands index constants, for use in bwc.qCommand(uint32_t cmd, uint32_t val, uint32_t xtime, uint32_t interval)
  (Sending commands from web pages or MQTT)
  cmd:
  0 SETTARGET
  1 SETUNIT
  2 SETBUBBLES
  3 SETHEATER
  4 SETPUMP
  5 RESETQ
  6 REBOOTESP
  7 GETTARGET
  8 RESETTIMES
  9 RESETCLTIMER
    ...may be extended
	
  val:
    1 = ON/Fahrenheit, 0 = OFF/Celsius, or any value for SETTARGET temperature
  xtime: eXecute at timestamp (unix timestamp in seconds)
  interval: repeat every INTERVAL seconds. 0 = no repeat.

 These are defined in the library so you can call bwc.qCommand(SETPUMP, 1, 1603321200, 3600) to
 turn on filter pump at 10/21/2020 @ 11:00pm, repeating every hour (3600s)
    
 The library is running an NTP and LittleFS so no need to include those here

*/


#include "BWC.h"
#include "globals.h"

Ticker updateMqttTimer;
Ticker updateWSTimer;
BWC bwc;

const int solarpin = D0;    //no interrupt or PWM
const int myoutputpin = D8; //pulled to GND. Boot fails if pulled HIGH.
bool runonce = true;

void setup() {
  // put your setup code here, to run once:
  pinMode(solarpin, INPUT_PULLUP);
  pinMode(myoutputpin, OUTPUT);
  digitalWrite(myoutputpin, LOW);
  Serial.begin(115200);		//As if you connected serial to your pump...
  startWiFi();
  startOTA();
  startServer();
  startWebSocket();
  bwc.begin();
  startMQTT();
  updateMqttTimer.attach(600, sendmqtt); //update mqtt every 10 minutes. Mqtt will also be updated on every state change
  updateWSTimer.attach(2.0, sendws);     //update webpage every 2 secs plus state changes
}

void loop() {
  webSocket.loop();             // constantly check for websocket events
  server.handleClient();        // run the server
  ArduinoOTA.handle();          // listen for OTA events
  MQTTclient.loop();            // Do MQTT magic
  bwc.loop();                   // Fiddle with the pump computer
  if (bwc.newData()) {
    sendMessage(1);//ws
    sendMessage(0);//mqtt
    //bwc.saveEventlog();       //will only fill up and wear flash memory eventually
  }

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
  
  //You can add own code here, but don't stall! If CPU is choking you can try to run @ 160 MHz, but that's cheating!
}

// If failing to connect at all, this will stall the loop = Erratic behavior may occur if called to often.
// Default frequency of once every 10 min shouldn't be a problem though.
// This function is called by the mqtt timer
void sendmqtt() {
  if (!MQTTclient.connected())
  {
    Serial.println("reconnecting");
    MQTT_Connect();
  }
  sendMessage(0);//0 = mqtt
}

// This function is called by the websockets timer.
void sendws(){
  sendMessage(1); //1 = ws
  String mqttJSONstatus = String("{\"CONTENT\":\"OTHER\",\"MQTT\":") + String(MQTTclient.connected()) + String("}");
  webSocket.broadcastTXT(mqttJSONstatus);
}

// Send status data to web client in JSON format (because it is easy to decode on the other side)
void sendMessage(int msgtype) {
  String jsonmsg = bwc.getJSONStates();
  
  //send states to web sockets
  if (msgtype == 1) {
    webSocket.broadcastTXT(jsonmsg);
  }
  
  //Send to MQTT - 877dev
  if (msgtype == 0) {
    if (MQTTclient.publish((String(base_mqtt_topic) + "/message").c_str(), String(jsonmsg).c_str(), true))
    {
      Serial.println(F("MQTT published"));
    }
    else
    {
      Serial.println(F("MQTT not published"));
    }
  }

  jsonmsg = bwc.getJSONTimes();
  
  if (msgtype == 1) {
    webSocket.broadcastTXT(jsonmsg);
  }
//  if you want up-times etc sent to mqtt:
//  if (msgtype == 0) {
//    if (MQTTclient.publish((String(base_mqtt_topic) + "/message").c_str(), String(jsonmsg).c_str(), true))
//    {
//      Serial.println(F("MQTT published"));
//    }
//    else
//    {
//      Serial.println(F("MQTT not published"));
//    }
  
}

/*
 * File handlers below. Most users can stop reading here.
 */

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += F("index.html");          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (LittleFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed version
    File file = LittleFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (LittleFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

void handleFileUpload() { // upload a new file to the LittleFS
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/" + path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
      if (LittleFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
        LittleFS.remove(pathWithGz);
    }
    Serial.print(F("handleFileUpload Name: ")); Serial.println(path);
    fsUploadFile = LittleFS.open(path, "w");            // Open the file for writing in LittleFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print(F("handleFileUpload Size: ")); Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

/*
 * Starters - bon apetit
 */

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println(F("WebSocket server started."));
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on(F("/upload.html"),  HTTP_POST, []() {  // If a POST request is sent to the /upload.html address,
    server.send(200, "text/plain", "");
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.on(F("/getconfig/"), handleGetConfig);
  server.on(F("/setconfig/"), handleSetConfig);
  server.on(F("/getcommands/"), handleGetCommandQueue);
  server.on(F("/addcommand/"), handleAddCommand);
  //  server.on(F("/remove.html"), handleLogRemove);  //not implemented

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println(F("HTTP server started."));
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println(F("OTA Start"));
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

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.autoConnect("AutoPortal");
  //WiFi.begin();
  Serial.println(F("Connecting"));
  while (WiFi.status() != WL_CONNECTED) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
    //checkAP_request();
  }
  Serial.println("\r\n");
  Serial.print(F("Connected to "));
  Serial.println(WiFi.SSID());             // Tell us what network we're connected to
  Serial.print(F("IP address:\t"));
  Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  Serial.println("\r\n");
}

/*
 * Web server functions to exchange data between server and web client
 */

//response to /getconfig/
void handleGetConfig() { // reply with json document
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String jsonmsg = bwc.getJSONSettings();
    server.send(200, "text/plain", jsonmsg);
  }
}

//response to /setconfig/
void handleSetConfig() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String message = server.arg(0);
    bwc.setJSONSettings(message);
    server.send(200, "plain/text", "");
  }
}


//response to /getcommands/
void handleGetCommandQueue() { // reply with json document
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String jsonmsg = bwc.getJSONCommandQueue();
    server.send(200, "text/plain", jsonmsg);
  }
}

//respone to /addcommand/
void handleAddCommand() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    DynamicJsonDocument doc(256);
    String message = server.arg(0);
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
      server.send(500, "text/plain", "");
      return;
    }
    uint32_t command = doc["CMD"];
    uint32_t value = doc["VALUE"];
    uint32_t xtime = doc["XTIME"];
    uint32_t interval = doc["INTERVAL"];
    bwc.qCommand(command, value, xtime, interval);
    server.send(200, "text/plain", "");
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) {
  // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        sendMessage(1);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
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
      break;
  }
}

/*
 * MQTT functions
 */

void startMQTT() { //MQTT setup and connect - 877dev
  //MQTTclient.setServer(mqtt_server_name, mqtt_port); //setup MQTT broker information as defined earlier
  MQTTclient.setServer(myMqttIP, myMqttPort); //setup MQTT broker information as defined earlier
  if (MQTTclient.setBufferSize (2048))      //set buffer for larger messages, new to library 2.8.0
  {
    Serial.println(F("MQTT buffer size successfully increased"));
  }
  MQTTclient.setCallback(MQTTcallback);          // set callback details - this function is called automatically whenever a message arrives on a subscribed topic.
  MQTT_Connect();                                //Connect to MQTT broker, publish Status/MAC/count, and subscribe to keypad topic.
}


void MQTTcallback(char* topic, byte* payload, unsigned int length) {  //877dev

  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (String(topic).equals(String(base_mqtt_topic) + "/command")) {
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
  if (MQTTclient.connect(mqtt_client_id, myMqttUser, myMqttPassword, (String(base_mqtt_topic) + "/Status").c_str(), 0, 1, "Dead"))
  {
    // We get here if the connection was successful...
    mqtt_connect_count++;
    Serial.println(F("CONNECTED!"));
    // Once connected, publish some announcements...
    // These all have the Retained flag set to true, so that the value is stored on the server and can be retrieved at any point
    // Check the .../Status topic to see that the device is still online before relying on the data from these retained topics
    MQTTclient.publish((String(base_mqtt_topic) + "/Status").c_str(), "Alive", true);
    MQTTclient.publish((String(base_mqtt_topic) + "/MAC_Address").c_str(), WiFi.macAddress().c_str(), true);                 // Device MAC Address
    MQTTclient.publish((String(base_mqtt_topic) + "/MQTT_Connect_Count").c_str(), String(mqtt_connect_count).c_str(), true); // MQTT Connect Count
    MQTTclient.loop();


    // ... and then re/subscribe to the watched topics
    MQTTclient.subscribe((String(base_mqtt_topic) + "/command").c_str());   // Watch the .../command topic for incoming MQTT messages
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
