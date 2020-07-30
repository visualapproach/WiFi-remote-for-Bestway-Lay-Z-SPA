#include "pitches.h"
#include "a_globals.h"

void setup() {
  startup();
}

int counter = 0;
void loop() {
  handleData();               // talk to display and controller
  yield();
  webSocket.loop();           // constantly check for websocket events
  yield();
  server.handleClient();      // run the server
  yield();
  ArduinoOTA.handle();        // listen for OTA events
  if (counter++ > 1000) {
    if (!DateTime.isTimeValid()) {
      Serial.println(F("Failed to get time from server, retry."));
      DateTime.begin();
    }
    sendWSmessage();
    counter -= 1000;
  }
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
      if (cmd > 0xF) {
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
            heater_cmd = !heater_red_sts;
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
          default:
            break;
        }
      }
      sendWSmessage();
      break;
  }
}

void sendWSmessage() {
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
  sendjson += "\"time\":\"" + String(DateTime.format(DateFormatter::SIMPLE) + "\"");
  sendjson += "}";
  webSocket.broadcastTXT(sendjson);
  //Serial.println("sent message: " + sendjson);
}

void handleData() {
  if (DSP_BRT_IN > 0) { //indication of connection
    updateDSP(DSP_BRT_IN);
    yield();            //calm the wifi gods
    realBTN = getBTN();
    yield();
    //If power is on and display is unlocked,
    //keep BTN_OUT until change has occured
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

    //fetch target temperature
    if (fetchTargetTemp == 1 && cur_tmp_str == "   ") fetchTargetTemp = 2;
    if (fetchTargetTemp == 2 && (cur_tmp_str != "   ")) {
      set_tmp_val = cur_tmp_val;
      fetchTargetTemp = 0;
    }

    filterButtons();

    bool prevlocked = locked_sts;
    bool prevpwr = power_sts;
    bool prevair = air_sts;
    bool prevheater = heater_red_sts;
    bool prevheatergreen = heater_green_sts;
    bool prevfilter = filter_sts;

    //copy DSP_IN (from CIO) to DSP_OUT (to the display) (unless we want to show custom values)
    bool changed = false;
    for (int i = 0; i < 11; i++) {
      while (readlock); //wait until data is free to read. This is probably not needed.
      if (DSP_OUT[i] != DSP_IN[i]) {
        DSP_OUT[i] = DSP_IN[i];
        changed = true;
      }
    }
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


    if (changed) {
      sendWSmessage(); //to webclients
      savelog();       //to SPIFFS
    }

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
    logfile.println(F("Now(), simple datetime, temp, target temp, locked, power, air, heater, heating, filter, celsius, heating hours (acc)"));
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
  logfile.println(heatingSeconds / 3600.0);

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
  sendByteToDSP(brightness, 8); //CIO decides brightness
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
