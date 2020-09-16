void loadConfiguration() {
  File file = LittleFS.open(cfgFileName, "r");
  if (!file) {
    Serial.println(F("Failed to open config file (load)"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<1024> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to read config file, using default configuration"));
    file.close();
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
  //  strlcpy(myConfig.hostname,                  // <- destination
  //          doc["hostname"] | "example.com",  // <- source
  //          sizeof(myConfig.hostname));         // <- destination's capacity

  file.close();

  Serial.println(F("loaded config"));
}


void saveConfiguration() {
  File file = LittleFS.open(cfgFileName, "w");
  if (!file) {
    Serial.println(F("Failed to create config file"));
    return;
  }

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

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write json to config file"));
  }
  file.close();


  DateTime.setTimeZone(myConfig.timezone);
  DateTime.begin();
}

void saveappdata() {
  //store data struct to flash (LittleFS)
  appdata.heattime = getHeatingTime();
  heaterStart = DateTime.now();
  appdata.uptime = getUpTime();
  uptimestamp = DateTime.now();
  appdata.airtime = getAirTime();
  airStart = DateTime.now();
  appdata.filtertime = getFilterTime();
  filterStart = DateTime.now();

  File file = LittleFS.open(appdataFileName, "w");
  if (!file) {
    Serial.println(F("Failed to write appdata file"));
    return;
  }
  file.write((uint8_t *)&appdata, sizeof(appdata));
  file.close();


}

void loadappdata() {
  //store data struct to flash (LittleFS)
  File file = LittleFS.open(appdataFileName, "r");
  if (!file) {
    Serial.println(F("Failed to open appdata file"));
    return;
  }
  //while (file.available()) {
  file.read((uint8_t *)&appdata, sizeof(appdata));
  //}
  file.close();

}

void savelog() {
  //Don't start logging until time is set
  if (!DateTime.isTimeValid()) return;
  File logfile;
  String fname = "/eventlog.csv";

  if (!LittleFS.exists(fname)) {
    logfile = LittleFS.open(fname, "a");
    logfile.println(F("Now(), simple datetime, temp, target temp, locked, power, air, heater, heating, filter, celsius, heating hours (acc), heating ratio"));
  } else {
    logfile = LittleFS.open(fname, "a");
  }
  logfile.print(DateTime.now());
  logfile.print(",");
  logfile.print(DateTime.format(DateFormatter::SIMPLE));
  logfile.print(",");
  logfile.print(cur_tmp_str);
  logfile.print(",");
  logfile.print(String(set_tmp_val));
  logfile.print(",");

  logfile.print(locked_sts);
  logfile.print(",");
  logfile.print(power_sts);
  logfile.print(",");
  logfile.print(air_sts);
  logfile.print(",");
  logfile.print(heater_green_sts);
  logfile.print(",");
  logfile.print(heater_red_sts);
  logfile.print(",");
  logfile.print(filter_sts);
  logfile.print(",");
  logfile.print(celsius_sts);
  logfile.print(",");
  float heatingRatio = (float) getHeatingTime() / (DateTime.now() - DateTime.getBootTime());
  logfile.print(getHeatingTime() / 3600.0);
  logfile.print(",");
  logfile.println(heatingRatio);

  logfile.close();


  //Serial.println(F("saved log")); //debug*************
}

bool loadmqtt() {
  File file = LittleFS.open(mqttcredentialsfilename, "r");
  if (!file) {
    Serial.println(F("Failed to open mqtt file (load)"));
    appdata.usemqtt = false;
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<1024> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to deserialize mqtt file"));
    file.close();
    return false;
  }

  // Copy values from the JsonDocument to the mqtt credentials
  mqtt_server_ip[0] = doc["mqtt_server_ip"][0];
  mqtt_server_ip[1] = doc["mqtt_server_ip"][1];
  mqtt_server_ip[2] = doc["mqtt_server_ip"][2];
  mqtt_server_ip[3] = doc["mqtt_server_ip"][3];
  mqtt_port = doc["mqtt_port"];
  strlcpy(mqtt_username,                  // <- destination
          doc["mqtt_username"],           // <- source
          sizeof(mqtt_username));         // <- destination's capacity
  strlcpy(mqtt_password,                  // <- destination
          doc["mqtt_password"],           // <- source
          sizeof(mqtt_password));         // <- destination's capacity
  strlcpy(mqtt_client_id,                 // <- destination
          doc["mqtt_client_id"],          // <- source
          sizeof(mqtt_client_id));        // <- destination's capacity
  strlcpy(base_mqtt_topic,                 // <- destination
          doc["base_mqtt_topic"],          // <- source
          sizeof(base_mqtt_topic));        // <- destination's capacity

  file.close();
  appdata.usemqtt = true;
  return true;
}

void savemqtt() {
  appdata.usemqtt = false;
  File file = LittleFS.open(mqttcredentialsfilename, "w");
  if (!file) {
    Serial.println(F("Failed to create mqtt file"));

    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<1024> doc;

  // Set the values in the document
  doc["mqtt_server_ip"][0] = mqtt_server_ip[0];
  doc["mqtt_server_ip"][1] = mqtt_server_ip[1];
  doc["mqtt_server_ip"][2] = mqtt_server_ip[2];
  doc["mqtt_server_ip"][3] = mqtt_server_ip[3];
  doc["mqtt_port"] = mqtt_port;
  doc["mqtt_username"] = mqtt_username;
  doc["mqtt_password"] = mqtt_password;
  doc["mqtt_client_id"] = mqtt_client_id;
  doc["base_mqtt_topic"] = base_mqtt_topic;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write json to mqtt file"));
  } else {
    appdata.usemqtt = true;
  }
  file.close();
}
