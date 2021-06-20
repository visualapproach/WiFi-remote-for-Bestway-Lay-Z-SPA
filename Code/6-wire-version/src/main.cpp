#include "main.h"

WiFiManager wm;
Ticker updateMqttTimer;
Ticker updateWSTimer;
BWC bwc;
bool sendWSFlag = false;
bool sendMQTTFlag = false;
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
<<<<<<< HEAD:Code/6-wire-version/src/main.cpp
  if(enableMQTT){
    if (!MQTTclient.loop()) MQTT_Connect();           // Do MQTT magic
  }
  
=======
  if (!MQTTclient.loop()) MQTT_Connect();           // Do MQTT magic
>>>>>>> master:Code/6-wire-version/src-for-mqtt/main.cpp
  bwc.loop();                   // Fiddle with the pump computer
  if (bwc.newData()) {
    sendMessage(1);//ws
    if(enableMQTT) sendMessage(0);//mqtt
    //bwc.saveEventlog();       //will only fill up and wear flash memory eventually
  }
  if (sendWSFlag) {
    sendWSFlag = false;
    sendMessage(1);//ws
  }
  if (sendMQTTFlag && enableMQTT) {
    sendMQTTFlag = false;
    sendMessage(0);//MQTT
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
  String jsonmsg = bwc.getJSONStates();

  //send states to web sockets
  if (msgtype == 1) {
    webSocket.broadcastTXT(jsonmsg);
  }

  //Send to MQTT - 877dev
  if (msgtype == 0) {
    if (MQTTclient.publish((String(base_mqtt_topic) + "/message").c_str(), String(jsonmsg).c_str(), true))
    {
      //Serial.println(F("MQTT published"));
    }
    else
    {
      //Serial.println(F("MQTT not published"));
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
  String mqttJSONstatus = String("{\"CONTENT\":\"OTHER\",\"MQTT\":") + String(MQTTclient.state()) + 
                          String(",\"PressedButton\":\"") + bwc.getPressedButton() +
                          String("\",\"HASJETS\":") + String(HASJETS) +
                          String("}");
  webSocket.broadcastTXT(mqttJSONstatus);
}

/*
   File handlers below. Most users can stop reading here.
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
  if (path.equalsIgnoreCase("/mqtt.txt")) return false; //don't broadcast credentials
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
	  if(upload.filename == "cmdq.txt"){
        bwc.reloadCommandQueue();
      }
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void handleFileRemove() { // delete a file from the LittleFS
  String path;
  path = server.arg("FileToRemove");
  if (!path.startsWith("/")) path = "/" + path;
  Serial.print(F("handleFileRemove Name: ")); Serial.println(path);
    if (LittleFS.exists(path) && LittleFS.remove(path)) {   // delete file if exists
      Serial.print(F("handleFileRemove success: ")); Serial.println(path);
      server.sendHeader("Location", "/success.html");       // Redirect the client to the success page
      server.send(303);
    }
    else {
      Serial.print(F("handleFileRemove error: ")); Serial.println(path);
      server.send(500, "text/plain", "500: couldn't delete file");
    }
}

/*
   Starters - bon apetit
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
  server.on(F("/getmqtt/"), handleGetMQTT);
  server.on(F("/setmqtt/"), handleSetMQTT);
  server.on(F("/resetwifi/"), handleResetWifi);
  server.on(F("/remove.html"), HTTP_POST, handleFileRemove);

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
  //wm.setConfigPortalBlocking(false);
  wm.autoConnect("AutoPortal");
  
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
   Web server functions to exchange data between server and web client
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


//response to /getmqtt/
void handleGetMQTT() { // reply with json document
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    DynamicJsonDocument doc(1024);

    // Set the values in the document
    doc["mqtt_server_ip"][0] = myMqttIP[0];
    doc["mqtt_server_ip"][1] = myMqttIP[1];
    doc["mqtt_server_ip"][2] = myMqttIP[2];
    doc["mqtt_server_ip"][3] = myMqttIP[3];
    doc["mqtt_port"] = myMqttPort;
    //doc["mqtt_username"] = myMqttUser;
    //doc["mqtt_password"] = myMqttPassword;
    doc["mqtt_username"] = "enter username";  //do not send credentials to webpage
    doc["mqtt_password"] = "enter password";
    doc["mqtt_client_id"] = mqtt_client_id;
    doc["base_mqtt_topic"] = base_mqtt_topic;
    doc["enableMQTT"] = enableMQTT;

    String jsonmsg;
    if (serializeJson(doc, jsonmsg) == 0) {
      jsonmsg = "{\"error\": \"Failed to serialize message\"}";
	  }
    server.send(200, "text/plain", jsonmsg);
  }
}

//response to /setmqtt/
void handleSetMQTT() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String message = server.arg(0);
    // Deserialize the JSON document
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
      Serial.println(F("Failed to read config file"));
      server.send(400, "plain/text", "Error deserializing message");
      return;
    }

    // Copy values from the JsonDocument to the mqtt credentials
    myMqttIP[0] = doc["mqtt_server_ip"][0];
    myMqttIP[1] = doc["mqtt_server_ip"][1];
    myMqttIP[2] = doc["mqtt_server_ip"][2];
    myMqttIP[3] = doc["mqtt_server_ip"][3];
    myMqttPort  = doc["mqtt_port"];
/*     strlcpy(const_cast<char*>(myMqttUser),                  // <- destination
            doc["mqtt_username"],           // <- source
            sizeof(myMqttUser));         // <- destination's capacity
    strlcpy(const_cast<char*>(myMqttPassword),                  // <- destination
            doc["mqtt_password"],           // <- source
            sizeof(myMqttPassword));         // <- destination's capacity
    strlcpy(const_cast<char*>(mqtt_client_id),                 // <- destination
            doc["mqtt_client_id"],          // <- source
            sizeof(mqtt_client_id));        // <- destination's capacity
    strlcpy(const_cast<char*>(base_mqtt_topic),                 // <- destination
            doc["base_mqtt_topic"],          // <- source
            sizeof(base_mqtt_topic));        // <- destination's capacity */
    myMqttUser      = doc["mqtt_username"].as<String>();
    myMqttPassword  = doc["mqtt_password"].as<String>();
    mqtt_client_id  = doc["mqtt_client_id"].as<String>();
    base_mqtt_topic = doc["base_mqtt_topic"].as<String>();
    enableMQTT      = doc["enableMQTT"];
	
    server.send(200, "plain/text", "");
    saveMQTT();
  }
}

void saveMQTT() {
  File file = LittleFS.open("mqtt.txt", "w");
  if (!file) {
    Serial.println(F("Failed to save mqtt.txt"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(1024);
  // Set the values in the document
  doc["mqtt_server_ip"][0] = myMqttIP[0];
  doc["mqtt_server_ip"][1] = myMqttIP[1];
  doc["mqtt_server_ip"][2] = myMqttIP[2];
  doc["mqtt_server_ip"][3] = myMqttIP[3];
  doc["mqtt_port"]         = myMqttPort;
  doc["mqtt_username"]     = myMqttUser;
  doc["mqtt_password"]     = myMqttPassword;
  doc["mqtt_client_id"]    = mqtt_client_id;
  doc["base_mqtt_topic"]   = base_mqtt_topic;
  doc["enableMQTT"]        = enableMQTT;

  if (serializeJson(doc, file) == 0) {
    Serial.println("{\"error\": \"Failed to serialize mqtt file\"}");
  }
  file.close();

}

void loadMQTT() {
  File file = LittleFS.open("mqtt.txt", "r");
  if (!file) {
    Serial.println(F("Failed to read mqtt.txt. Using default."));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(1024);  
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to deserialize mqtt.txt"));
    file.close();
    return;
  }

  // Copy values from the JsonDocument to the mqtt credentials
  myMqttIP[0] = doc["mqtt_server_ip"][0];
  myMqttIP[1] = doc["mqtt_server_ip"][1];
  myMqttIP[2] = doc["mqtt_server_ip"][2];
  myMqttIP[3] = doc["mqtt_server_ip"][3];
  myMqttPort  = doc["mqtt_port"];
/*   strlcpy(const_cast<char*>(myMqttUser),                  // <- destination
          doc["mqtt_username"],           // <- source
          sizeof(myMqttUser));         // <- destination's capacity
  strlcpy(const_cast<char*>(myMqttPassword),                  // <- destination
          doc["mqtt_password"],           // <- source
          sizeof(myMqttPassword));         // <- destination's capacity
  strlcpy(const_cast<char*>(mqtt_client_id.),                 // <- destination
          doc["mqtt_client_id"],          // <- source
          sizeof(mqtt_client_id));        // <- destination's capacity
  strlcpy(const_cast<char*>(base_mqtt_topic),                 // <- destination
          doc["base_mqtt_topic"],          // <- source
          sizeof(base_mqtt_topic));        // <- destination's capacity */
	myMqttUser      = doc["mqtt_username"].as<String>();
	myMqttPassword  = doc["mqtt_password"].as<String>();
	mqtt_client_id  = doc["mqtt_client_id"].as<String>();
	base_mqtt_topic = doc["base_mqtt_topic"].as<String>();
  enableMQTT      = doc["enableMQTT"];
}

void handleResetWifi(){
  WiFi.disconnect();
  delay(3000);
  Serial.println("resetting");
  ESP.reset();
  //Do this before giving away the device
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
      {
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
      }
      break;
      
    default:
      break;
  }
}

/*
   MQTT functions
*/

void startMQTT() { //MQTT setup and connect - 877dev
  //load mqtt credential file if it exists, and update default strings  ********************
  loadMQTT();

  //MQTTclient.setServer(mqtt_server_name, mqtt_port); //setup MQTT broker information as defined earlier
  MQTTclient.setServer(myMqttIP, myMqttPort); //setup MQTT broker information as defined earlier
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
  if (MQTTclient.connect(mqtt_client_id.c_str(), myMqttUser.c_str(), myMqttPassword.c_str(), (String(base_mqtt_topic) + "/Status").c_str(), 0, 1, "Dead"))
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