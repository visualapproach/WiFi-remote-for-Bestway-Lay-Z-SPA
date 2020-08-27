/*
upload this firmware and let it run for a minute or two. Then browse to the ip/rawdata.csv - for example 192.168.4.123/rawdata.csv
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
  getBTN();
  updateDSP(DSP_BRT_IN);
  delay(300);
}




void saveData() {
  File logfile;
  String fname = "/rawdata.csv";

  logfile = SPIFFS.open(fname, "a");
  if (logfile) {
    for(int i = 0; i < 11; i++){
      logfile.print(DSP_IN[i], HEX);
      if(i < 10) logfile.print(',');
    }
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
    sendByteToDSP(DSP_IN[i], 8);
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(50);
  digitalWrite(CS_DSP_PIN, LOW);
  sendByteToDSP(brightness, 8);
  digitalWrite(CS_DSP_PIN, HIGH);
  delayMicroseconds(50);
}
