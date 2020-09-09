//this function is executed every loop
//There is likely redundant flags but at least it works now.
void schedule() {
  static bool heaterOffFlag = false;

  if (myConfig.automode) {
    //timers are setting flags to request filter on off, or heater on off
    if (filterOnFlag) {
      turnOnFilter();//Flag will be reset when mission accomplished in this function
    }

    if (filterOffFlag) {
      turnOffFilter();//Flag will be reset when mission accomplished in this function
    }

    if (heaterDisableFlag) {
      savedHeaterState = heater_red_sts || heater_green_sts;
      heaterOffFlag = true;
      heaterDisableFlag = false;
      heaterEnabled = false;
    }
    if (heaterOffFlag) {
      if (heater_red_sts || heater_green_sts) {
        setHeater(0);
      } else {
        heaterOffFlag = false;
      }
    }

    if (heaterEnableFlag) {
      if (savedHeaterState != (heater_red_sts || heater_green_sts)) {
        heaterEnabled = true;
        setHeater(savedHeaterState);
      } else {
        heaterEnableFlag = false;
      }
    }

  } //end if automode

  if (saveCfgFlag) {
    saveConfiguration();
    Serial.println(F("Config saved"));
    saveCfgFlag = false;
  }
  if (saveAppdataFlag) {
    saveAppdataFlag = false;
    saveappdata();
  }

}


//executed daily
void dayTimer() {
  saveAppdataFlag = true; //Flash memory only lasts for so many writes (~100 000?)
  //event and appdata is saved every time something changed (check handleData())
}


//executed every minute
void minuteTimer() {
  if (!DateTime.isTimeValid()) {
    Serial.println(F("Failed to get time from server, retry."));
    DateTime.begin();
  }
  uint32_t t = DateTime.now();
  appdata.uptime += t-uptimestamp;
  uptimestamp = t;
  
  DateTimeParts p = DateTime.getParts();
  int h = p.getHours();
  //execute ONCE every new hour. This method avoids drifting problems over long time, compared to an hourTimer
  if (h != prevHour) {
    prevHour = h;
    if (isheaterhours()) {
      heaterEnableFlag = true;
    } 
    if (h == myConfig.filteronhour) {
      filterOnFlag = true;
    }
    if (h == myConfig.filteroffhour) {
      filterOffFlag = true;
    }

  }

  //check every minute. Do whatever it takes to shut down the heater ;-)
  if(!isheaterhours() && (heater_red_sts || heater_green_sts)) {
      heaterDisableFlag = true;
    }
}

//you get it by now. Except this is executed every 1.4 secs...
void secondTimer() {
  appdata.cost = getHeatingTime() * 1900 + getFilterTime() * 40 + getAirTime() * 800 + appdata.uptime * 2; //wattSeconds
  appdata.cost /= 3600000.0; //kWh
  appdata.cost *= myConfig.price; //money
  sendWSmessage();
}
