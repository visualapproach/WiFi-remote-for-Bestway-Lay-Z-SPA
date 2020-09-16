/*
   https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA

   To remove the log file, go to webadress IP/remove.html
   To look at the log file, go to webadress IP/eventlog.csv
   To upload a file to the filesystem, go to webadress IP/upload.html
   To check last reboot time, go to IP/tmp.txt
   "IP" is the local IP of the ESP.

   The main web page "SPA control" should be easy to understand since it mimics the pump display panel. Sort of.
   The slider to set target temperature is not implemented yet, so it's basically just a monitor.
   Use the UP/DOWN buttons to change the temp.

   The "reset timer" is meant to be pressed when new chlorine is added. That way you know when it's time to do it again.

   To force AP-mode, press "C/F" and then POWER on the pump. I thought this key combination would be unusual enough to
   not trigger AP mode accidentally. Not that it hurts anything, but it might be annoying, and leaves an open wifi for a short time.

   See next tab for more info
*/



#include "pitches.h"
#include "a_globals.h"

void setup() {
  startup();
}

void loop() {
  handleData();               // talk to display and controller
  yield();
  webSocket.loop();           // constantly check for websocket events
  yield();
  server.handleClient();      // run the server
  yield();
  ArduinoOTA.handle();        // listen for OTA events
  delay(50);
}

//This function handles most of the high level logic
//regarding status variables updates, and button presses
//and scheduled events.
void handleData() {

  if (DSP_BRT_IN == 0) return; //indication of connection
  updateDSP(DSP_BRT_IN);      //CIO decides brightness
  realBTN = getBTN();


  //fetch target temperature
  if (fetchTargetTemp == 1 && cur_tmp_str == "   ") fetchTargetTemp = 2;
  if (fetchTargetTemp == 2 && (cur_tmp_str != "   ")) {
    set_tmp_val = cur_tmp_val;
    fetchTargetTemp = 0;
  }


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
  //compare prev and current (new) data. Filter out one time glitches
  if (newData) {
    newData = false;
    for (int i = 0; i < 11; i++) {
      if (prev_DSP_IN[i] != DSP_IN[i]) {
        changed = true;
      }
    }
    if (!changed) {
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

  //convert bits in the transmitted array to easy to understand variables
  locked_sts = DSP_OUT[LCK_IDX] & (1 << LCK_BIT);
  power_sts = DSP_OUT[PWR_IDX] & (1 << PWR_BIT);
  air_sts = DSP_OUT[AIR_IDX] & (1 << AIR_BIT);
  heater_red_sts = DSP_OUT[REDHTR_IDX] & (1 << REDHTR_BIT);
  heater_green_sts = DSP_OUT[GRNHTR_IDX] & (1 << GRNHTR_BIT);
  filter_sts = DSP_OUT[FLT_IDX] & (1 << FLT_BIT);
  celsius_sts = DSP_OUT[C_IDX] & (1 << C_BIT);

  //feedback to user - play notes on change
  if (prevlocked < locked_sts) playOff(); //locking
  if (prevpwr < power_sts) playOn();      //powering up
  if (prevair < air_sts) {                //air bubbles turned on
    playOn();
    airStart = DateTime.now();
  }
  if (prevheater < heater_red_sts) {      //heating element turning on
    playOn();
    heaterStart = DateTime.now();
  }
  if (prevfilter < filter_sts) {          //filter pump turning on
    playOn();
    filterStart = DateTime.now();
  }
  if (prevlocked > locked_sts) playOn();
  if (prevpwr > power_sts) playOff();
  if (prevair > air_sts) {
    playOff();
    appdata.airtime += DateTime.now() - airStart;
  }
  if (prevheater > heater_red_sts) {
    playOff();
    appdata.heattime += DateTime.now() - heaterStart;
  }
  if (prevfilter > filter_sts) {
    playOff();
    appdata.filtertime += DateTime.now() - filterStart;
  }

  schedule(); //check automated tasks
  validateButtons();


  checkTargetTempNeeded();

  if (realchange) {
    sendWSmessage(); //to webclients
    savelog();       //saves status variables to LittleFS
    saveappdata();   //saves uptime, heating time etc to LittleFS
  }
}

void releaseButtons() {
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
        if (((heater_green_sts || heater_red_sts) == heater_cmd) || power_sts == false || locked_sts == true) {
          virtualBTN = NOBTN;
          savedHeaterState = heater_cmd;
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

  if (autoBTN != NOBTN) {
    switch (autoBTN) {
      case HTR:
        if (power_sts == false || locked_sts == true) {
          autoBTN = NOBTN;
        }
        if ((heater_green_sts || heater_red_sts) == heater_cmd) {
          heaterDisableFlag = false;
          heaterEnableFlag = false;
          autoBTN = NOBTN;
        }
        break;
      case FLT:
        if (power_sts == false || locked_sts == true) {
          autoBTN = NOBTN;
        }
        if (filter_sts == filter_cmd) {
          filterOffFlag = false; //mission accomplished, don't trigger this again
          filterOnFlag = false; //mission accomplished, don't trigger this again
          if (filter_cmd)
            textOut("on");
          else
            textOut("off");
          autoBTN = NOBTN;
        }
        break;
      case LCK:
        if (locked_sts == locked_cmd || power_sts == false) {
          autoBTN = NOBTN;
        }
        break;
      case PWR:
        if (power_sts == power_cmd) {
          autoBTN = NOBTN;
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

void setHeater(bool state) {
  if (state != (heater_red_sts || heater_green_sts)) {
    if (unlockDevice()) {
      autoBTN = HTR;
      heater_cmd = state;
    }
  }
}

void setFilter(bool state) {
  if (unlockDevice()) {
    autoBTN = FLT;
    filter_cmd = state;
  }
}

bool unlockDevice() {
  if (!power_sts) {
    autoBTN = PWR;
    power_cmd = 1;
    return false;
  }
  else if (locked_sts) {
    autoBTN = LCK;
    locked_cmd = 0;
    return false;
  }
  else return true;
}

void validateButtons() {
  //realBTN overrides virtualBTN
  //autoBTN overrides realBTN
  //realBTN is what is physically pressed on the display.
  //virtualBTN is coming from a web client
  //BTN_OUT is what the CIO (pump computer) sees (from ISR_funcs tab)
  //we need to discard forbidden buttons before setting BTN_OUT
  releaseButtons(); //release them before setting BTN_OUT in case we're already good
  uint16_t tmpButton;

  tmpButton = virtualBTN;
  if (realBTN != NOBTN) tmpButton = realBTN;

  //discard forbidden actions
  if (myConfig.automode && !isheaterhours()) {
    if (tmpButton == HTR) {
      if (!(heater_red_sts || heater_green_sts)) {
        tmpButton = NOBTN;        //discard button if not allowed
        virtualBTN = NOBTN;
        textOut("  nono");        //spank the user
      }
    }
  }
  if (autoBTN != NOBTN) tmpButton = autoBTN;

  //check special combo UNIT-POWER to force AP mode
  if (realBTN == PWR && prevBTN == UNT) {
    textOut(F("cfg"));
    enterAPmode();
  }
  prevBTN = realBTN;

  if (realBTN == UP || realBTN == DWN) fetchTargetTemp = 1;


  //BTN_OUT will be sent to CIO at any time it is requested (via ISR)
  BTN_OUT = tmpButton;
}



//turn off heater but keep filter pump running if wanted
bool isheaterhours() {
  DateTimeParts p = DateTime.getParts();
  return myConfig.heaterhours[p.getHours()];
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



//DSP comm
//Polling wich button is pressed
uint16_t getBTN() {
  uint16_t result = 0;

  //send request
  //pinMode(DATA_DSP_PIN, OUTPUT);
  digitalWrite(CS_DSP_PIN, LOW); //start of packet
  delayMicroseconds(50);
  sendBitsToDSP(DSP_RQ, 8); //request button presses
  result = receiveBitsFromDSP();
  digitalWrite(CS_DSP_PIN, HIGH); //end of packet
  delayMicroseconds(30);

  return result;
}

void sendBitsToDSP(uint32_t outBits, int bitsToSend) {
  pinMode(DATA_DSP_PIN, OUTPUT);
  delayMicroseconds(20);
  for (int i = 1; i <= bitsToSend; i++) {
    digitalWrite(CLK_DSP_PIN, LOW);
    digitalWrite(DATA_DSP_PIN, outBits & (1 << (bitsToSend - i)));
    delayMicroseconds(20);
    digitalWrite(CLK_DSP_PIN, HIGH);
    delayMicroseconds(20);
  }

}

uint16_t receiveBitsFromDSP() {
  //bitbanging the answer from Display
  uint16_t result = 0;
  pinMode(DATA_DSP_PIN, INPUT);

  for (int i = 0; i < 16; i++) {
    digitalWrite(CLK_DSP_PIN, LOW);  //clock leading edge
    delayMicroseconds(20);
    digitalWrite(CLK_DSP_PIN, HIGH); //clock trailing edge
    delayMicroseconds(20);
    result |= digitalRead(DATA_DSP_PIN) << (15 - i);
  }

  return result;
}

void updateDSP(uint8_t brightness) {
  delayMicroseconds(30);
  digitalWrite(CS_DSP_PIN, LOW); //start of packet
  sendBitsToDSP(0x80, 8);
  digitalWrite(CS_DSP_PIN, HIGH); //end of packet

  delayMicroseconds(50);
  digitalWrite(CS_DSP_PIN, LOW);//start of packet
  sendBitsToDSP(0x02, 8);
  digitalWrite(CS_DSP_PIN, HIGH);//end of packet

  delayMicroseconds(50);
  digitalWrite(CS_DSP_PIN, LOW);//start of packet
  for (int i = 0; i < 11; i++)
    sendBitsToDSP(DSP_OUT[i], 8);
  digitalWrite(CS_DSP_PIN, HIGH);//end of packet

  delayMicroseconds(50);
  digitalWrite(CS_DSP_PIN, LOW);//start of packet
  sendBitsToDSP(brightness, 8);
  digitalWrite(CS_DSP_PIN, HIGH);//end of packet
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
  if (myConfig.audio == 0) return;
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
  if (myConfig.audio == 0) return;
  int longnote = 125;
  int shortnote = 63;
  tone(AUDIO_OUT_PIN, NOTE_C6, shortnote);
  delay(shortnote);
  tone(AUDIO_OUT_PIN, NOTE_C7, shortnote);
  delay(shortnote);
  noTone(AUDIO_OUT_PIN);
}

void playOff() {
  if (myConfig.audio == 0) return;
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
      delay(230);
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

//Keep track of heating time
uint32_t getHeatingTime() {
  uint32_t t = 0;
  if (heater_red_sts) {
    t = DateTime.now() - heaterStart;
  }
  return appdata.heattime + t;
}
uint32_t getFilterTime() {
  uint32_t t = 0;
  if (filter_sts) {
    t = DateTime.now() - filterStart;
  }
  return appdata.filtertime + t;
}
uint32_t getAirTime() {
  uint32_t t = 0;
  if (air_sts) {
    t = DateTime.now() - airStart;
  }
  return appdata.airtime + t;
}
uint32_t getUpTime() {
  uint32_t t = 0;
  t = DateTime.now() - uptimestamp;
  return appdata.uptime + t;
}
