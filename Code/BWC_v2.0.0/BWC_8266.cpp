#include "BWC_8266.h"

CIO *pointerToClass;
//BWC *pointerToBWC;

//set flag instead of saving. This may avoid crashes. Some functions appears to crash when called from a timer.
// static void tick(void){
	// pointerToBWC->saveSettingsFlag();
// }

static void ICACHE_RAM_ATTR chipselectpin(void) {
  pointerToClass->packetHandler();
}
static void ICACHE_RAM_ATTR clockpin(void) {
  pointerToClass->clkHandler();
}

void CIO::begin(int cio_cs_pin, int cio_data_pin, int cio_clk_pin) {
  pointerToClass = this;
  _CS_PIN = cio_cs_pin;
  _DATA_PIN = cio_data_pin;
  _CLK_PIN = cio_clk_pin;  
  pinMode(_CS_PIN, INPUT);
  pinMode(_DATA_PIN, INPUT);
  pinMode(_CLK_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(_CS_PIN), chipselectpin, CHANGE);
  attachInterrupt(digitalPinToInterrupt(_CLK_PIN), clockpin, CHANGE); //Write on falling edge and read on rising edge
}

//match 7 segment pattern to a real digit
char CIO::_getChar(uint8_t value) {
  for (int index = 0; index < sizeof(CHARCODES); index++) {
    if (value == CHARCODES[index]) {
      return CHARS[index];
    }
  }
  return ' ';
}

void CIO::loop(void) {
	//newdata is true when a data packet has arrived from cio
	if(newData) {
		static int capturePhase = 0;
		static uint32_t buttonReleaseTime;
		static uint16_t prevButton = ButtonCodes[NOBTN];
		//determine if anything changed, so we can update webclients
		for(int i = 0; i < 11; i++){
			if (payload[i] != _prevPayload[i]) dataAvailable = true;
			_prevPayload[i] = payload[i];
		}
		
		newData = false;
		//copy private array to public array
		for(int i = 0; i < 11; i++){
			payload[i] = _payload[i];
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
		//Determine if display is showing target temp or actual temp or anything else.
		//capture TARGET after UP/DOWN has been pressed...
		if( ((button == ButtonCodes[UP]) || (button == ButtonCodes[DOWN])) && (prevButton != ButtonCodes[UP]) && (prevButton != ButtonCodes[DOWN]) ) capturePhase = 1;
		//...until 2 seconds after UP/DOWN released
		if( (button == ButtonCodes[UP]) || (button == ButtonCodes[DOWN]) ) buttonReleaseTime = millis();
		if(millis()-buttonReleaseTime > 2000) capturePhase = 0;
		//convert text on display to a value
		String tempstring = String((char)states[CHAR1])+String((char)states[CHAR2])+String((char)states[CHAR3]);
		uint8_t tmpTemp = tempstring.toInt();
		//capture only if showing plausible values (not blank screen while blinking)
		if( (capturePhase == 1) && (tmpTemp > 19) ) states[TARGET] = tmpTemp;
		//wait 4 seconds after UP/DOWN is released to be sure that actual temp is shown
		if( (capturePhase == 0) && (millis()-buttonReleaseTime > 10000)) states[TEMPERATURE] = tmpTemp;		
		prevButton = button;
	}
}

//end of packet
void ICACHE_RAM_ATTR CIO::eopHandler(void) {
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
void ICACHE_RAM_ATTR CIO::packetHandler(void) {
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
void ICACHE_RAM_ATTR CIO::clkHandler(void) {

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

char DSP::_getCode(char value) {
  for (int index = 0; index < sizeof(CHARS); index++) {
    if (value == CHARS[index]) {
      return CHARCODES[index];
    }
  }
  return 0x00;  //no match, return 'space'
}

void DSP::updateDSP(uint8_t brightness) {
	 //refresh display with ~10Hz
	if(millis() -_dspLastRefreshTime > 99){
		_dspLastRefreshTime = millis();
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
		  _sendBitsToDSP(DSP_DIM_BASE+DSP_DIM_ON+brightness, 8);
		  digitalWrite(_CS_PIN, HIGH);//end of packet
		  delayMicroseconds(50);
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

void DSP::begin(int dsp_cs_pin, int dsp_data_pin, int dsp_clk_pin, int dsp_audio_pin) {
	_CS_PIN = dsp_cs_pin;
	_DATA_PIN = dsp_data_pin;
	_CLK_PIN = dsp_clk_pin;
	_AUDIO_PIN = dsp_audio_pin;
	
	pinMode(_CS_PIN, OUTPUT);
	pinMode(_DATA_PIN, INPUT);
	pinMode(_CLK_PIN, OUTPUT);
	pinMode(_AUDIO_PIN, OUTPUT);
	digitalWrite(_CS_PIN, HIGH); 	//Active LOW
	digitalWrite(_CLK_PIN, HIGH); 	//shift on falling, latch on rising
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

void DSP::beep() {
  int longnote = 125;
  int shortnote = 63;
  tone(_AUDIO_PIN, NOTE_C6, shortnote);
  delay(shortnote);
  tone(_AUDIO_PIN, NOTE_C7, shortnote);
  delay(shortnote);
  noTone(_AUDIO_PIN);
}

BWC::BWC(){}

void BWC::begin(void){
	_cio.begin(D1, D7, D2);
	_dsp.begin(D3, D5, D4, D6);	
	begin2();
}

void BWC::begin(
			int cio_cs_pin, 
			int cio_data_pin, 
			int cio_clk_pin, 
			int dsp_cs_pin, 
			int dsp_data_pin, 
			int dsp_clk_pin, 
			int dsp_audio_pin 
			)
			{
	//pointerToBWC = this;
	//start CIO and DSP modules
	_cio.begin(cio_cs_pin, cio_data_pin, cio_clk_pin);
	_dsp.begin(dsp_cs_pin, dsp_data_pin, dsp_clk_pin, dsp_audio_pin);
	begin2();
}

void BWC::begin2(){
	//other stuff like intro music
	_dsp.textOut(F("   hello   "));
	if(_audio) _dsp.playIntro();
	_startNTP();
	LittleFS.begin();
	_loadSettings();
	_loadCommandQueue();
	_saveRebootInfo();
	saveSettingsTimer.attach(3600.0, std::bind(&BWC::saveSettingsFlag, this));	
}

void BWC::loop(){
  //feed the dog
  ESP.wdtFeed();
  ESP.wdtDisable();
  
	if (!DateTime.isTimeValid()) {
      //Serial.println("Failed to get time from server, retry.");
      DateTime.begin();
    } 
	_timestamp = DateTime.now();
  
  //update DSP payload (memcpy(dest*, source*, len))
  //memcpy(&_dsp.payload[0], &_cio.payload[0], 11);
  for(int i = 0; i < 11; i++){
	  _dsp.payload[i] = _cio.payload[i];
  }
  _dsp.updateDSP(7); //_cio.brightness);
  _updateTimes();
  //feed the dog
  //ESP.wdtFeed();
  //update cio public payload
  _cio.loop();
  //manage command queue
  _handleCommandQ();
  _handleButtonQ();//queue overrides real buttons
  if(_saveEventlogNeeded) saveEventlog();
  if(_saveCmdqNeeded) _saveCommandQueue();
  if(_saveSettingsNeeded) saveSettings();
  //if set target command overshot we need to correct that
  if( (_cio.states[TARGET] != _latestTarget) && (_qButtonLen == 0) && (_latestTarget != 0) ) qCommand(SETTARGET, _latestTarget, 0, 0);
  //if target temp is unknown, find out.
  if( (_cio.states[TARGET] == 0) && (_qButtonLen == 0) ) qCommand(GETTARGET, (uint32_t)' ', 0, 0);

  ESP.wdtEnable(0);
}

/* int BWC::_CodeToButton(uint16_t val){
	for(int i = 0; i < sizeof(ButtonCodes)/sizeof(uint16_t); i++){
		if(val == ButtonCodes[i]) return i;
	}
	return 0;
} */



void BWC::_qButton(uint32_t btn, uint32_t state, uint32_t value, uint32_t maxduration) {
	if(_qButtonLen == MAXBUTTONS) return;	//maybe textout an error message if queue is full?
	_buttonQ[_qButtonLen][0] = btn;
	_buttonQ[_qButtonLen][1] = state;
	_buttonQ[_qButtonLen][2] = value;
	_buttonQ[_qButtonLen][3] = maxduration + millis();
	_qButtonLen++;
}

void BWC::_handleButtonQ(void) {
	if(_qButtonLen > 0){
		//check if state is as desired, or duration is up. If so - remove row. Else set BTNCODE
		if( (_cio.states[_buttonQ[0][1]] == _buttonQ[0][2]) || (millis() > _buttonQ[0][3]) ){
			if(_buttonQ[0][0] == UP || _buttonQ[0][0] == DOWN) maxeffort = false;
			//remove row
			for(int i = 0; i < _qButtonLen-1; i++){
				_buttonQ[i][0] = _buttonQ[i+1][0];
				_buttonQ[i][1] = _buttonQ[i+1][1];
				_buttonQ[i][2] = _buttonQ[i+1][2];
				_buttonQ[i][3] = _buttonQ[i+1][3];
			}
			_qButtonLen--;	
			_cio.button = ButtonCodes[NOBTN];			
		} else {
			if(_buttonQ[0][0] == UP || _buttonQ[0][0] == DOWN) maxeffort = true;
			//set buttoncode
			_cio.button = ButtonCodes[_buttonQ[0][0]];
		}
	} else {
		//no queue so let dsp value through
		_cio.button = _dsp.getButton();
	}
	
}

bool BWC::qCommand(uint32_t cmd, uint32_t val, uint32_t xtime, uint32_t interval) {
	//handle special commands
	if(cmd == RESETQ){
		_qButtonLen = 0;
		_qCommandLen = 0;
		_saveCommandQueue();
		return true;
	} 

	//add parameters to _commandQ[rows][parameter columns] and sort the array on xtime.
	int row = _qCommandLen;
	if (_qCommandLen == MAXCOMMANDS) return false;
	//sort array on xtime
	for (int i = 0; i < _qCommandLen; i++) {
		if (xtime < _commandQ[i][2]){
			//insert row at [i]
			row = i;
			break;
		}
	}	
	//make room for new row
	for (int i = _qCommandLen; i > (row); i--){
		_commandQ[i][0] = _commandQ[i-1][0];
		_commandQ[i][1] = _commandQ[i-1][1];
		_commandQ[i][2] = _commandQ[i-1][2];
		_commandQ[i][3] = _commandQ[i-1][3];
	}
	//add new command
	_commandQ[row][0] = cmd;
	_commandQ[row][1] = val;
	_commandQ[row][2] = xtime;
	_commandQ[row][3] = interval;
	_qCommandLen++;
	delay(0);
	_saveCommandQueue();
	return true;
}

void BWC::_handleCommandQ(void) {
	bool restartESP = false;
	if(_qCommandLen > 0) { 
	//cmp time with xtime. If more, then execute (adding buttons to buttonQ).
	
		if (_timestamp >= _commandQ[0][2]){
			_qButton(POWER, POWERSTATE, 1, 5000); //press POWER button until states[POWERSTATE] is 1, max 5000 ms
			_qButton(LOCK, LOCKEDSTATE, 0, 5000); //press LOCK button until states[LOCKEDSTATE] is 0
			switch (_commandQ[0][0]) {
				case SETTARGET:
					_latestTarget = _commandQ[0][1];
					//choose which direction to go (up or down)
					if(_cio.states[TARGET] == 0 )
					{
						_qButton(UP, TARGET, _commandQ[0][1], 10000);
						_qButton(DOWN, TARGET, _commandQ[0][1], 10000);
					}
					if(_cio.states[TARGET] > _commandQ[0][1]) _qButton(DOWN, TARGET, _commandQ[0][1], 10000);
					if(_cio.states[TARGET] < _commandQ[0][1]) _qButton(UP, TARGET, _commandQ[0][1], 10000);
					break;
				case SETUNIT:
					_qButton(UNIT, UNITSTATE, _commandQ[0][1], 5000);
					break;
				case SETBUBBLES:
					_qButton(BUBBLES, BUBBLESSTATE, _commandQ[0][1], 5000);
					break;
				case SETHEATER:
					_qButton(HEAT, HEATSTATE, _commandQ[0][1], 5000);
					break;
				case SETPUMP:
					_qButton(PUMP, PUMPSTATE, _commandQ[0][1], 5000);
					break;
				case REBOOTESP:				
					restartESP = true;
					break;
				case GETTARGET:
					_qButton(UP, CHAR3, _commandQ[0][1], 700);
					break;
				case RESETTIMES:
					_uptime = 0;
					_pumptime = 0;
					_heatingtime = 0;
					_airtime = 0;
					_uptime_ms = 0;
					_pumptime_ms = 0;
					_heatingtime_ms = 0;
					_airtime_ms = 0;
					_cost = 0;
					_saveSettingsNeeded = true;		
					break;
				case RESETCLTIMER:
					_cltime = _timestamp;
					_saveSettingsNeeded = true;
					break;
				case RESETFTIMER:
					_ftime = _timestamp;
					_saveSettingsNeeded = true;
					break;
			}
			//If interval > 0 then append to commandQ with updated xtime.
			if(_commandQ[0][3] > 0) qCommand(_commandQ[0][0],_commandQ[0][1],_commandQ[0][2]+_commandQ[0][3],_commandQ[0][3]);
			//remove from commandQ and decrease qCommandLen
			for(int i = 0; i < _qCommandLen-1; i++){
			_commandQ[i][0] = _commandQ[i+1][0];
			_commandQ[i][1] = _commandQ[i+1][1];
			_commandQ[i][2] = _commandQ[i+1][2];
			_commandQ[i][3] = _commandQ[i+1][3];
			}
			_qCommandLen--;
			_saveCommandQueue();
			if(restartESP) {
				saveSettings();			
				ESP.restart();
			}
		}
	}
}
	

String BWC::getJSONStates() {
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
  //feed the dog
  ESP.wdtFeed();
    DynamicJsonDocument doc(1024);

    // Set the values in the document
    doc["CONTENT"] = "STATES";
    doc["LCK"] = _cio.states[LOCKEDSTATE];
    doc["PWR"] = _cio.states[POWERSTATE];
    doc["UNT"] = _cio.states[UNITSTATE];
    doc["AIR"] = _cio.states[BUBBLESSTATE];
    doc["GRN"] = _cio.states[HEATGRNSTATE];
    doc["RED"] = _cio.states[HEATREDSTATE];
    doc["FLT"] = _cio.states[PUMPSTATE];
    doc["TGT"] = _cio.states[TARGET];
    doc["TMP"] = _cio.states[TEMPERATURE];
    doc["CH1"] = _cio.states[CHAR1];
    doc["CH2"] = _cio.states[CHAR2];
    doc["CH3"] = _cio.states[CHAR3];

    // Serialize JSON to string
    String jsonmsg;
    if (serializeJson(doc, jsonmsg) == 0) {
      jsonmsg = "{\"error\": \"Failed to serialize message\"}";
	}
	return jsonmsg;
}

String BWC::getJSONTimes() {
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
  //feed the dog
  ESP.wdtFeed();
    DynamicJsonDocument doc(1024);

    // Set the values in the document
    doc["CONTENT"] = "TIMES";
    doc["TIME"] = _timestamp;
    doc["CLTIME"] = _cltime;
    doc["FTIME"] = _ftime;
    doc["UPTIME"] = _uptime + _uptime_ms/1000;
    doc["PUMPTIME"] = _pumptime + _pumptime_ms/1000;
    doc["HEATINGTIME"] = _heatingtime + _heatingtime_ms/1000;
    doc["AIRTIME"] = _airtime + _airtime_ms/1000;
    doc["COST"] = _cost;
    doc["FINT"] = _fint;
    doc["CLINT"] = _clint;

    // Serialize JSON to string
    String jsonmsg;
    if (serializeJson(doc, jsonmsg) == 0) {
      jsonmsg = "{\"error\": \"Failed to serialize message\"}";
	}
	return jsonmsg;
}

String BWC::getJSONSettings(){
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
  //feed the dog
  ESP.wdtFeed();
    DynamicJsonDocument doc(1024);

    // Set the values in the document
    doc["CONTENT"] = "SETTINGS";
    doc["TIMEZONE"] = _timezone;
    doc["PRICE"] = _price;
    doc["FINTERVAL"] = _finterval;
    doc["CLINTERVAL"] = _clinterval;
    doc["AUDIO"] = _audio;
    doc["REBOOTINFO"] = ESP.getResetReason();
    doc["REBOOTTIME"] = DateTime.getBootTime();
	
    // Serialize JSON to string
    String jsonmsg;
    if (serializeJson(doc, jsonmsg) == 0) {
      jsonmsg = "{\"error\": \"Failed to serialize message\"}";
	}
	return jsonmsg;	
}

void BWC::setJSONSettings(String message){
  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  //feed the dog
  ESP.wdtFeed();
  DynamicJsonDocument doc(1024);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.println(F("Failed to read config file"));
    return;
  }

  // Copy values from the JsonDocument to the variables
  _timezone = doc["TIMEZONE"];
  _price = doc["PRICE"];
  _finterval = doc["FINTERVAL"];
  _clinterval = doc["CLINTERVAL"];
  _audio = doc["AUDIO"];
  saveSettings();	
}

String BWC::getJSONCommandQueue(){
  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  //feed the dog
  ESP.wdtFeed();
  DynamicJsonDocument doc(1024);
  // Set the values in the document
  doc["LEN"] = _qCommandLen;
  for(int i = 0; i < _qCommandLen; i++){
	  doc["CMD"][i] = _commandQ[i][0];
	  doc["VALUE"][i] = _commandQ[i][1];
	  doc["XTIME"][i] = _commandQ[i][2];
	  doc["INTERVAL"][i] = _commandQ[i][3];
  }

  // Serialize JSON to file
  String jsonmsg;
  if (serializeJson(doc, jsonmsg) == 0) {
    jsonmsg = "{\"error\": \"Failed to serialize message\"}";
  }
  return jsonmsg;
}

bool BWC::newData(){
	bool result = _cio.dataAvailable;
	_cio.dataAvailable = false;
	if (result && _audio) _dsp.beep();
	if(maxeffort) return false; else return result;
}
	
void BWC::_startNTP() {
  // setup this after wifi connected
  // you can use custom timezone,server and timeout
  // DateTime.setTimeZone(-4);
  DateTime.setTimeZone(_timezone);
  DateTime.setServer("pool.ntp.org");
  //   DateTime.begin(15 * 1000);
  DateTime.begin();
  DateTime.begin();
//  delay(5000);
  int c = 0;
  while (!DateTime.isTimeValid()) {
    Serial.println(F("Failed to get time from server. Trying again."));
    delay(1000);
    //DateTime.setServer("time.cloudflare.com");
    //DateTime.setTimeZone(_timezone);
    DateTime.begin();
    if (c++ > 5) break;
  }
  Serial.println(DateTime.format(DateFormatter::SIMPLE));
}	

void BWC::_loadSettings(){
  File file = LittleFS.open("settings.txt", "r");
  if (!file) {
    Serial.println(F("Failed to load settings.txt"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  DynamicJsonDocument doc(1024);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to read settings.txt"));
    file.close();
    return;
  }

  // Copy values from the JsonDocument to the variables
  _cltime = doc["CLTIME"];
  _ftime = doc["FTIME"];
  _uptime = doc["UPTIME"];
  _pumptime = doc["PUMPTIME"];
  _heatingtime = doc["HEATINGTIME"];
  _airtime = doc["AIRTIME"];
  _timezone = doc["TIMEZONE"];
  _price = doc["PRICE"];
  _finterval = doc["FINTERVAL"];
  _clinterval = doc["CLINTERVAL"];
  _audio = doc["AUDIO"];
  file.close();
}

void BWC::saveSettingsFlag(){
	//ticker fails if duration is more than 71 min. So we use a counter every 60 minutes
	if(++_tickerCount >= 3){
		_saveSettingsNeeded = true;	
		_tickerCount = 0;
	}
}

void BWC::saveSettings(){
  if(maxeffort) {
	  _saveSettingsNeeded = true;
	  return;
  }
  //kill the dog
  ESP.wdtDisable();
  _saveSettingsNeeded = false;
  File file = LittleFS.open("settings.txt", "w");
  if (!file) {
    Serial.println(F("Failed to save settings.txt"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(1024);
	_heatingtime += _heatingtime_ms/1000;
	_pumptime += _pumptime_ms/1000;
	_airtime += _airtime_ms/1000;
	_uptime += _uptime_ms/1000;
	_heatingtime_ms = 0;
	_pumptime_ms = 0;
	_airtime_ms = 0;
	_uptime_ms = 0;
  // Set the values in the document
  doc["CLTIME"] = _cltime;
  doc["FTIME"] = _ftime;
  doc["UPTIME"] = _uptime;
  doc["PUMPTIME"] = _pumptime;
  doc["HEATINGTIME"] = _heatingtime;
  doc["AIRTIME"] = _airtime;
  doc["TIMEZONE"] = _timezone;
  doc["PRICE"] = _price;
  doc["FINTERVAL"] = _finterval;
  doc["CLINTERVAL"] = _clinterval;
  doc["AUDIO"] = _audio;
  doc["SAVETIME"] = DateTime.format(DateFormatter::SIMPLE);

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write json to settings.txt"));
  }
  file.close();
  //update clock
  DateTime.setTimeZone(_timezone);
  DateTime.begin();	
  //revive the dog
  ESP.wdtEnable(0);

}

void BWC::_loadCommandQueue(){
  File file = LittleFS.open("cmdq.txt", "r");
  if (!file) {
    Serial.println(F("Failed to read cmdq.txt"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(1024);
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to deserialize cmdq.txt"));
    file.close();
    return;
  }

  // Set the values in the variables
  _qCommandLen = doc["LEN"];
  for(int i = 0; i < _qCommandLen; i++){
	  _commandQ[i][0] = doc["CMD"][i];
	  _commandQ[i][1] = doc["VALUE"][i];
	  _commandQ[i][2] = doc["XTIME"][i];
	  _commandQ[i][3] = doc["INTERVAL"][i];
  }

  file.close();		
}

void BWC::_saveCommandQueue(){
  if(maxeffort) {
	  _saveCmdqNeeded = true;
	  return;
  }
  //kill the dog
  ESP.wdtDisable();
  
  _saveCmdqNeeded = false;
  File file = LittleFS.open("cmdq.txt", "w");
  if (!file) {
    Serial.println(F("Failed to save cmdq.txt"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(1024);

  // Set the values in the document
  doc["LEN"] = _qCommandLen;
  for(int i = 0; i < _qCommandLen; i++){
	  doc["CMD"][i] = _commandQ[i][0];
	  doc["VALUE"][i] = _commandQ[i][1];
	  doc["XTIME"][i] = _commandQ[i][2];
	  doc["INTERVAL"][i] = _commandQ[i][3];
  }

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write cmdq.txt"));
  }
  file.close();	
  //revive the dog
  ESP.wdtEnable(0);

}

void BWC::saveEventlog(){
  if(maxeffort) {
	  _saveEventlogNeeded = true;
	  return;
  }
  _saveEventlogNeeded = false;
  //kill the dog
  ESP.wdtDisable();
  File file = LittleFS.open("eventlog.txt", "a");
  if (!file) {
    Serial.println(F("Failed to save eventlog.txt"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(1024);

  // Set the values in the document
  for(int i = 0; i < sizeof(_cio.states); i++){
	doc[i] = _cio.states[i];
  }
  doc["timestamp"] = DateTime.format(DateFormatter::SIMPLE);

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write eventlog.txt"));
  }
  file.close();		
  //revive the dog
  ESP.wdtEnable(0);

}

void BWC::_saveRebootInfo(){
  File file = LittleFS.open("bootlog.txt", "a");
  if (!file) {
    Serial.println(F("Failed to save bootlog.txt"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(1024);

  // Set the values in the document
  doc["BOOTINFO"] = ESP.getResetReason() + " " + DateTime.format(DateFormatter::SIMPLE);

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write bootlog.txt"));
  }
  file.println();
  file.close();	
}

void BWC::_updateTimes(){
	uint32_t now = millis();
	static uint32_t prevtime;
	int elapsedtime = now-prevtime;
	prevtime = now;
	if (elapsedtime < 0) return; //millis() rollover every 49 days
	if(_cio.states[HEATREDSTATE]){
		_heatingtime_ms += elapsedtime;
	}
	if(_cio.states[PUMPSTATE]){
		_pumptime_ms += elapsedtime;
	}
	if(_cio.states[BUBBLESSTATE]){
		_airtime_ms += elapsedtime;
	}
	_uptime_ms += elapsedtime;
	
	if(_uptime_ms > 1000000000){
		_heatingtime += _heatingtime_ms/1000;
		_pumptime += _pumptime_ms/1000;
		_airtime += _airtime_ms/1000;
		_uptime += _uptime_ms/1000;
		_heatingtime_ms = 0;
		_pumptime_ms = 0;
		_airtime_ms = 0;
		_uptime_ms = 0;
	}
	
	_cost = _price*((_heatingtime+_heatingtime_ms/1000)*1900+(_pumptime+_pumptime_ms/1000)*40+(_airtime+_airtime_ms/1000)*800+(_uptime+_uptime_ms/1000)*2)/3600000.0;
	_clint = _clinterval;
	_fint = _finterval;
}

void BWC::print(String txt){
	_dsp.textOut(txt);
}

uint8_t BWC::getState(int state){
	return _cio.states[state];
}
