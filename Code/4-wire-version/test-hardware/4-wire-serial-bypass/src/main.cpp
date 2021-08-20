#include <Arduino.h>

const int CIO_TX = D2;
const int CIO_RX = D3;
const int DSP_TX = D6;
const int DSP_RX = D7;

void setup() {
  // put your setup code here, to run once:
  pinMode(CIO_TX, INPUT);
  pinMode(CIO_RX, OUTPUT);
  pinMode(DSP_TX, INPUT);
  pinMode(DSP_RX, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(DSP_RX, digitalRead(CIO_TX));
  digitalWrite(CIO_RX, digitalRead(DSP_TX));
}