/*
upload this firmware and let it run for a minute or two. Press a couple of touch buttons.
Then browse to the ip/rawdata.csv - for example 192.168.4.123/rawdata.csv
copy and paste it into a new issue on github.
*/


#include "a_globals.h"

void setup() {
  startup();
}


void loop() {
  static int counter = 0;
  saveData();               // log raw data for analysis
  yield();
  server.handleClient();      // run the server
  yield();
  ArduinoOTA.handle();        // listen for OTA events
  BTN_OUT = getBTN();
  litDSP(0xF1);
  delay(500);
  darkDSP(0xF1);
  delay(300);
}




void saveData() {
  File logfile;
  String fname = "/rawdata.csv";

  logfile = LittleFS.open(fname, "a");
  if (logfile) {
    for(int i = 0; i < 11; i++){
      logfile.print(DSP_IN[i], HEX);
      logfile.print(',');
    }
    logfile.print(BTN_OUT, HEX);
    logfile.println();
    logfile.close();
  }
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

uint8_t ALLON[11] = {0x03, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t ALLOFF[11] = {0x03, 0x80, 0x7F, 0x80, 0x7F, 0x80, 0x7F, 0x80, 0xFF, 0x80, 0xFF};

void litDSP(uint8_t brightness) {
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
    sendByteToDSP(ALLON[i], 8);
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(50);
  digitalWrite(CS_DSP_PIN, LOW);
  sendByteToDSP(brightness, 8);
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(50);
}
void darkDSP(uint8_t brightness) {
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
    sendByteToDSP(ALLOFF[i], 8);
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(50);
  digitalWrite(CS_DSP_PIN, LOW);
  sendByteToDSP(brightness, 8);
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(50);
}
