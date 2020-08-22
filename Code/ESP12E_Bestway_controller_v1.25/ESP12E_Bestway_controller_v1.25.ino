/*
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * 
 * To remove the log file, go to webadress IP/remove.html
 * To look at the log file, go to webadress IP/eventlog.csv
 * To upload a file to the filesystem, go to webadress IP/upload.html
 * "IP" is the local IP of the ESP.
 * 
 * The main web page "SPA control" should be easy to understand since it mimics the pump display panel. Sort of.
 * The slider to set target temperature is not implemented yet, so it's basically just a monitor.
 * Use the UP/DOWN buttons to change the temp.
 * 
 * The "reset timer" is meant to be pressed when new chlorine is added. That way you know when it's time to do it again.
 * 
 * To force AP-mode, press "C/F" and then POWER on the pump. I thought this key combination would be unusual enough to
 * not trigger AP mode accidentally. Not that it hurts anything, but it might be annoying, and leaves an open wifi for a short time.
 */



#include "pitches.h"
#include "a_globals.h"

void setup() {
  startup();
}


void loop() {
  static int counter = 0;
  handleData();               // talk to display and controller
  yield();
  webSocket.loop();           // constantly check for websocket events
  yield();
  server.handleClient();      // run the server
  yield();
  ArduinoOTA.handle();        // listen for OTA events
  if (counter++ > 300) {
    if (!DateTime.isTimeValid()) {
      Serial.println(F("Failed to get time from server, retry."));
      DateTime.begin();
    }
    sendWSmessage();
    counter -= 300;
  }
}


//store UNIX time in a SPIFFS file so we can calculate how many days since we added CL to the water
void setClTimer() {
  clTime = DateTime.now();
  //save to spiffs
  //while (!DateTime.isTimeValid()) delay(1000);
  File clFile;
  String fname = "/clTimestamp";
  clFile = SPIFFS.open(fname, "w");
  if (clFile) clFile.write((char *)&clTime, 4);
  clFile.close();
}

//Keep track of heating time
uint32_t getHeatingTime() {
  uint32_t t = 0;
  if (heater_red_sts) {
    t = DateTime.now() - heaterStart;
  }
  return heatingSeconds + t;
}


//This function handles most of the high level logic
//regarding status variables updates, and button presses
//and scheduled events.
void handleData() {

  if (DSP_BRT_IN == 0) return; //indication of connection
  updateDSP(DSP_BRT_IN);      //CIO decides brightness
  yield();                    //calm the wifi gods
  realBTN = getBTN();
  yield();
  releaseVirtualButtons();

  //fetch target temperature
  if (fetchTargetTemp == 1 && cur_tmp_str == "   ") fetchTargetTemp = 2;
  if (fetchTargetTemp == 2 && (cur_tmp_str != "   ")) {
    set_tmp_val = cur_tmp_val;
    fetchTargetTemp = 0;
  }

  filterButtons();
  schedule();

  bool prevlocked = locked_sts;
  bool prevpwr = power_sts;
  bool prevair = air_sts;
  bool prevheater = heater_red_sts;
  bool prevheatergreen = heater_green_sts;
  bool prevfilter = filter_sts;

  //copy DSP_IN (from CIO) to DSP_OUT (to the display) (unless we want to show custom values)
  bool changed = false;
  bool realchange = false;
  static uint8_t prev_DSP_IN[11]; //due to glitches, wait for two equal messages from CIO
  static uint8_t prev_DSP_valid[11]; //to check for real changes
  //compare prev and current (new) data.
  if (newData) {
    newData = false;
    for (int i = 0; i < 11; i++) {
      if (prev_DSP_IN[i] != DSP_IN[i]) {
        changed = true;
      }
    }
    if(!changed){
      for (int i = 0; i < 11; i++) {
        DSP_valid[i] = DSP_IN[i]; //data not changed
      }
    } else {
      for (int i = 0; i < 11; i++) {
        DSP_valid[i] = prev_DSP_valid[i]; //data changed but not valid
      }      
    }
    for (int i = 0; i < 11; i++) {
      if (DSP_valid[i] != prev_DSP_valid[i]) {
        realchange = true;
      }
    }

    //copy current data to prev data
    for (int i = 0; i < 11; i++) {
      prev_DSP_IN[i] = DSP_IN[i];
      prev_DSP_valid[i] = DSP_valid[i];
    }
  }
  updateDSPOUT();

  //update status variables
  //convert 7-segment code to real characters
  char c1, c2, c3;
  c1 = getChar(DSP_OUT[DGT1_IDX]);
  c2 = getChar(DSP_OUT[DGT2_IDX]);
  c3 = getChar(DSP_OUT[DGT3_IDX]);
  cur_tmp_str = String(c1) + String(c2) + String(c3);
  cur_tmp_val = cur_tmp_str.toInt();

  locked_sts = DSP_OUT[LCK_IDX] & (1 << LCK_BIT);
  power_sts = DSP_OUT[PWR_IDX] & (1 << PWR_BIT);
  air_sts = DSP_OUT[AIR_IDX] & (1 << AIR_BIT);
  heater_red_sts = DSP_OUT[REDHTR_IDX] & (1 << REDHTR_BIT);
  heater_green_sts = DSP_OUT[GRNHTR_IDX] & (1 << GRNHTR_BIT);
  filter_sts = DSP_OUT[FLT_IDX] & (1 << FLT_BIT);
  celsius_sts = DSP_OUT[C_IDX] & (1 << C_BIT);


  //feedback to user - play notes on change
  if (prevlocked < locked_sts) playOff();
  if (prevpwr < power_sts) playOn();
  if (prevair < air_sts) playOn();
  if (prevheater < heater_red_sts) {
    playOn();
    heaterStart = DateTime.now();
  }
  if (prevfilter < filter_sts) playOn();
  if (prevlocked > locked_sts) playOn();
  if (prevpwr > power_sts) playOff();
  if (prevair > air_sts) playOff();
  if (prevheater > heater_red_sts) {
    playOff();
    heatingSeconds += DateTime.now() - heaterStart;
  }
  if (prevfilter > filter_sts) playOff();
  
  yield();
  
  checkTargetTempNeeded();
  
  if (realchange) {
    sendWSmessage(); //to webclients
    savelog();       //to SPIFFS
  }
}

void releaseVirtualButtons() {
  //If power is on and display is unlocked,
  //release BTN_OUT after change has occured
  if (virtualBTN != NOBTN) {
    switch (virtualBTN) {
      case LCK:
        if (locked_sts == locked_cmd || power_sts == false) {
          virtualBTN = NOBTN;
        }
        break;
      case PWR:
        if (power_sts == power_cmd) {
          virtualBTN = NOBTN;
        }
        break;
      case AIR:
        if (air_sts == air_cmd || power_sts == false || locked_sts == true) {
          virtualBTN = NOBTN;
        }
        break;
      case HTR:
        if (heater_green_sts == heater_cmd || heater_red_sts == heater_cmd || power_sts == false || locked_sts == true) {
          virtualBTN = NOBTN;
        }
        break;
      case FLT:
        if (filter_sts == filter_cmd || power_sts == false || locked_sts == true) {
          virtualBTN = NOBTN;
        }
        break;
      case UNT:
        if (celsius_sts == celsius_cmd || power_sts == false || locked_sts == true) {
          virtualBTN = NOBTN;
        }
        break;
      case UP:
        //wait for digits and capture
        if (millis() > BTN_timeout) {
          virtualBTN = NOBTN;
          fetchTargetTemp = 1;
        }
        break;
      case DWN:
        //wait for digits and capture
        if (millis() > BTN_timeout) {
          virtualBTN = NOBTN;
          fetchTargetTemp = 1;
        }
        break;
    }
  }
}


void updateDSPOUT() {
  bool changed = false;
  for (int i = 0; i < 11; i++) {
      DSP_OUT[i] = DSP_valid[i];
  }
}

void schedule() {
  DateTimeParts p = DateTime.getParts();
  int h = p.getHours();
  if (h != prevHour) {
    if (h == filterOffHour) {
      turnOffFilter();
      Serial.println("auto filter off");
      textOut("off");
    }
    if (h == filterOnHour) {
      turnOnFilter();
      Serial.println("auto filter on");
      textOut("on");
    }
  }
}

void turnOffFilter() {
  DateTimeParts p = DateTime.getParts();
  int h = p.getHours();
  if (unlockDevice()) {
    virtualBTN = FLT;
    filter_cmd = 0;
    prevHour = h;
    savedHeaterState = heater_red_sts | heater_green_sts;
  }
}

void turnOnFilter() {
  DateTimeParts p = DateTime.getParts();
  int h = p.getHours();
  if (unlockDevice()) {
    if (savedHeaterState) {
      virtualBTN = HTR;
      heater_cmd = 1;
    } else {
      virtualBTN = FLT;
      filter_cmd = 1;
    }
    prevHour = h;
  }
}


void filterButtons() {
  //real buttons overrides virtual buttons
  uint16_t tempButton; //we need to filter forbidden buttons before setting BTN_OUT
  if (realBTN == NOBTN) {
    tempButton = virtualBTN;
  } else {
    tempButton = realBTN;
    if (realBTN == PWR && prevBTN == UNT) { //UNIT-POWER = special combo to activate config (AP portal) mode
      textOut(F("cfg"));
      enterAPmode();
    }
    prevBTN = realBTN;
  }
  //forbid heater button at defined hours. Not really implemented.
  if (isForbiddenHeaterHours()) {
    if (tempButton == HTR) tempButton = NOBTN;
    if (heater_red_sts || heater_green_sts) turnOffHeater();
  }
  else
  {
    restoreHeater(); //remember to change status when heater is switched on or off manually************
  }

  BTN_OUT = tempButton;
}

void turnOffHeater() {
  if (unlockDevice()) {
    virtualBTN = HTR;
    heater_cmd = 0;
  }
}

void restoreHeater() {

}

bool unlockDevice() {
  if (!power_sts) {
    virtualBTN = PWR;
    power_cmd = 1;
    return false;
  }
  else if (locked_sts) {
    virtualBTN = LCK;
    locked_cmd = 0;
    return false;
  }
  else return true;
}

//turn off heater but keep filter pump running if wanted
bool isForbiddenHeaterHours() {
  DateTimeParts p = DateTime.getParts();
  if (forbiddenHeaterHours[p.getHours()]) return true;
  return false;
}

void checkTargetTempNeeded() {
  //check if we need to populate set_tmp
  if (set_tmp_val < 20)
    if (unlockDevice()) {
      if (millis() > target_timeout) {
        virtualBTN = DWN;
        BTN_timeout = millis() + 500;
        target_timeout = millis() + 10000;
      }
    }
}

void savelog() {
  //Don't start logging until time is set
  if (!DateTime.isTimeValid()) return;
  File logfile;
  String fname = "/eventlog.csv";

  if (!SPIFFS.exists(fname)) {
    logfile = SPIFFS.open(fname, "a");
    logfile.println(F("Now(), simple datetime, temp, target temp, locked, power, air, heater, heating, filter, celsius, heating hours (acc), heating ratio"));
  } else {
    logfile = SPIFFS.open(fname, "a");
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
}

//DSP comm
//Polling wich button is pressed
uint16_t getBTN() {
  uint16_t result = 0;

  //send request
  pinMode(DATA_DSP_PIN, OUTPUT);
  digitalWrite(CS_DSP_PIN, LOW);
  delayMicroseconds(50);
  for (int i = 0; i < 8; i++) {
    digitalWrite(CLK_DSP_PIN, LOW);
    digitalWrite(DATA_DSP_PIN, DSP_RQ & (1 << 7 - i));
    delayMicroseconds(20);
    digitalWrite(CLK_DSP_PIN, HIGH);
    delayMicroseconds(20);
  }

  //bitbanging the answer from Display
  pinMode(DATA_DSP_PIN, INPUT);

  for (int i = 0; i < 16; i++) {
    digitalWrite(CLK_DSP_PIN, LOW);  //clock leading edge
    delayMicroseconds(20);
    digitalWrite(CLK_DSP_PIN, HIGH); //clock trailing edge
    delayMicroseconds(20);
    result |= digitalRead(DATA_DSP_PIN) << (15 - i);
  }
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(50);
  return result;
}

void sendByteToDSP(uint32_t outBits, int bitsToSend) {
  delayMicroseconds(20);
  pinMode(DATA_DSP_PIN, OUTPUT);
  for (int i = 1; i <= bitsToSend; i++) {
    digitalWrite(CLK_DSP_PIN, LOW);
    digitalWrite(DATA_DSP_PIN, outBits & (1 << (bitsToSend - i)));
    delayMicroseconds(20);
    digitalWrite(CLK_DSP_PIN, HIGH);
    delayMicroseconds(20);
  }
}


void updateDSP(uint8_t brightness) {
  delayMicroseconds(50);
  digitalWrite(CS_DSP_PIN, LOW);
  sendByteToDSP(0x80, 8);
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(CS_DSP_PIN, LOW);
  sendByteToDSP(0x02, 8);
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(20);

  digitalWrite(CS_DSP_PIN, LOW);
  yield();
  for (int i = 0; i < 11; i++)
    sendByteToDSP(DSP_OUT[i], 8);
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(50);
  digitalWrite(CS_DSP_PIN, LOW);
  sendByteToDSP(brightness, 8); 
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(50);
}

//match 7 segment pattern to a real digit
char getChar(uint8_t value) {
  for (int index = 0; index < sizeof(CHARCODES); index++) {
    if (value == CHARCODES[index]) {
      return CHARS[index];
    }
  }
  return '?';
}
//match real digit to a 7 segment pattern

char getCode(char value) {
  for (int index = 0; index < sizeof(CHARS); index++) {
    if (value == CHARS[index]) {
      return CHARCODES[index];
    }
  }
  return 0x00;
}

void playIntro() {

  int longnote = 125;
  int shortnote = 63;

  tone(AUDIO_OUT_PIN, NOTE_C7, longnote);
  delay(2 * longnote);
  tone(AUDIO_OUT_PIN, NOTE_G6, shortnote);
  delay(2 * shortnote);
  tone(AUDIO_OUT_PIN, NOTE_G6, shortnote);
  delay(2 * shortnote);
  tone(AUDIO_OUT_PIN, NOTE_A6, longnote);
  delay(2 * longnote);
  tone(AUDIO_OUT_PIN, NOTE_G6, longnote);
  delay(2 * longnote);
  //paus
  delay(2 * longnote);
  tone(AUDIO_OUT_PIN, NOTE_B6, longnote);
  delay(2 * longnote);
  tone(AUDIO_OUT_PIN, NOTE_C7, longnote);
  delay(2 * longnote);
  noTone(AUDIO_OUT_PIN);
}

void playOn() {
  int longnote = 125;
  int shortnote = 63;
  tone(AUDIO_OUT_PIN, NOTE_C6, shortnote);
  delay(shortnote);
  tone(AUDIO_OUT_PIN, NOTE_C7, shortnote);
  delay(shortnote);
  noTone(AUDIO_OUT_PIN);
}

void playOff() {
  int longnote = 125;
  int shortnote = 63;
  tone(AUDIO_OUT_PIN, NOTE_C6, shortnote);
  delay(shortnote);
  tone(AUDIO_OUT_PIN, NOTE_C5, shortnote);
  delay(shortnote);
  noTone(AUDIO_OUT_PIN);
}

void textOut(String txt) {
  int len = txt.length();
  if (len >= 3) {
    for (int i = 0; i < len - 2; i++) {
      DSP_OUT[DGT1_IDX] = getCode(txt.charAt(i));
      DSP_OUT[DGT2_IDX] = getCode(txt.charAt(i + 1));
      DSP_OUT[DGT3_IDX] = getCode(txt.charAt(i + 2));
      updateDSP(0xF1);
      delay(333);
    }
  }
  else if (len == 2) {
    DSP_OUT[DGT1_IDX] = getCode(' ');
    DSP_OUT[DGT2_IDX] = getCode(txt.charAt(0));
    DSP_OUT[DGT3_IDX] = getCode(txt.charAt(1));
    updateDSP(0xF1);
  }
  else if (len == 1) {
    DSP_OUT[DGT1_IDX] = getCode(' ');
    DSP_OUT[DGT2_IDX] = getCode(' ');
    DSP_OUT[DGT3_IDX] = getCode(txt.charAt(0));
    updateDSP(0xF1);
  }
}
