#include "BWC_6w_type2.h"
#include "pitches.h"

CIO *pointerToClass;

static void IRAM_ATTR LEDdatapin(void) {
  pointerToClass->packetHandler();
}
static void IRAM_ATTR clockpin(void) {
  pointerToClass->clkHandler();
}

void CIO::begin(int cio_td_pin, int cio_clk_pin, int cio_ld_pin) {
  pointerToClass = this;
  _CIO_TD_PIN = cio_td_pin;
  _CIO_CLK_PIN = cio_clk_pin;
  _CIO_LD_PIN = cio_ld_pin;
  pinMode(_CIO_LD_PIN, INPUT);
  pinMode(_CIO_TD_PIN, OUTPUT);
  pinMode(_CIO_CLK_PIN, INPUT);
  digitalWrite(_CIO_TD_PIN, 1); //idle high
  attachInterrupt(digitalPinToInterrupt(_CIO_LD_PIN), LEDdatapin, CHANGE);
  attachInterrupt(digitalPinToInterrupt(_CIO_CLK_PIN), clockpin, CHANGE); //Write on falling edge and read on rising edge
}

void CIO::stop(){
  detachInterrupt(digitalPinToInterrupt(_CIO_LD_PIN));
  detachInterrupt(digitalPinToInterrupt(_CIO_CLK_PIN));
}


void CIO::loop(void) {
  //newdata is true when a data packet has arrived from cio
  if(!newData) return;
  newData = false;
  static uint32_t buttonReleaseTime;
  enum Readmode: int {readtemperature, uncertain, readtarget};
  static int capturePhase = readtemperature;

  //capture TARGET after UP/DOWN has been pressed...
  if ((button == ButtonCodes[UP]) || (button == ButtonCodes[DOWN]))
  {
    buttonReleaseTime = millis(); //updated as long as buttons are pressed
    capturePhase = readtarget;
  } 

  /*
    * This model is only sending messages when something updated
    * so this section is not useful
    */
  //require two consecutive messages to be equal before registering

  //copy private array to public array
  for(unsigned int i = 0; i < sizeof(payload); i++){
    payload[i] = _payload[i];
  }

  //determine if anything changed, so we can update webclients
  for(unsigned int i = 0; i < sizeof(payload); i++){
    if (payload[i] != _prevPayload[i]) dataAvailable = true;
    _prevPayload[i] = payload[i];
  }

  //brightness = _brightness & 7; //extract only the brightness bits (0-7)
  //extract information from payload to a better format
  states[LOCKEDSTATE] = (payload[LCK_IDX] & (1 << LCK_BIT)) > 0;
  states[POWERSTATE] = 1;  //(payload[PWR_IDX] & (1 << PWR_BIT)) > 0;
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
  //Unreadable characters - exit
  if(states[CHAR1] == '*' || states[CHAR2] == '*' || states[CHAR3] == '*') return;
  //Error or user plays with timer button - exit (error notification can be dealt with in main.cpp or elsewhere)
  if(states[CHAR1] == 'E' || states[CHAR3] == 'H' || states[CHAR3] == ' ') return;
  
  //Stop expecting target temp after timeout
  if((millis()-buttonReleaseTime) > 2000) capturePhase = uncertain;
  if((millis()-buttonReleaseTime) > 6000) capturePhase = readtemperature;
  //convert text on display to a value if the chars are recognized
  String tempstring = String((char)states[CHAR1])+String((char)states[CHAR2])+String((char)states[CHAR3]);
  uint8_t parsedValue = tempstring.toInt();
  //capture target temperature only if showing plausible values (not blank screen while blinking)
  if( (capturePhase == readtarget) && (parsedValue > 19) ) {
    states[TARGET] = parsedValue;
  }
  //wait 6 seconds after UP/DOWN is released to be sure that actual temp is shown
  if(capturePhase == readtemperature)
  {
    if(states[TEMPERATURE] != parsedValue) dataAvailable = true;
    states[TEMPERATURE] = parsedValue;
  }

  //If any of these states changes, we need to set a flag to save states. Used to restore them after reboot.
  if(states[UNITSTATE] != _prevUNT || states[HEATSTATE] != _prevHTR || states[PUMPSTATE] != _prevFLT) {
    stateChanged = true;
    _prevUNT = states[UNITSTATE];
    _prevHTR = states[HEATSTATE];
    _prevFLT = states[PUMPSTATE];
  }
}

}

//CIO comm
//packet start/stop
void IRAM_ATTR CIO::packetHandler(void) {
  //Check START/END condition: _LD_PIN change when _CLK_PIN is high.
  if (READ_PERI_REG(PIN_IN) & (1 << _CIO_CLK_PIN)) {
    _byteCount = 0;
    _bitCount = 0;
    _received_cmd = 0;
    newData = READ_PERI_REG(PIN_IN) & (1 << _CIO_LD_PIN);
  }
}

void IRAM_ATTR CIO::clkHandler(void) {
  //read data on _cio_ld_pin and write to _cio_td_pin (LSBF)

  uint16_t td_bitnumber = _bitCount % 10;
  uint16_t ld_bitnumber = _bitCount % 8;
  uint16_t buttonwrapper = (B11111110 << 8) | (button<<1); //startbit @ bit0, stopbit @ bit9

  //rising or falling edge?
  bool risingedge = READ_PERI_REG(PIN_IN) & (1 << _CIO_CLK_PIN);
  if(risingedge){
    //clk rising edge
    _byteCount = _bitCount / 8;
    if(_byteCount == 0){
      _received_cmd |= ((READ_PERI_REG(PIN_IN) & (1 << _CIO_LD_PIN))>0) << ld_bitnumber;
    }
    else if( (_byteCount<6) && (_received_cmd == CMD2) ){ //only write to payload after CMD2. Also protect from buffer overflow
      //overwrite the old payload bit with new bit
      _payload[_byteCount-1] = (_payload[_byteCount-1] & ~(1 << ld_bitnumber)) | ((READ_PERI_REG(PIN_IN) & (1 << _CIO_LD_PIN))>0) << ld_bitnumber;
    }
    //store brightness in _cio local variable. It is not used, but put here in case we want to obey the pump.
    if(_bitCount == 7 && (_received_cmd & B11000000) == B10000000) _brightness = _received_cmd;
    _bitCount++;
  } else {
    //clk falling edge
    //first and last bit is a dummy start/stop bit (0/1), then 8 data bits in btwn
    if (buttonwrapper & (1 << td_bitnumber)) {
      WRITE_PERI_REG( PIN_OUT_SET, 1 << _CIO_TD_PIN);
    } else {
      WRITE_PERI_REG( PIN_OUT_CLEAR, 1 << _CIO_TD_PIN);
    }
  }
}

uint16_t DSP::getButton(void) {
  if(millis() - _dspLastGetButton > 20){
    uint16_t newButton = 0;
    _dspLastGetButton = millis();
    //startbit
    digitalWrite(_DSP_CLK_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    //clock in 8 data bits
    for(int i = 0; i < 8; i++){
      digitalWrite(_DSP_CLK_PIN, LOW);
      delayMicroseconds(CLKPW);
      digitalWrite(_DSP_CLK_PIN, HIGH);
      newButton |= digitalRead(_DSP_TD_PIN)<<i;
      delayMicroseconds(CLKPW);
    }
    //stop bit
    digitalWrite(_DSP_CLK_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    //work around for glitches. Only register change after two consecutive and equal values
    if(newButton == _prevButton){
      _oldButton = newButton;
    } else {
      _prevButton = newButton;
    }
  }
  return (_oldButton);
}


//bitsToSend can only be 8 with this solution of LSB first
void DSP::_sendBitsToDSP(uint32_t outBits, int bitsToSend) {
  for (int i = 0; i < bitsToSend; i++) {
    digitalWrite(_DSP_CLK_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(_DSP_LD_PIN, outBits & (1 << i));
    delayMicroseconds(CLKPW-5);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
  }
}

uint16_t DSP::_receiveBitsFromDSP() {
  return 0;
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
    digitalWrite(_DSP_LD_PIN, LOW); //start of packet
    delayMicroseconds(CLKPW);
    _sendBitsToDSP(CMD1, 8);
    //end of packet: clock low, make sure LD is low before rising clock then LD
    digitalWrite(_DSP_CLK_PIN, LOW);
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_LD_PIN, HIGH);
    delayMicroseconds(CLKPW);

    digitalWrite(_DSP_LD_PIN, LOW); //start of packet
    delayMicroseconds(CLKPW);
    _sendBitsToDSP(CMD2, 8);
    for(unsigned int i=0; i<sizeof(payload); i++){
      _sendBitsToDSP(payload[i], 8);
    }
    //end of packet: clock low, make sure LD is low before rising clock then LD
    digitalWrite(_DSP_CLK_PIN, LOW);
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_LD_PIN, HIGH);
    delayMicroseconds(CLKPW);
    
    digitalWrite(_DSP_LD_PIN, LOW); //start of packet
    delayMicroseconds(CLKPW);
    _sendBitsToDSP((CMD3 & 0xF8)|enableLED|brightness, 8);

    //end of packet: clock low, make sure LD is low before raising clock then LD
    digitalWrite(_DSP_CLK_PIN, LOW);
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_LD_PIN, HIGH);
    delayMicroseconds(CLKPW);
  }
}

void DSP::textOut(String txt) {
  int len = txt.length();
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
  //todo: clear payload first...
  for(unsigned int y = 0; y < sizeof(payload); y++){
    for(int x = 0; x < 9; x++){
      payload[y] = (1 << x);
      updateDSP(7);
      delay(200);
    }
  }
}

void DSP::begin(int dsp_td_pin, int dsp_clk_pin, int dsp_ld_pin, int dsp_audio_pin) {
  _DSP_TD_PIN = dsp_td_pin;
  _DSP_CLK_PIN = dsp_clk_pin;
  _DSP_LD_PIN = dsp_ld_pin;
  _DSP_AUDIO_PIN = dsp_audio_pin;

  pinMode(_DSP_LD_PIN, OUTPUT);
  pinMode(_DSP_TD_PIN, INPUT);
  pinMode(_DSP_CLK_PIN, OUTPUT);
  pinMode(_DSP_AUDIO_PIN, OUTPUT);
  digitalWrite(_DSP_LD_PIN, HIGH);   //idle high
  digitalWrite(_DSP_CLK_PIN, HIGH); //shift on falling, latch on rising
  digitalWrite(_DSP_AUDIO_PIN, LOW);
}

void DSP::playIntro() {
  int longnote = 125;
  int shortnote = 63;

  tone(_DSP_AUDIO_PIN, NOTE_C7, longnote);
  delay(2 * longnote);
  tone(_DSP_AUDIO_PIN, NOTE_G6, shortnote);
  delay(2 * shortnote);
  tone(_DSP_AUDIO_PIN, NOTE_G6, shortnote);
  delay(2 * shortnote);
  tone(_DSP_AUDIO_PIN, NOTE_A6, longnote);
  delay(2 * longnote);
  tone(_DSP_AUDIO_PIN, NOTE_G6, longnote);
  delay(2 * longnote);
  //paus
  delay(2 * longnote);
  tone(_DSP_AUDIO_PIN, NOTE_B6, longnote);
  delay(2 * longnote);
  tone(_DSP_AUDIO_PIN, NOTE_C7, longnote);
  delay(2 * longnote);
  noTone(_DSP_AUDIO_PIN);
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
  tone(_DSP_AUDIO_PIN, NOTE_D6, shortnote);
  delay(shortnote);
  tone(_DSP_AUDIO_PIN, NOTE_D7, shortnote);
  delay(shortnote);
  tone(_DSP_AUDIO_PIN, NOTE_D8, shortnote);
  delay(shortnote);
  noTone(_DSP_AUDIO_PIN);
}


