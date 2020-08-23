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
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed version
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}
void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}
void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/" + path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
      if (SPIFFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
        SPIFFS.remove(pathWithGz);
    }
    Serial.print(F("handleFileUpload Name: ")); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
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
  if (SPIFFS.exists("/eventlog.csv")) SPIFFS.remove("/eventlog.csv");
  server.send(200,"text/plain", "OK");

  Serial.println("removed");
}

void enterAPmode() {
  WiFiManager wm;

  //reset settings - for testing
  //wifiManager.resetSettings();

  // set configportal timeout
  wm.setConfigPortalTimeout(180);

  if (!wm.startConfigPortal("ManualPortal")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
}

//when web client is sending commands we simulate corresponding key press
//and toggle the wanted state for that function.
//When the wanted state equals the actual state, the simulator can stop mimic a key press.
//Handled in releaseVirtualButtons()
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
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
            heater_cmd = !(heater_red_sts|heater_green_sts);
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
            setClTimer();
            break;
          default:
            break;
        }
      }
      sendWSmessage();
      break;
  }
}

//send status data to web client in JSON format (because it is easy to decode on the other side)
void sendWSmessage() {
  //calculate days since clTimestamp
  uint32_t daysSinceClReset = 0;
  daysSinceClReset = DateTime.now() - clTime;
  daysSinceClReset /= (3600 * 24);

  float heatingRatio = (float)getHeatingTime() / (DateTime.now() - DateTime.getBootTime());

  String sendjson = "{";
  sendjson += "\"temp\":\"" + String(cur_tmp_val) + "\", ";
  sendjson += "\"target\":\"" + String(set_tmp_val) + "\", ";
  sendjson += "\"locked\":\"" + String(locked_sts) + "\", ";
  sendjson += "\"air\":\"" + String(air_sts) + "\", ";
  sendjson += "\"celsius\":\"" + String(celsius_sts) + "\", ";
  sendjson += "\"heating\":\"" + String(heater_red_sts) + "\", ";
  sendjson += "\"heater\":\"" + String(heater_green_sts) + "\", ";
  sendjson += "\"filter\":\"" + String(filter_sts) + "\", ";
  sendjson += "\"power\":\"" + String(power_sts) + "\", ";
  sendjson += "\"time\":\"" + String(DateTime.format(DateFormatter::SIMPLE) + "\", ");
  sendjson += "\"ratio\":\"" + String(heatingRatio) + "\", ";
  sendjson += "\"timer\":\"" + String(daysSinceClReset) + "\"";
  sendjson += "}";
  webSocket.broadcastTXT(sendjson);
  //Serial.println("sent message: " + sendjson);
}
