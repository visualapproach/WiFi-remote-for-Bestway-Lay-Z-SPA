
void startup() {
  setpins();
  Serial.begin(115200);
  Serial.println(F("Starting Bestway spa controller."));
  Serial.println(F("Playing intro."));
  playIntro();
  Serial.println(F("Writing message on display"));
  textOut("  have a nice spa");
  delay(100);

  Serial.print("timezone:");
  Serial.println(myConfig.timezone);

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  startOTA();                  // Start the OTA service
  startLittleFS();               // Start the LittleFS and list all contents
  loadConfiguration();
  loadappdata();
  setupDateTime();             //NTP
  Serial.println(DateTime.format(DateFormatter::SIMPLE));

  DateTimeParts p = DateTime.getParts();
  prevHour = p.getHours();
  uptimestamp = DateTime.now();
  //DEBUG***********************VVV
  File file = LittleFS.open("tmp.txt", "a");
  file.print("\nREBOOT ");
  file.println(DateTime.format(DateFormatter::SIMPLE));
  file.print("Reason: ");
  file.println(ESP.getResetReason());
  file.close();
  filterStart = DateTime.now();
  airStart = filterStart;
  heaterStart = filterStart;

  startWebSocket();            // Start a WebSocket server
  startServer();               // Start a HTTP server with a file read handler and an upload handler
  startTimers();
  startMQTT();                // Start the MQTT server - 877dev
  attachInterrupt(digitalPinToInterrupt(CS_CIO_PIN), ISR_slaveSelect, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLK_CIO_PIN), ISR_CLK_CIO_PIN_byte, CHANGE); //Write on falling edge and read on rising edge
  //unfortunately, the CIO is inconsistent and switches state on data line both during HIGH and LOW clock, so occasionally there will be misreadings.
  //It might show up in the log file and the speaker may beep once in a while. Hopefully fixed now, by disregarding a single transmission if it differs from previous.
}

void setpins() {
  //setup CIO bit bang pins
  pinMode(CS_CIO_PIN, INPUT);
  pinMode(CLK_CIO_PIN, INPUT);
  pinMode(DATA_CIO_PIN, INPUT);//high impedance until output needed

  //setup DSP bit bang pins
  pinMode(CS_DSP_PIN, OUTPUT);
  digitalWrite(CS_DSP_PIN, HIGH); //Active LOW
  pinMode(CLK_DSP_PIN, OUTPUT);
  digitalWrite(CLK_DSP_PIN, HIGH);//shift on falling, latch on rising
  pinMode(DATA_DSP_PIN, INPUT);//high impedance until output needed
  pinMode(AUDIO_OUT_PIN, OUTPUT);
  digitalWrite(AUDIO_OUT_PIN, LOW);
}

/*

   SETUP FUNCTIONS

*/
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
  textOut(WiFi.localIP().toString());
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

void startLittleFS() { // Start the LittleFS and list all contents
  LittleFS.begin();                             // Start the SPI Flash File System (LittleFS)
  Serial.println(F("LittleFS started. Contents:"));
  {
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

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
  server.on(F("/getmqtt/"), handleGetMqtt);
  server.on(F("/setmqtt/"), handleSetMqtt);

  server.on(F("/remove.html"), handleLogRemove);                      // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println(F("HTTP server started."));
}

void setupDateTime() {
  // setup this after wifi connected
  // you can use custom timezone,server and timeout
  // DateTime.setTimeZone(-4);
  //   DateTime.setServer("asia.pool.ntp.org");
  //   DateTime.begin(15 * 1000);
  DateTime.setTimeZone(myConfig.timezone);
  DateTime.begin();
  delay(1000);
  int c = 0;
  while (!DateTime.isTimeValid()) {
    Serial.println(F("Failed to get time from server. Trying again."));
    delay(1000);
    DateTime.setServer("time.cloudflare.com");
    DateTime.setTimeZone(myConfig.timezone);
    DateTime.begin();
    if (c++ > 5) break;
  }
}

void startTimers() {
  tickerSecond.attach(1.4, secondTimer);
  tickerMinute.attach(60, minuteTimer);
  tickerDay.attach(24 * 3600, dayTimer); //save data every day
}

void startMQTT() { //MQTT setup and connect - 877dev
  if (loadmqtt()) {
    MQTTclient.setServer(mqtt_server_ip, mqtt_port); //setup MQTT broker information as defined earlier
    if (MQTTclient.setBufferSize (1024))      //set buffer for larger messages, new to library 2.8.0
    {
      Serial.println(F("MQTT buffer size successfully increased"));
    }
    MQTTclient.setCallback(MQTTcallback);          // set callback details - this function is called automatically whenever a message arrives on a subscribed topic.
    MQTT_Connect();                                //Connect to MQTT broker, publish Status/MAC/count, and subscribe to keypad topic.
  }
}
