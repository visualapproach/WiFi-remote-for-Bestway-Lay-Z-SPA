/*
 * Interrupt service routines
 * Basically bitbanging data to and from the CIO
 * 
 * Direct port manipulation is used, not for speed, but because
 * everything must be stored in iRAM. A digitalRead() would crash the device since it is not stored in RAM.
 * 
 * 
 */


//CIO comm
//end of packet
void ICACHE_RAM_ATTR slaveDeselected() {
  //process latest data and enter corresponding mode (like listen for DSP_STS or send BTN_OUT)
  //pinMode(DATA_CIO_PIN, INPUT);
  WRITE_PERI_REG( PIN_DIR_INPUT, 1 << DATA_CIO_PIN);

  uint8_t msg = receivedByte;
  bitCount = 0;
  byteCount = 0;
  readlock = false;

  switch (msg) {
    case DSP_STS1:
      chk1 = true;
      break;
    case DSP_STS2:
      if (chk1) {
        chk2 = true;
        chk1 = false;
      }
      break;
    default:
      if (chk3) {
        DSP_BRT_IN = msg;
        chk3 = false;
      }
      if (chk2) {
        chk3 = true;
        chk2 = false;
        newData = true;
      }
      break;
  }
}

//CIO comm
//packet start
void ICACHE_RAM_ATTR ISR_slaveSelect() {
  if (!(READ_PERI_REG(PIN_IN) & (1 << CS_CIO_PIN))) {
    CS_CIO_active = true;
    readlock = true;
  }
  else {
    CS_CIO_active = false;
    CS_answermode = false;
    slaveDeselected();
  }
}



//CIO comm
//Read incoming bits, and take action after a complete byte
void ICACHE_RAM_ATTR ISR_CLK_CIO_PIN_byte() {

  if (!CS_CIO_active) return;
  //CS line is active, so send/receive bits on DATA line

  bool clockstate = READ_PERI_REG(PIN_IN) & (1 << CLK_CIO_PIN);
  //send bits on low clock (falling edge)
  if (!clockstate & CS_answermode) {
    //send BTN_OUT
    sendBit--;
    if (BTN_OUT & (1 << sendBit)) {
      //digitalWrite(DATA_CIO_PIN, HIGH);
      WRITE_PERI_REG( PIN_OUT_SET, 1 << DATA_CIO_PIN);
    }
    else {
      //digitalWrite(DATA_CIO_PIN, LOW);
      WRITE_PERI_REG( PIN_OUT_CLEAR, 1 << DATA_CIO_PIN);
    }
  }

  //read bits on high clock (rising edge)
  if (clockstate & !CS_answermode) {
    //read data pin for a byte
    //receivedByte = (receivedByte << 1) | digitalRead(DATA_CIO_PIN);
    receivedByte = (receivedByte << 1) | ( ( (READ_PERI_REG(PIN_IN) & (1 << DATA_CIO_PIN)) ) > 0);
    bitCount++;
    if (bitCount == 8) {
      bitCount = 0;
      if (chk2) {
        DSP_IN[byteCount] = receivedByte;
        byteCount++;
      }
      else if (receivedByte == DSP_RQ) {
        sendBit = 16;
        CS_answermode = true;
        //pinMode(DATA_CIO_PIN, OUTPUT);
        WRITE_PERI_REG( PIN_DIR_OUTPUT, 1 << DATA_CIO_PIN);
      }
    }
  }
}
