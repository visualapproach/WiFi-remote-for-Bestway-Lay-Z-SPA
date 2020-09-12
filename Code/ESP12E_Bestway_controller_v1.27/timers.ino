//this function is executed every loop
//allowedHeaterHours should work like this:
//Turn off heater (once) when entering a forbidden hour. Then discard any HTR ON button during that hour.
//Restore heater (once) when entering an allowed hour.
void schedule() {

  if (myConfig.automode) {
    //timers are setting flags to request filter on off, or heater on off
    if (filterOnFlag) {
      turnOnFilter();//Flag will be reset when mission accomplished in this function
    }

    if (filterOffFlag) {
      turnOffFilter();//Flag will be reset when mission accomplished in this function
    }

    //flags are reset in releaseButtons after misson accomplished
    if (heaterDisableFlag) {
      setHeater(0);
    }

    if (heaterEnableFlag && filter_sts) {
      setHeater(savedHeaterState);
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
  if (!isheaterhours() && (heater_red_sts || heater_green_sts)) {
    heaterDisableFlag = true;
  }
}

//you get it by now. Except this is executed every 1.4 secs...
void secondTimer() {
  appdata.cost = getHeatingTime() * 1900 + getFilterTime() * 40 + getAirTime() * 800 + appdata.uptime * 2; //wattSeconds
  appdata.cost /= 3600000.0; //kWh
  appdata.cost *= myConfig.price; //money
  uint32_t t = DateTime.now();
  appdata.uptime = t - uptimestamp;

  sendWSmessage();
}
