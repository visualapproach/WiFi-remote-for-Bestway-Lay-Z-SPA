#include "BWC_8266_4w.h"

void CIO::begin() {
  //Setup serial to CIO here
  //Using hardware UART/Serial on default pins (RX/TX)
  cio_serial.begin(9600, SWSERIAL_8N1, D6, D7, false, 64);
  cio_serial.setTimeout(50);
  dsp_serial.begin(9600, SWSERIAL_8N1, D1, D2, false, 64);
  dsp_serial.setTimeout(50);

  states[TARGET] = 20;
  //Not used. Here for compatibility reasons
  states[LOCKEDSTATE] = false;
  states[POWERSTATE] = true;
  states[UNITSTATE] = true;   //Celsius
  states[CHAR1] = ' ';
  states[CHAR2] = ' ';
  states[CHAR3] = ' ';
  cio_tx = false;
  dsp_tx = false;
}

void CIO::loop(void) {
  //check if CIO has sent a message
  int msglen = 0;
  if(cio_serial.available()){
    msglen = cio_serial.readBytes(from_CIO_buf, PAYLOADSIZE);
  }
  //pass message to display
  if(msglen == PAYLOADSIZE){
    for(int i = 0; i < PAYLOADSIZE; i++){
      if(to_DSP_buf[i] != from_CIO_buf[i]) dataAvailable = true;
      to_DSP_buf[i] = from_CIO_buf[i];
    }
    states[TEMPERATURE] = from_CIO_buf[TEMPINDEX];
    states[ERROR] =       from_CIO_buf[ERRORINDEX];
    //do stuff here if you want to alter the message
    dsp_serial.write(to_DSP_buf, PAYLOADSIZE);
    digitalWrite(D4, !digitalRead(D4));  //blink  
    cio_tx = true;  //show the user that this line works (appears to work)
  }
  //check if cio send error msg
  states[CHAR1] = ' ';
  states[CHAR2] = ' ';
  states[CHAR3] = ' ';
  if(states[ERROR]){
    to_CIO_buf[COMMANDINDEX] = 0; //clear any commands
    GODMODE = false;
    states[CHAR1] = 'E';
    states[CHAR2] = 48+(to_CIO_buf[ERRORINDEX]/10);
    states[CHAR3] = 48+(to_CIO_buf[ERRORINDEX]%10);
  }
  //check if display sent a message
  msglen = 0;
  if(dsp_serial.available()){
    msglen = dsp_serial.readBytes(from_DSP_buf, PAYLOADSIZE);

  }
  //pass message to CIO
  if(msglen == PAYLOADSIZE){
    for(int i = 0; i < PAYLOADSIZE; i++){
      to_CIO_buf[i] = from_DSP_buf[i];
    }
    //Do stuff here to command the CIO
    if(GODMODE){
      updatePayload();
    } else {
      updateStates();
    }
    cio_serial.write(to_CIO_buf, PAYLOADSIZE);
    dsp_tx = true;  //show the user that this line works (appears to work)
  }    
}

void CIO::updateStates(){
  static uint8_t prevchksum;
  //extract information from payload to a better format
  states[BUBBLESSTATE] = (from_DSP_buf[COMMANDINDEX] & BUBBLESBITMASK) > 0;
  states[HEATGRNSTATE] = 2;                           //unknowable
  states[HEATREDSTATE] = (from_DSP_buf[COMMANDINDEX] & HEATBITMASK) > 0;  //
  states[HEATSTATE] = 2;                              //unknowable
  states[PUMPSTATE] = (from_DSP_buf[COMMANDINDEX] & PUMPBITMASK) > 0;
  states[JETSSTATE] = (from_DSP_buf[COMMANDINDEX] & JETSBITMASK) > 0;
  if(from_DSP_buf[DSP_CHECKSUMINDEX] != prevchksum) dataAvailable = true;
  prevchksum = from_DSP_buf[DSP_CHECKSUMINDEX];
}

void CIO::updatePayload(){
  //alter payload to CIO to reflect user setting (GODMODE)
  //this is a simple thermostat with no inertia. Add average temperature to compare with *************
  static uint8_t prevchksum;
  if(states[HEATSTATE] && states[TEMPERATURE] < states[TARGET]){
    states[HEATREDSTATE] = true;
  } else {
    states[HEATREDSTATE] = false;
  }

  //antifreeze
  if(states[TEMPERATURE] < 10) states[HEATREDSTATE] = true;

  states[HEATGRNSTATE] = !states[HEATREDSTATE] && states[HEATSTATE];
  to_CIO_buf[COMMANDINDEX] =  (states[HEATREDSTATE] * HEATBITMASK)    |  
                              (states[JETSSTATE] * JETSBITMASK)       | 
                              (states[PUMPSTATE] * PUMPBITMASK)       |
                              (states[BUBBLESSTATE] * BUBBLESBITMASK) | 
                              (states[PUMPSTATE] * PUMPBITMASK);
  if(to_CIO_buf[COMMANDINDEX] > 0) to_CIO_buf[COMMANDINDEX] |= POWERBITMASK;

  //calc checksum -> byte5
  //THIS NEEDS TO BE IMPROVED IF OTHER CHECKSUMS IS USED (FOR OTHER BYTES in different models)
  to_CIO_buf[DSP_CHECKSUMINDEX] = to_CIO_buf[1] + to_CIO_buf[2] + to_CIO_buf[3] + to_CIO_buf[4];
  if(to_CIO_buf[DSP_CHECKSUMINDEX] != prevchksum) dataAvailable = true;
  prevchksum = to_CIO_buf[DSP_CHECKSUMINDEX];
}


void BWC::begin(void){
	_cio.begin();
	_startNTP();
	LittleFS.begin();
	_loadSettings();
	_loadCommandQueue();
	_saveRebootInfo();
	saveSettingsTimer.attach(3600.0, std::bind(&BWC::saveSettingsFlag, this));
  cio_tx = false;
  dsp_tx = false;	
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
  
   _updateTimes();
  //feed the dog
  //ESP.wdtFeed();
  //update cio public payload
  _cio.loop();
  //manage command queue
  _handleCommandQ();
  if(_saveEventlogNeeded) saveEventlog();
  if(_saveCmdqNeeded) _saveCommandQueue();
  if(_saveSettingsNeeded) saveSettings();
  ESP.wdtEnable(0);
  cio_tx = _cio.cio_tx;
  dsp_tx = _cio.dsp_tx;
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
			switch (_commandQ[0][0]) {
				case SETTARGET:
					_cio.states[TARGET] = _commandQ[0][1];
          if(_cio.states[TARGET] > 40) _cio.states[TARGET] = 40;  //don't cook anyone
					break;
				case SETUNIT:
          _cio.states[UNITSTATE] = _commandQ[0][1];
					break;
				case SETBUBBLES:
					_cio.states[BUBBLESSTATE] = _commandQ[0][1];
          _cio.states[PUMPSTATE] = false;
          _cio.states[HEATSTATE] = false;
					break;
				case SETHEATER:
					_cio.states[HEATSTATE] = _commandQ[0][1];
          if(_commandQ[0][1]) {
            _cio.states[PUMPSTATE] = true;
            _cio.states[BUBBLESSTATE] = false;
            _cio.states[JETSSTATE] = false;
          } 
					break;
				case SETPUMP:
					
          //let pump run a bit to cool element
          if(_cio.states[HEATSTATE] && !_commandQ[0][1]) {
            qCommand(SETHEATER, 0, 0, 0);
            qCommand(SETPUMP, 0, _timestamp + 10, 0);
          } else {
            _cio.states[PUMPSTATE] = _commandQ[0][1];
          }
					break;
				case REBOOTESP:				
					restartESP = true;
					break;
				case GETTARGET:
					
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
        case SETJETS:
          _cio.states[JETSSTATE] = _commandQ[0][1];
          if(_commandQ[0][1]) {
            _cio.states[PUMPSTATE] = false;
            _cio.states[HEATSTATE] = false;
          } 
          break;
        case SETGODMODE:
          _cio.GODMODE = _commandQ[0][1];
          _cio.states[HEATGRNSTATE] = 0;
          _cio.states[HEATREDSTATE] = 0;
          _cio.states[HEATSTATE] = 0;
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
    doc["JET"] = _cio.states[JETSSTATE];
    doc["GOD"] = _cio.GODMODE;

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
    doc["FINT"] = _finterval;
    doc["CLINT"] = _clinterval;

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
    doc["FINT"] = _finterval;
    doc["CLINT"] = _clinterval;
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
  _finterval = doc["FINT"];
  _clinterval = doc["CLINT"];
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
	return result;
}
	
void BWC::_startNTP() {
  // setup this after wifi connected
  // you can use custom timezone,server and timeout
  // DateTime.setTimeZone(-4);
  //DateTime.setTimeZone(_timezone); //deprecated
  DateTime.setServer("pool.ntp.org");
  //   DateTime.begin(15 * 1000);
  DateTime.begin();
  DateTime.begin();
//  delay(5000);
  int c = 0;
  while (!DateTime.isTimeValid()) {
    delay(1000);
    //DateTime.setServer("time.cloudflare.com");
    //DateTime.setTimeZone(_timezone);
    DateTime.begin();
    if (c++ > 5) break;
  }
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
  _finterval = doc["FINT"];
  _clinterval = doc["CLINT"];
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
  doc["FINT"] = _finterval;
  doc["CLINT"] = _clinterval;
  doc["AUDIO"] = _audio;
  doc["SAVETIME"] = DateTime.format(DateFormatter::SIMPLE);

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write json to settings.txt"));
  }
  file.close();
  //update clock
  //DateTime.setTimeZone(_timezone); //deprecated
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
  for(uint16_t i = 0; i < sizeof(_cio.states); i++){
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
}

void BWC::print(String txt){
}

uint8_t BWC::getState(int state){
	return _cio.states[state];
}
