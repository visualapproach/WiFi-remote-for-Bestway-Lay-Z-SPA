#include "BWC_6w_type1.h"
#include "pitches.h"

CIO *pointerToClass;

static void IRAM_ATTR chipselectpin(void) {
  pointerToClass->packetHandler();
}
static void IRAM_ATTR clockpin(void) {
  pointerToClass->clkHandler();
}

void CIO::begin(int cio_data_pin, int cio_clk_pin, int cio_cs_pin) {
  pointerToClass = this;
  _DATA_PIN = cio_data_pin;
  _CLK_PIN = cio_clk_pin;
  _CS_PIN = cio_cs_pin;
  pinMode(_CS_PIN, INPUT);
  pinMode(_DATA_PIN, INPUT);
  pinMode(_CLK_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(_CS_PIN), chipselectpin, CHANGE);
  attachInterrupt(digitalPinToInterrupt(_CLK_PIN), clockpin, CHANGE); //Write on falling edge and read on rising edge
}

void CIO::stop(){
  detachInterrupt(digitalPinToInterrupt(_CS_PIN));
  detachInterrupt(digitalPinToInterrupt(_CLK_PIN));
}


void CIO::loop(void) {
  //newdata is true when a data packet has arrived from cio
  if(newData) {
    newData = false;
    static int capturePhase = 0;
    static uint32_t buttonReleaseTime;

    //capture TARGET after UP/DOWN has been pressed...
    if ((button == ButtonCodes[UP]) || (button == ButtonCodes[DOWN]))
    {
      buttonReleaseTime = millis();
      capturePhase = 1;
    } 
    //require two consecutive messages to be equal before registering
    static uint8_t prev_checksum = 0;
    uint8_t checksum = 0;
    for(int i = 0; i < 11; i++){
      checksum += _payload[i];
    }
    if(checksum != prev_checksum) {
      prev_checksum = checksum;
      return;
    }
    
    //copy private array to public array
    for(unsigned int i = 0; i < sizeof(payload); i++){
      payload[i] = _payload[i];
    }

    //determine if anything changed, so we can update webclients
    for(unsigned int i = 0; i < sizeof(payload); i++){
      if (payload[i] != _prevPayload[i]) dataAvailable = true;
      _prevPayload[i] = payload[i];
    }

    brightness = _brightness & 7; //extract only the brightness bits (0-7)
    //extract information from payload to a better format
    states[LOCKEDSTATE] = (payload[LCK_IDX] & (1 << LCK_BIT)) > 0;
    states[POWERSTATE] = (payload[PWR_IDX] & (1 << PWR_BIT)) > 0;
    states[UNITSTATE] = (payload[C_IDX] & (1 << C_BIT)) > 0;
    states[BUBBLESSTATE] = (payload[AIR_IDX] & (1 << AIR_BIT)) > 0;
    states[HEATGRNSTATE] = (payload[GRNHTR_IDX] & (1 << GRNHTR_BIT)) > 0;
    states[HEATREDSTATE] = (payload[REDHTR_IDX] & (1 << REDHTR_BIT)) > 0;
    states[HEATSTATE] = states[HEATGRNSTATE] || states[HEATREDSTATE];
    states[PUMPSTATE] = (payload[FLT_IDX] & (1 << FLT_BIT)) > 0;
    states[CHAR1] = (uint8_t)_getChar(payload[DGT1_IDX]);
    states[CHAR2] = (uint8_t)_getChar(payload[DGT2_IDX]);
    states[CHAR3] = (uint8_t)_getChar(payload[DGT3_IDX]);
    if(HASJETS) states[JETSSTATE] = (payload[HJT_IDX] & (1 << HJT_BIT)) > 0;
    else states[JETSSTATE] = 0;
    //Determine if display is showing target temp or actual temp or anything else.
    //...until 4 seconds after UP/DOWN released
    if((millis()-buttonReleaseTime) > 2000) capturePhase = 0;
    //convert text on display to a value if the chars are recognized
    if(states[CHAR1] == '*' || states[CHAR2] == '*' || states[CHAR3] == '*') return;
    String tempstring = String((char)states[CHAR1])+String((char)states[CHAR2])+String((char)states[CHAR3]);
    uint8_t tmpTemp = tempstring.toInt();
    //capture only if showing plausible values (not blank screen while blinking)
    if( (capturePhase == 1) && (tmpTemp > 19) ) {
      states[TARGET] = tmpTemp;
    }
    //wait 6 seconds after UP/DOWN is released to be sure that actual temp is shown
    if( (capturePhase == 0) && (states[CHAR3]!='H') && (states[CHAR3]!=' ') && ((millis()-buttonReleaseTime) > 6000) )
    {
      if(states[TEMPERATURE] != tmpTemp) dataAvailable = true;
      states[TEMPERATURE] = tmpTemp;
    }

    if(states[UNITSTATE] != _prevUNT || states[HEATSTATE] != _prevHTR || states[PUMPSTATE] != _prevFLT) {
      stateChanged = true;
      _prevUNT = states[UNITSTATE];
      _prevHTR = states[HEATSTATE];
      _prevFLT = states[PUMPSTATE];
    }
  }
}

//end of packet
void IRAM_ATTR CIO::eopHandler(void) {
  //process latest data and enter corresponding mode (like listen for DSP_STS or send BTN_OUT)
  //pinMode(_DATA_PIN, INPUT);
  WRITE_PERI_REG( PIN_DIR_INPUT, 1 << _DATA_PIN);
  _byteCount = 0;
  _bitCount = 0;
  uint8_t msg = _receivedByte;

  switch (msg) {
    case DSP_CMD1_MODE6_11_7:
      _CIO_cmd_matches = 1;
      break;
    case DSP_CMD2_DATAWRITE:
      if (_CIO_cmd_matches == 1) {
        _CIO_cmd_matches = 2;
      } else {
        _CIO_cmd_matches = 0; //reset - DSP_CMD1_MODE6_11_7 must be followed by DSP_CMD2_DATAWRITE to activate command
      }
      break;
    default:
      if (_CIO_cmd_matches == 3) {
        _brightness = msg;
        _CIO_cmd_matches = 0;
        newData = true;
    }
      if (_CIO_cmd_matches == 2) {
        _CIO_cmd_matches = 3;
      }
      break;
  }
}

//CIO comm
//packet start
//arduino core 3.0.1+ should work with digitalWrite() now.
void IRAM_ATTR CIO::packetHandler(void) {
  if (!(READ_PERI_REG(PIN_IN) & (1 << _CS_PIN))) {
    //packet start
    _packet = true;
  }
  else {
    //end of packet
    _packet = false;
    _dataIsOutput = false;
    eopHandler();
  }
}

//CIO comm
//Read incoming bits, and take action after a complete byte
void IRAM_ATTR CIO::clkHandler(void) {
  //sanity check on clock signal
  static uint32_t prev_us = 0;
  uint32_t us = micros();
  uint32_t period = 0;
  if(us > prev_us) period = us - prev_us; //will be negative on rollover (once every hour-ish)
  prev_us = us;
  if(period < clk_per) clk_per = period;

  if (!_packet) return;
  //CS line is active, so send/receive bits on DATA line

  bool clockstate = READ_PERI_REG(PIN_IN) & (1 << _CLK_PIN);

  //shift out bits on low clock (falling edge)
  if (!clockstate & _dataIsOutput) {
    //send BTN_OUT
    if (button & (1 << _sendBit)) {
      //digitalWrite(_DATA_PIN, HIGH);
      WRITE_PERI_REG( PIN_OUT_SET, 1 << _DATA_PIN);
    }
    else {
      //digitalWrite(_DATA_PIN, LOW);
      WRITE_PERI_REG( PIN_OUT_CLEAR, 1 << _DATA_PIN);
    }
    _sendBit++;
  if(_sendBit > 15) _sendBit = 0;
  }

  //read bits on high clock (rising edge)
  if (clockstate & !_dataIsOutput) {
    //read data pin to a byte
    //_receivedByte = (_receivedByte << 1) | digitalRead(_DATA_PIN);
    //_receivedByte = (_receivedByte << 1) | ( ( (READ_PERI_REG(PIN_IN) & (1 << _DATA_PIN)) ) > 0);
    //_receivedByte = (_receivedByte >> 1) | digitalRead(_DATA_PIN) << 7;
    _receivedByte = (_receivedByte >> 1) | ( ( (READ_PERI_REG(PIN_IN) & (1 << _DATA_PIN)) ) > 0) << 7;
    _bitCount++;
    if (_bitCount == 8) {
      _bitCount = 0;
      if (_CIO_cmd_matches == 2) { //meaning we have received the header for 11 data bytes to come
        _payload[_byteCount] = _receivedByte;
        _byteCount++;
      }
      else if (_receivedByte == DSP_CMD2_DATAREAD) {
        _sendBit = 8;
        _dataIsOutput = true;
        //pinMode(_DATA_PIN, OUTPUT);
        WRITE_PERI_REG( PIN_DIR_OUTPUT, 1 << _DATA_PIN);
      }
    }
  }
}

uint16_t DSP::getButton(void) {
  if(millis() - _dspLastGetButton > 50){
    uint16_t newButton = 0;
    _dspLastGetButton = millis();
    //send request
    //pinMode(_DATA_PIN, OUTPUT);
    digitalWrite(_CS_PIN, LOW); //start of packet
    delayMicroseconds(50);
    _sendBitsToDSP(DSP_CMD2_DATAREAD, 8); //request button presses
    newButton = _receiveBitsFromDSP();
    digitalWrite(_CS_PIN, HIGH); //end of packet
    delayMicroseconds(30);
    _oldButton = newButton;
    return (newButton);
  } else return (_oldButton);
}


//bitsToSend can only be 8 with this solution of LSB first
void DSP::_sendBitsToDSP(uint32_t outBits, int bitsToSend) {
  pinMode(_DATA_PIN, OUTPUT);
  delayMicroseconds(20);
  for (int i = 0; i < bitsToSend; i++) {
    digitalWrite(_CLK_PIN, LOW);
    digitalWrite(_DATA_PIN, outBits & (1 << i));
    delayMicroseconds(20);
    digitalWrite(_CLK_PIN, HIGH);
    delayMicroseconds(20);
  }
}

uint16_t DSP::_receiveBitsFromDSP() {
  //bitbanging the answer from Display
  uint16_t result = 0;
  pinMode(_DATA_PIN, INPUT);

  for (int i = 0; i < 16; i++) {
    digitalWrite(_CLK_PIN, LOW);  //clock leading edge
    delayMicroseconds(20);
    digitalWrite(_CLK_PIN, HIGH); //clock trailing edge
    delayMicroseconds(20);
  int j = (i+8)%16;  //bit 8-16 then 0-7
    result |= digitalRead(_DATA_PIN) << j;
  }
  return result;
}


void DSP::updateDSP(uint8_t brightness) {
   //refresh display with ~10Hz
  if(millis() -_dspLastRefreshTime > 99)
  {
    _dspLastRefreshTime = millis();
    uint8_t enableLED = 0;
    if(brightness > 0)
    {
      enableLED = DSP_DIM_ON;
      brightness -= 1; 
    } 
    delayMicroseconds(30);
    digitalWrite(_CS_PIN, LOW); //start of packet
    _sendBitsToDSP(DSP_CMD1_MODE6_11_7, 8);
    digitalWrite(_CS_PIN, HIGH); //end of packet

    delayMicroseconds(50);
    digitalWrite(_CS_PIN, LOW);//start of packet
    _sendBitsToDSP(DSP_CMD2_DATAWRITE, 8);
    digitalWrite(_CS_PIN, HIGH);//end of packet

    //payload
    delayMicroseconds(50);
    digitalWrite(_CS_PIN, LOW);//start of packet
    for (int i = 0; i < 11; i++)
    _sendBitsToDSP(payload[i], 8);
    digitalWrite(_CS_PIN, HIGH);//end of packet

    delayMicroseconds(50);
    digitalWrite(_CS_PIN, LOW);//start of packet
    _sendBitsToDSP(DSP_DIM_BASE|enableLED|brightness, 8);
    digitalWrite(_CS_PIN, HIGH);//end of packet
    delayMicroseconds(50);
  }
}

void DSP::textOut(String txt) {
  int len = txt.length();
  //Set CMD3 (address 00H)
  payload[0] = 0xC0;
  if (len >= 3) {
    for (int i = 0; i < len - 2; i++) {
      payload[DGT1_IDX] = _getCode(txt.charAt(i));
      payload[DGT2_IDX] = _getCode(txt.charAt(i + 1));
      payload[DGT3_IDX] = _getCode(txt.charAt(i + 2));
      updateDSP(7);
      delay(230);
    }
  }
  else if (len == 2) {
    payload[DGT1_IDX] = _getCode(' ');
    payload[DGT2_IDX] = _getCode(txt.charAt(0));
    payload[DGT3_IDX] = _getCode(txt.charAt(1));
    updateDSP(7);
  }
  else if (len == 1) {
    payload[DGT1_IDX] = _getCode(' ');
    payload[DGT2_IDX] = _getCode(' ');
    payload[DGT3_IDX] = _getCode(txt.charAt(0));
    updateDSP(7);
  }
}

void DSP::LEDshow() {
  for(int y = 7; y < 11; y++){
    for(int x = 1; x < 9; x++){
      payload[y] = (1 << x) + 1;
      updateDSP(7);
      delay(200);
    }
  }
}

void DSP::begin(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int dsp_audio_pin) {
  _DATA_PIN = dsp_data_pin;
  _CLK_PIN = dsp_clk_pin;
  _CS_PIN = dsp_cs_pin;
  _AUDIO_PIN = dsp_audio_pin;

  pinMode(_CS_PIN, OUTPUT);
  pinMode(_DATA_PIN, INPUT);
  pinMode(_CLK_PIN, OUTPUT);
  pinMode(_AUDIO_PIN, OUTPUT);
  digitalWrite(_CS_PIN, HIGH);   //Active LOW
  digitalWrite(_CLK_PIN, HIGH);   //shift on falling, latch on rising
  digitalWrite(_AUDIO_PIN, LOW);
}

void DSP::playIntro() {
  int longnote = 125;
  int shortnote = 63;

  tone(_AUDIO_PIN, NOTE_C7, longnote);
  delay(2 * longnote);
  tone(_AUDIO_PIN, NOTE_G6, shortnote);
  delay(2 * shortnote);
  tone(_AUDIO_PIN, NOTE_G6, shortnote);
  delay(2 * shortnote);
  tone(_AUDIO_PIN, NOTE_A6, longnote);
  delay(2 * longnote);
  tone(_AUDIO_PIN, NOTE_G6, longnote);
  delay(2 * longnote);
  //paus
  delay(2 * longnote);
  tone(_AUDIO_PIN, NOTE_B6, longnote);
  delay(2 * longnote);
  tone(_AUDIO_PIN, NOTE_C7, longnote);
  delay(2 * longnote);
  noTone(_AUDIO_PIN);
}

//silent beep instead of annoying beeps every time something changes
void DSP::beep() {
  //int longnote = 125;
  // int shortnote = 63;
  // tone(_AUDIO_PIN, NOTE_C6, shortnote);
  // delay(shortnote);
  // tone(_AUDIO_PIN, NOTE_C7, shortnote);
  // delay(shortnote);
  // noTone(_AUDIO_PIN);
}

//new beep for button presses only
void DSP::beep2() {
  //int longnote = 125;
  int shortnote = 40;
  tone(_AUDIO_PIN, NOTE_D6, shortnote);
  delay(shortnote);
  tone(_AUDIO_PIN, NOTE_D7, shortnote);
  delay(shortnote);
  tone(_AUDIO_PIN, NOTE_D8, shortnote);
  delay(shortnote);
  noTone(_AUDIO_PIN);
}


