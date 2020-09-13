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
  myConfig.automode = doc["automode"];
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
  doc["automode"] = myConfig.automode;

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
