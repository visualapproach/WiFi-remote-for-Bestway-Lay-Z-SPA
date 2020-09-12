String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

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

void handleLogRemove() {
  if (LittleFS.exists("/eventlog.csv")) LittleFS.remove("/eventlog.csv");
  server.send(200, "text/plain", "OK");

  Serial.println(F("removed"));
}

void enterAPmode() {
  WiFiManager wm;

  //reset settings - for testing
  //wifiManager.resetSettings();

  // set configportal timeout
  wm.setConfigPortalTimeout(180);

  if (!wm.startConfigPortal("ManualPortal")) {
    Serial.println(F("failed to connect and hit timeout"));
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println(F("connected...yeey :)"));
}

//when web client is sending commands we simulate corresponding key press
//and toggle the wanted state for that function.
//When the wanted state equals the actual state, the simulator can stop mimic a key press.
//Handled in releaseVirtualButtons()
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) {
  // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        sendWSmessage();
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
      uint32_t cmd = strtol((const char *) &payload[0], NULL, 16);
      if (cmd > 0x13) {
        //target temperature received
        //Not implemented.
      } else {
        //a button is received
        switch (cmd) {
          case 0x0:
            //reboot
            saveappdata();
            ESP.restart();
            break;
          case 0x1:
            myConfig.automode = true;
            saveappdata();
            break;
          case 0x2:
            myConfig.automode = false;
            saveappdata();
            break;
          case 0x3:
            heaterStart = DateTime.now();
            filterStart = DateTime.now();
            airStart = DateTime.now();
            appdata.heattime = 0;
            appdata.uptime = 0;
            appdata.airtime = 0;
            appdata.filtertime = 0;
            appdata.cost = 0;
            saveappdata();
            break;
          case 0x5:
            //power
            virtualBTN = PWR;
            power_cmd = !power_sts;
            break;
          case 0x6:
            //up
            virtualBTN = UP;
            BTN_timeout = millis() + 500;
            break;
          case 0x7:
            //down
            virtualBTN = DWN;
            BTN_timeout = millis() + 500;
            break;
          case 0x8:
            //filter
            virtualBTN = FLT;
            filter_cmd = !filter_sts;
            break;
          case 0xA:
            //heater
            virtualBTN = HTR;
            heater_cmd = !(heater_red_sts | heater_green_sts);
            break;
          case 0xB:
            //unit
            virtualBTN = UNT;
            celsius_cmd = !celsius_sts;
            break;
          case 0xC:
            //air
            virtualBTN = AIR;
            air_cmd = !air_sts;
            break;
          case 0xD:
            //timer
            virtualBTN = TMR;
            break;
          case 0xE:
            //lock
            virtualBTN = LCK;
            locked_cmd = !locked_sts;
            break;
          case 0xF:
            //no button
            virtualBTN = NOBTN;
            break;
          case 0x10:
            //reset chlorine timer
            {
              appdata.clts = DateTime.now();
              saveappdata();
              break;
            }
          default:
            break;
        }
      }
      //sendWSmessage();
      break;
  }

}

//send status data to web client in JSON format (because it is easy to decode on the other side)
void sendWSmessage() {
  StaticJsonDocument<1200> doc; //I hope this is enough
  doc["temp"] = (cur_tmp_val);
  doc["target"] = (set_tmp_val);
  doc["locked"] = (locked_sts);
  doc["air"] = (air_sts);
  doc["celsius"] = (celsius_sts);
  doc["heating"] = (heater_red_sts);
  doc["heater"] = (heater_green_sts);
  doc["filter"] = (filter_sts);
  doc["power"] = (power_sts);
  doc["time"] = (DateTime.format(DateFormatter::SIMPLE));
  doc["clts"] = (DateTime.now() - appdata.clts);
  doc["heattime"] = getHeatingTime();
  doc["uptime"] = appdata.uptime+DateTime.now()-uptimestamp;
  doc["airtime"] = getAirTime();
  doc["filtertime"] = getFilterTime();
  doc["cost"] = appdata.cost; //updates every second -ish
  doc["auto"] = myConfig.automode; //updates every second -ish
  String jsonmsg;
  if (serializeJson(doc, jsonmsg) == 0) {
    Serial.println(F("Failed to serialize ws message"));
  } else {
    webSocket.broadcastTXT(jsonmsg);
  }

}

void handleGetConfig() { // reply with json document
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<1024> doc;

    // Set the values in the document
    doc["filteroffhour"] = myConfig.filteroffhour;
    doc["filteronhour"] = myConfig.filteronhour;
    doc["timezone"] = myConfig.timezone;
    for (int i = 0; i < 24; i++) {
      doc["heaterhours"][i] = myConfig.heaterhours[i];
    }
    doc["audio"] = myConfig.audio;
    doc["price"] = myConfig.price;
    doc["rb"] = DateFormatter::format(DateFormatter::SIMPLE, (time_t) DateTime.getBootTime(), myConfig.timezone);

    // Serialize JSON to string
    String jsonmsg;
    if (serializeJson(doc, jsonmsg) == 0) {
      Serial.println(F("Failed to serialize message"));
      server.send(500, "text/plain", "error serializing object");
    } else {
      server.send(200, "text/plain", jsonmsg);
    }
  }
}

void handleSetConfig() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String message;
    message = server.arg(0);
    //Serial.println(message);
    StaticJsonDocument<1024> doc;
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, message);
    if (error){
      Serial.println(F("Failed to set config"));
      server.send(500, "plain/text", "Failed to set config");
      return;
    }

    // Copy values from the JsonDocument to the Config
    myConfig.filteroffhour = doc["filteroffhour"];
    myConfig.filteronhour = doc["filteronhour"];
    myConfig.timezone = doc["timezone"];
    for (int i = 0; i < 24; i++) {
      myConfig.heaterhours[i] = doc["heaterhours"][i];
    }
    myConfig.audio = doc["audio"];
    myConfig.price = doc["price"];

    server.send(200, "plain/text", "");
    saveCfgFlag = true;
  }
}
