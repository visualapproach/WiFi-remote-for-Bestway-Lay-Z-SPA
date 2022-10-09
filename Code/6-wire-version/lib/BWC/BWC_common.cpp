#include "BWC_common.h"

//match 7 segment pattern to a real digit
char CIO::_getChar(uint8_t value) {
  for (unsigned int index = 0; index < sizeof(CHARCODES); index++) {
    if (value == CHARCODES[index]) {
      return CHARS[index];
    }
  }
  return '*';
}

char DSP::_getCode(char value) {
  for (unsigned int index = 0; index < sizeof(CHARS); index++) {
    if (value == CHARS[index]) {
      return CHARCODES[index];
    }
  }
  return 0x00;  //no match, return 'space'
}

BWC::BWC(){}

void BWC::begin(void){
  _cio.begin(ciopins[0], ciopins[1], ciopins[2]);
  _dsp.begin(dsppins[0], dsppins[1], dsppins[2], dsppins[3]);
  begin2();
}

void BWC::begin2(){
  //Initialize variables
  _dspBrightness = 7; //default = max brightness
  _cltime = 0;
  _ftime = 0;
  _uptime = 0;
  _pumptime = 0;
  _heatingtime = 0;
  _airtime = 0;
  _jettime = 0;
  _timezone = 0;
  _price = 1;
  _finterval = 30;
  _clinterval = 14;
  _audio = true;
  _restoreStatesOnStart = false;
  LittleFS.begin();
  _loadSettings();
  _loadCommandQueue();
  _restoreStates();
  if(_audio) _dsp.playIntro();
  //_dsp.LEDshow();
  saveSettingsTimer.attach(3600.0, std::bind(&BWC::saveSettingsFlag, this));
  _tttt = 0;
  _tttt_calculated = 0;
  _tttt_time0 = DateTime.now()-3600;
  _tttt_time1 = DateTime.now();
  _tttt_temp0 = 20;
  _tttt_temp1 = 20;

  _ambient_temp = 20;
  _virtualTempFix = -99;
}

void BWC::stop(){
  _cio.stop();
}

void BWC::loop(){
  //feed the dog
  ESP.wdtFeed();
  // ESP.wdtDisable();

  _timestamp = DateTime.now();

  //update DSP payload (memcpy(dest*, source*, len))
  for(unsigned int i = 0; i < sizeof(_dsp.payload); i++){
    _dsp.payload[i] = _cio.payload[i];
  }
  //override_dsp_brt_timer is set to 5000 ms after physical button is touched.
  //it is decremented in updateTimes()
  //so display will be bright 5 secs after a buttoon press
  uint8_t brt = _dspBrightness;
  if(override_dsp_brt_timer > 0) brt = 8;
  _dsp.updateDSP(brt);
  _updateTimes();
  //update cio public payload
  _cio.loop();
  //manage command queue
  _handleCommandQ();
  //queue overrides real buttons
  _handleButtonQ();
  if(_saveEventlogNeeded) saveEventlog();
  if(_saveSettingsNeeded) saveSettings();
  //if set target command missed we need to correct that
  if( (_cio.states[TARGET] != _sliderTarget) && (_qButtonLen == 0) && (_sliderTarget != 0) && (_sliderPrio) ) qCommand(SETTARGET, _sliderTarget, 0, 0);
  //if target temp is unknown, find out.
  if( (_cio.states[TARGET] == 0) && (_qButtonLen == 0) ) qCommand(GETTARGET, (uint32_t)' ', 0, 0);

  //calculate time (in seconds) to target temperature
  //these variables can change anytime in the interrupts so copy them first
  uint8_t temperature = _cio.states[TEMPERATURE];
  uint8_t target = _cio.states[TARGET];
  //uint8_t unit = _cio.states[UNITSTATE];
  if(temperature != _tttt_temp1){
    _tttt_temp0 = _tttt_temp1;
    _tttt_temp1 = temperature;
    _tttt_time0 = _tttt_time1;
    _tttt_time1 = _timestamp;
  }
  int dtemp = _tttt_temp1 - _tttt_temp0;  //usually 1 or -1
  int dtime = _tttt_time1 - _tttt_time0;
  if(dtemp != 0 && abs(dtemp)<2) {        //if dtemp is larger we probably have a bad reading
    _tttt_calculated = (target-_tttt_temp1) * dtime/dtemp;
  }
  _tttt = _tttt_calculated - _timestamp + _tttt_time1;

  _handleStateChanges();
  _calcVirtualTemp();
  // ESP.wdtEnable(0);
}

void BWC::_handleStateChanges()
{
  //not used now, but possibility to take action some time after the change occured
  //static bool delayedAction[14];
  if(_cio.state_changed[UNITSTATE])
  {
    if(_cio.states[UNITSTATE])
    {
      _cio.states[TARGET] = round(_F2C(_cio.states[TARGET]));
    }
    else
    {
      _cio.states[TARGET] = round(_C2F(_cio.states[TARGET]));
    }
    _sliderTarget = _cio.states[TARGET];
  }

  // Save these changes so we can restore states on reboot
  if(_cio.state_changed[UNITSTATE] || _cio.state_changed[HEATSTATE] || _cio.state_changed[PUMPSTATE])
  {
    _saveStates();
  }

  // Temp must be stable for 500 ms before we do anything. Avoids spikes
  //if(delayedAction[TEMPERATURE] && (_cio.state_age[TEMPERATURE] > 500))
  if(_cio.state_changed[TEMPERATURE])
  {
    _updateVirtualTempFix_ontempchange();
    //delayedAction[TEMPERATURE] = false;
  }

  // Store virtual temp data point
  if(_cio.state_changed[HEATREDSTATE])
  {
    _updateVirtualTempFix_onheaterchange();
  }

  // Reset state ages for changed states
  for(int i = 0; i < 14; i++)
  {
    if(_cio.state_changed[i])
    {
      _cio.state_changed[i] = false;
      _cio.state_age[i] = 0;
      //delayedAction[i] = true;
      _newDataToSend = true;
    }
  }
}

// return how many hours until pool is ready
float BWC::_estHeatingTime()
{
  int targetInC = _cio.states[TARGET];
  if(!_cio.states[UNITSTATE]) targetInC = _F2C(targetInC);
  if(_virtualTemp > targetInC) return -2;  //Let us know when temp has fallen to target

  float degAboveAmbient = _virtualTemp - _ambient_temp;
  float fraction = 1.0 - (degAboveAmbient - floor(degAboveAmbient));
  int deltaTemp = targetInC - _virtualTemp;

  //integrate the time needed to reach target
  //how long to next integer temp
  float coolingPerHour = degAboveAmbient / R_COOLING;
  float netRisePerHour;
  if(_cio.states[HEATREDSTATE])
  {
    netRisePerHour = _heatingDegPerHour - coolingPerHour;
  }
  else
  {
    netRisePerHour = - coolingPerHour;
  }
  float hoursRemaining = fraction / netRisePerHour;
  //iterate up to target
  for(int i = 1; i <= deltaTemp; i++)
  {
    degAboveAmbient = _virtualTemp + i - _ambient_temp;
    coolingPerHour = degAboveAmbient / R_COOLING;
    if(_cio.states[HEATREDSTATE])
    {
      netRisePerHour = _heatingDegPerHour - coolingPerHour;
    }
    else
    {
      netRisePerHour = - coolingPerHour;
    }
    if(netRisePerHour != 0)
      hoursRemaining += 1 / netRisePerHour;
    else
      hoursRemaining = -1;
  }

  if(hoursRemaining >= 0)
  return hoursRemaining;
  else return -1;
}

//virtual temp is always C in this code and will be converted when sending externally
void BWC::_calcVirtualTemp()
{
  // calculate from last updated VTFix.
  float netRisePerHour;
  float degAboveAmbient = _virtualTemp - _ambient_temp;
  float coolingPerHour = degAboveAmbient / R_COOLING;

  if(_cio.states[HEATREDSTATE])
  {
    netRisePerHour = _heatingDegPerHour - coolingPerHour;
  }
  else
  {
    netRisePerHour = - coolingPerHour;
  }
  float elapsed_hours = _virtualTempFix_age / 3600.0 / 1000.0;
  float newvt = _virtualTempFix + netRisePerHour * elapsed_hours;

  // clamp VT to +/- 1 from measured temperature if pump is running
  if(_cio.states[PUMPSTATE] && (_cio.state_age[PUMPSTATE] > 5*60000))
  {
    float dev = newvt-_cio.states[TEMPERATURE];
    if(dev > 0.99) dev = 0.99;
    if(dev < -0.99) dev = -0.99;
    newvt = _cio.states[TEMPERATURE] + dev;
  }

  // Rebase start of calculation from new integer temperature
  if(int(_virtualTemp) != int(newvt))
  {
    _virtualTempFix = newvt;
    _virtualTempFix_age = 0;
  }
  _virtualTemp = newvt;

  /* Using Newtons law of cooling
      T(t) = Tenv + (T(0) - Tenv)*e^(-t/r)
      r = -t / ln( (T(t)-Tenv) / (T(0)-Tenv) )
      dT/dt = (T(t) - Tenv) / r
      ----------------------------------------
      T(t) : Temperature at time t
      Tenv : _ambient_temp (considered constant)
      T(0) : Temperature at time 0 (_virtualTempFix)
      e    : natural number 2,71828182845904
      r    : a constant we need to find out by measurements
  */

}

//Called on temp change
void BWC::_updateVirtualTempFix_ontempchange()
{
  int tempInC = _cio.states[TEMPERATURE];
  float conversion = 1;
  if(!_cio.states[UNITSTATE]) {
    tempInC = _F2C(tempInC);
    conversion = _F2C(1);
  }
  //startup init
  if(_virtualTempFix < -10)
  {
    _virtualTempFix = tempInC;
    _virtualTemp = _virtualTempFix;
    _virtualTempFix_age = 0;
    return;
  }

  //Do not process if temperature changed > 1 degree (reading spikes)
  if(abs(_cio.deltaTemp) != 1) return;

  //readings are only valid if pump is running and has been running for 5 min.
  if(!_cio.states[PUMPSTATE] || (_cio.state_age[PUMPSTATE] < 5*60000)) return;

  _virtualTemp = tempInC;
  _virtualTempFix = tempInC;
  _virtualTempFix_age = 0;
  /*
  update_coolingDegPerHourArray
  Measured temp has changed by 1 degree over a certain time
  1 degree/(temperature age in ms / 3600 / 1000)hours = 3 600 000 / temperature age in ms
  */

  // We can only know something about rate of change if we had continous cooling since last update
  // (Nobody messed with the heater during the 1 degree change)
  if(_cio.state_age[HEATREDSTATE] < _cio.state_age[TEMPERATURE]) return;
  // rate of heating is not subject to change (fixed wattage and pool size) so do this only if cooling
  // and do not calibrate if bubbles has been on
  if(_cio.states[HEATREDSTATE] || _cio.states[BUBBLESSTATE] || (_cio.state_age[BUBBLESSTATE] < _cio.state_age[TEMPERATURE])) return;
  if(_cio.deltaTemp > 0 && _virtualTemp > _ambient_temp) return; //temp is rising when it should be falling. Bail out
  if(_cio.deltaTemp < 0 && _virtualTemp < _ambient_temp) return; //temp is falling when it should be rising. Bail out
  float degAboveAmbient = _virtualTemp - _ambient_temp;
  // can't calibrate if ambient ~ virtualtemp
  if(abs(degAboveAmbient) <= 1) return;
  R_COOLING = (_cio.state_age[TEMPERATURE]/3600000.0) / log((conversion*degAboveAmbient) / (conversion*(degAboveAmbient + _cio.deltaTemp)));
}

//Called on heater state change
void BWC::_updateVirtualTempFix_onheaterchange()
{
  _virtualTempFix = _virtualTemp;
  _virtualTempFix_age = 0;
}

void BWC::setAmbientTemperature(int64_t amb, bool unit)
{
  _ambient_temp = (int)amb;
  if(!unit) _ambient_temp = _F2C(_ambient_temp);

  _virtualTempFix = _virtualTemp;
  _virtualTempFix_age = 0;
}

//save out debug text to file "debug.txt" on littleFS
void BWC::saveDebugInfo(String s){
  File file = LittleFS.open("debug.txt", "a");
  if (!file) {
    Serial.println(F("Failed to save debug.txt"));
    return;
  }

  DynamicJsonDocument doc(1024);

  // Set the values in the document
  doc["timestamp"] = DateTime.format(DateFormatter::SIMPLE);
  doc["message"] = s;
  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write debug.txt"));
  }
  file.close();
}


int BWC::_CodeToButton(uint16_t val){
  for(unsigned int i = 0; i < sizeof(ButtonCodes)/sizeof(uint16_t); i++){
    if(val == ButtonCodes[i]) return i;
  }
  return 0;
}

void BWC::_qButton(uint32_t btn, uint32_t state, uint32_t value, int32_t maxduration) {
  if(_qButtonLen == MAXBUTTONS) return;  //maybe textout an error message if queue is full?
  _buttonQ[_qButtonLen][0] = btn;
  _buttonQ[_qButtonLen][1] = state;
  _buttonQ[_qButtonLen][2] = value;
  _buttonQ[_qButtonLen][3] = maxduration;
  _qButtonLen++;
}

void BWC::_handleButtonQ(void) {
  static uint32_t prevMillis = millis();
  static uint32_t elapsedTime = 0;

  elapsedTime = millis() - prevMillis;
  prevMillis = millis();
  if(_qButtonLen > 0)
  {
    // First subtract elapsed time from maxduration
    _buttonQ[0][3] -= elapsedTime;
    //check if state is as desired, or duration is up. If so - remove row. Else set BTNCODE
    if( (_cio.states[_buttonQ[0][1]] == _buttonQ[0][2]) || (_buttonQ[0][3] <= 0) )
    {
      //remove row
      for(int i = 0; i < _qButtonLen-1; i++){
        _buttonQ[i][0] = _buttonQ[i+1][0];
        _buttonQ[i][1] = _buttonQ[i+1][1];
        _buttonQ[i][2] = _buttonQ[i+1][2];
        _buttonQ[i][3] = _buttonQ[i+1][3];
      }
      _qButtonLen--;
      _cio.button = ButtonCodes[NOBTN];
    }
    else
    {
      //set buttoncode
      _cio.button = ButtonCodes[_buttonQ[0][0]];
    }
  }
  else
  {
    static uint16_t prevbtn = ButtonCodes[NOBTN];
    //no queue so let dsp value through
    uint16_t pressedButton = _dsp.getButton();
    int index = _CodeToButton(pressedButton);
    //if button is not enabled, NOBTN will result (buttoncodes[0])
    _cio.button = ButtonCodes[index * EnabledButtons[index]];
    //prioritize manual temp setting by not competing with the set target command
    if (pressedButton == ButtonCodes[UP] || pressedButton == ButtonCodes[DOWN]) _sliderPrio = false;
    //do things when a new button is pressed
    if(index && (prevbtn == ButtonCodes[NOBTN]))
    {
      //make noise
      if(_audio && EnabledButtons[index]) _dsp.beep2();
      //store pressed buttons sequence
      for(int i = 0; i < 3; i++) _btnSequence[i] = _btnSequence[i+1];
      _btnSequence[3] = index;
      //set display brightness to max for 5 secs
      override_dsp_brt_timer = 5000;
    }
    prevbtn = pressedButton;
  }
}

//check for special button sequence
bool BWC::getBtnSeqMatch()
{
  if( _btnSequence[0] == POWER &&
      _btnSequence[1] == LOCK &&
      _btnSequence[2] == TIMER &&
      _btnSequence[3] == POWER)
  {
    return true;
  }
  return false;
}

bool BWC::qCommand(int64_t cmd, int64_t val, int64_t xtime, int64_t interval) {
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

void BWC::unlock()
{
  _qButton(POWER, POWERSTATE, 1, 5000); //press POWER button until states[POWERSTATE] is 1, max 5000 ms
  _qButton(LOCK, LOCKEDSTATE, 0, 5000); //press LOCK button until states[LOCKEDSTATE] is 0
}

void BWC::_handleCommandQ(void) {
  bool restartESP = false;
  if(_qCommandLen > 0) {
  //cmp time with xtime. If more, then execute (adding buttons to buttonQ).

    if (_timestamp >= _commandQ[0][2]){
      switch (_commandQ[0][0])
      {
        case SETTARGET:
          {
            unlock();
            uint8_t impliedunitcelsius = _commandQ[0][1] < 41;
            int diff;
            int desiredvalue = _commandQ[0][1];
            //pump is set to celsius but F given
            if(_cio.states[UNITSTATE] && !impliedunitcelsius)
              desiredvalue = (int)round(_F2C(_commandQ[0][1]));
            //pump is set to farenheit but C given
            if(!_cio.states[UNITSTATE] && impliedunitcelsius)
              desiredvalue = (int)round(_C2F(_commandQ[0][1]));
            diff = desiredvalue - (int)_cio.states[TARGET];
            int pushtime = 500;
            int releasetime = 300;
            uint32_t updown;
            diff<0 ? updown = DOWN : updown = UP;
            _qButton(updown, CHAR1, 0xFF, pushtime);
            _qButton(NOBTN, CHAR1, 0xFF, releasetime);
            _sliderTarget = desiredvalue;
            _sliderPrio = true;
            break;
          }
        case SETUNIT:
          unlock();
          _qButton(UNIT, UNITSTATE, _commandQ[0][1], 5000);
          _qButton(NOBTN, CHAR3, 0xFF, 300);
          break;
        case SETBUBBLES:
          unlock();
          _qButton(BUBBLES, BUBBLESSTATE, _commandQ[0][1], 5000);
          break;
        case SETHEATER:
          unlock();
          _qButton(HEAT, HEATSTATE, _commandQ[0][1], 5000);
          break;
        case SETPUMP:
          unlock();
          _qButton(PUMP, PUMPSTATE, _commandQ[0][1], 5000);
          break;
        case REBOOTESP:
          restartESP = true;
          break;
        case GETTARGET:
          unlock();
          _qButton(UP, CHAR3, 32, 500); //ignore desired value and wait for first blink. 32 = ' '
          _qButton(NOBTN, CHAR1, 0xFF, 5000);  //block further presses until blinking stops
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
          _energyTotal = 0;
          _saveSettingsNeeded = true;
          _cio.dataAvailable = true;
          break;
        case RESETCLTIMER:
          _cltime = _timestamp;
          _saveSettingsNeeded = true;
          _cio.dataAvailable = true;
          break;
        case RESETFTIMER:
          _ftime = _timestamp;
          _saveSettingsNeeded = true;
          _cio.dataAvailable = true;
          break;
        case SETJETS:
          unlock();
          _qButton(HYDROJETS, JETSSTATE, _commandQ[0][1], 5000);
          break;
        case SETBRIGHTNESS:
          if(_commandQ[0][1] < 9) _dspBrightness = _commandQ[0][1];
          break;
        case SETBEEP:
          _commandQ[0][1] == 0 ? _dsp.beep2() : _dsp.playIntro();
          break;
        case SETAMBIENTF:
          setAmbientTemperature(_commandQ[0][1], false);
          break;
        case SETAMBIENTC:
          setAmbientTemperature(_commandQ[0][1], true);
          break;
        case RESETDAILY:
          _energyDaily = 0;
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
        delay(3000);
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
  DynamicJsonDocument doc(1536);

  // Set the values in the document
  doc["CONTENT"] = "STATES";
  doc["TIME"] = _timestamp;
  doc["LCK"] = _cio.states[LOCKEDSTATE];
  doc["PWR"] = _cio.states[POWERSTATE];
  doc["UNT"] = _cio.states[UNITSTATE];
  doc["AIR"] = _cio.states[BUBBLESSTATE];
  doc["GRN"] = _cio.states[HEATGRNSTATE];
  doc["RED"] = _cio.states[HEATREDSTATE];
  doc["FLT"] = _cio.states[PUMPSTATE];
  doc["CH1"] = _cio.states[CHAR1];
  doc["CH2"] = _cio.states[CHAR2];
  doc["CH3"] = _cio.states[CHAR3];
  doc["HJT"] = _cio.states[JETSSTATE];
  doc["BRT"] = _dspBrightness;
  doc["TGT"] = _cio.states[TARGET];
  doc["TMP"] = _cio.states[TEMPERATURE];
  doc["VTF"] = _virtualTempFix; // **************************REMOVE THIS LINE
  doc["VTMC"] = _virtualTemp;
  doc["VTMF"] = _C2F(_virtualTemp);
  doc["AMBC"] = _ambient_temp;
  doc["AMBF"] = round(_C2F(_ambient_temp));
  if(_cio.states[UNITSTATE])
  {
    //celsius
    doc["AMB"] = _ambient_temp;
    doc["VTM"] = _virtualTemp;
    doc["TGTC"] = _cio.states[TARGET];
    doc["TMPC"] = _cio.states[TEMPERATURE];
    doc["TGTF"] = round(_C2F((float)_cio.states[TARGET]));
    doc["TMPF"] = round(_C2F((float)_cio.states[TEMPERATURE]));
    doc["VTMF"] = _C2F(_virtualTemp);
  }
  else
  {
    //farenheit
    doc["AMB"] = round(_C2F(_ambient_temp));
    doc["VTM"] = _C2F(_virtualTemp);
    doc["TGTF"] = _cio.states[TARGET];
    doc["TMPF"] = _cio.states[TEMPERATURE];
    doc["TGTC"] = round(_F2C((float)_cio.states[TARGET]));
    doc["TMPC"] = round(_F2C((float)_cio.states[TEMPERATURE]));
    doc["VTMC"] = _virtualTemp;
  }

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
  doc["JETTIME"] = _jettime + _jettime_ms/1000;
  doc["COST"] = _energyTotal * _price;
  doc["FINT"] = _finterval;
  doc["CLINT"] = _clinterval;
  doc["KWH"] = _energyTotal;
  doc["KWHD"] = _energyDaily;
  doc["WATT"] = _energyPower;
  doc["TTTT"] = _tttt;
  float t2r = _estHeatingTime();
  String t2r_string = String(t2r);
  if(t2r == -2) t2r_string = F("Already");
  if(t2r == -1) t2r_string = F("Never");
  doc["T2R"] = t2r_string;
  doc["MINCLK"] = _cio.clk_per;
  _cio.clk_per = 1000;  //reset minimum clock period

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
  doc["RESTORE"] = _restoreStatesOnStart;
  doc["MODEL"] = MYMODEL;

  // Serialize JSON to string
  String jsonmsg;
  if (serializeJson(doc, jsonmsg) == 0) {
    jsonmsg = "{\"error\": \"Failed to serialize message\"}";
  }
  return jsonmsg;
}

void BWC::setJSONSettings(String message){
  //feed the dog
  // ESP.wdtFeed();
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
  _restoreStatesOnStart = doc["RESTORE"];
  saveSettings();
}

String BWC::getJSONCommandQueue(){
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
  bool result = _newDataToSend;
  _newDataToSend = false;
  if (result && _audio) _dsp.beep();
  return result;
}

void BWC::_startNTP() {
  // setup this after wifi connected
  DateTime.setServer("pool.ntp.org");
  DateTime.begin();
  DateTime.begin();
  int c = 0;
  while (!DateTime.isTimeValid()) {
    Serial.println(F("Failed to get time from server. Trying again."));
    delay(1000);
    //DateTime.setServer("time.cloudflare.com");
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
  _jettime = doc["JETTIME"];
  _timezone = doc["TIMEZONE"];
  _price = doc["PRICE"];
  _finterval = doc["FINT"];
  _clinterval = doc["CLINT"];
  _audio = doc["AUDIO"];
  _energyTotal = doc["KWH"];
  _energyDaily = doc["KWHD"];
  _restoreStatesOnStart = doc["RESTORE"];
  if(doc.containsKey("R")) R_COOLING = doc["R"]; //else use default
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
  // ESP.wdtDisable();
  ESP.wdtFeed();
  _saveSettingsNeeded = false;
  File file = LittleFS.open("settings.txt", "w");
  if (!file) {
    Serial.println(F("Failed to save settings.txt"));
    return;
  }

  DynamicJsonDocument doc(1024);
  _heatingtime += _heatingtime_ms/1000;
  _pumptime += _pumptime_ms/1000;
  _airtime += _airtime_ms/1000;
  _jettime += _jettime_ms/1000;
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
  doc["JETTIME"] = _jettime;
  doc["TIMEZONE"] = _timezone;
  doc["PRICE"] = _price;
  doc["FINT"] = _finterval;
  doc["CLINT"] = _clinterval;
  doc["AUDIO"] = _audio;
  doc["KWH"] = _energyTotal;
  doc["KWHD"] = _energyDaily;
  doc["SAVETIME"] = DateTime.format(DateFormatter::SIMPLE);
  doc["RESTORE"] = _restoreStatesOnStart;
  doc["R"] = R_COOLING;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write json to settings.txt"));
  }
  file.close();
  //revive the dog
  // ESP.wdtEnable(0);
}

void BWC::_loadCommandQueue(){
  File file = LittleFS.open("cmdq.txt", "r");
  if (!file) {
    Serial.println(F("Failed to read cmdq.txt"));
    return;
  }

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
  // ESP.wdtDisable();
  ESP.wdtFeed();
  File file = LittleFS.open("cmdq.txt", "w");
  if (!file) {
    Serial.println(F("Failed to save cmdq.txt"));
    return;
  }

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
  // ESP.wdtEnable(0);

}

void BWC::reloadCommandQueue(){
    _loadCommandQueue();
    return;
}

void BWC::reloadSettings(){
    _loadSettings();
    return;
}

void BWC::_saveStates() {
  //kill the dog
  // ESP.wdtDisable();
  ESP.wdtFeed();
  _saveStatesNeeded = false;
  File file = LittleFS.open("states.txt", "w");
  if (!file) {
    Serial.println(F("Failed to save states.txt"));
    return;
  }

  DynamicJsonDocument doc(1024);

  // Set the values in the document
  doc["UNT"] = _cio.states[UNITSTATE];
  doc["HTR"] = _cio.states[HEATSTATE];
  doc["FLT"] = _cio.states[PUMPSTATE];

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write states.txt"));
  }
  file.close();
  //revive the dog
  // ESP.wdtEnable(0);
}

void BWC::_restoreStates() {
  if(!_restoreStatesOnStart) return;
  File file = LittleFS.open("states.txt", "r");
  if (!file) {
    Serial.println(F("Failed to read states.txt"));
    return;
  }
  DynamicJsonDocument doc(512);
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to deserialize states.txt"));
    file.close();
    return;
  }

  uint8_t unt = doc["UNT"];
  uint8_t flt = doc["FLT"];
  uint8_t htr = doc["HTR"];
  qCommand(SETUNIT, unt, 0, 0);
  _cio.states[UNITSTATE] = unt;
  qCommand(SETPUMP, flt, 0, 0);
  qCommand(SETHEATER, htr, 0, 0);
  Serial.println("restoring states");
  file.close();
}

void BWC::saveEventlog(){
  //kill the dog
  // ESP.wdtDisable();
  ESP.wdtFeed();
  File file = LittleFS.open("eventlog.txt", "a");
  if (!file) {
    Serial.println(F("Failed to save eventlog.txt"));
    return;
  }

  DynamicJsonDocument doc(1024);

  // Set the values in the document
  for(unsigned int i = 0; i < sizeof(_cio.states); i++){
  doc[i] = _cio.states[i];
  }
  doc["timestamp"] = DateTime.format(DateFormatter::SIMPLE);

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write eventlog.txt"));
  }
  file.close();
  //revive the dog
  // ESP.wdtEnable(0);

}

void BWC::saveRebootInfo(){
  File file = LittleFS.open("bootlog.txt", "a");
  if (!file) {
    Serial.println(F("Failed to save bootlog.txt"));
    return;
  }

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
  static uint32_t prevtime = now;
  int elapsedtime_ms = now-prevtime;
   prevtime = now;
 //(some of) these age-counters resets when the state changes
  for(unsigned int i = 0; i < sizeof(_cio.state_age)/sizeof(uint32_t); i++)
  {
      _cio.state_age[i] += elapsedtime_ms;
  }
  _virtualTempFix_age += elapsedtime_ms;

  if (elapsedtime_ms < 0) return; //millis() rollover every 49 days
  if(_cio.states[HEATREDSTATE]){
    _heatingtime_ms += elapsedtime_ms;
  }
  if(_cio.states[PUMPSTATE]){
    _pumptime_ms += elapsedtime_ms;
  }
  if(_cio.states[BUBBLESSTATE]){
    _airtime_ms += elapsedtime_ms;
  }
  if(_cio.states[JETSSTATE]){
    _jettime_ms += elapsedtime_ms;
  }
  _uptime_ms += elapsedtime_ms;


  if(_uptime_ms > 1000000000){
    _heatingtime += _heatingtime_ms/1000;
    _pumptime += _pumptime_ms/1000;
    _airtime += _airtime_ms/1000;
    _jettime += _jettime_ms/1000;
    _uptime += _uptime_ms/1000;
    _heatingtime_ms = 0;
    _pumptime_ms = 0;
    _airtime_ms = 0;
    _jettime_ms = 0;
    _uptime_ms = 0;
  }

  if(override_dsp_brt_timer > 0) override_dsp_brt_timer -= elapsedtime_ms; //counts down to or below zero

  // watts, kWh today, total kWh
  float heatingEnergy = (_heatingtime+_heatingtime_ms/1000)/3600.0 * HEATERPOWER;
  float pumpEnergy = (_pumptime+_pumptime_ms/1000)/3600.0 * PUMPPOWER;
  float airEnergy = (_airtime+_airtime_ms/1000)/3600.0 * AIRPOWER;
  float idleEnergy = (_uptime+_uptime_ms/1000)/3600.0 * IDLEPOWER;
  float jetEnergy = (_jettime+_jettime_ms/1000)/3600.0 * JETPOWER;
  _energyTotal = (heatingEnergy + pumpEnergy + airEnergy + idleEnergy + jetEnergy)/1000; //Wh -> kWh
  _energyPower = _cio.states[HEATREDSTATE] * HEATERPOWER;
  _energyPower += _cio.states[PUMPSTATE] * PUMPPOWER;
  _energyPower += _cio.states[BUBBLESSTATE] * AIRPOWER;
  _energyPower += IDLEPOWER;
  _energyPower += _cio.states[JETSSTATE] * JETPOWER;

  _energyDaily += (elapsedtime_ms / 1000.0) / 3600.0 * _energyPower / 1000.0;
}

void BWC::print(String txt){
  _dsp.textOut(txt);
}

uint8_t BWC::getState(int state){
  return _cio.states[state];
}

String BWC::getPressedButton(){
  uint16_t btn = _dsp.getButton();
  uint8_t hib, lob;
  String s;
  hib = (uint8_t)(btn>>8);
  lob = (uint8_t)(btn & 0xFF);
  s = hib < 16 ? "0" + String(hib, HEX) : String(hib, HEX);
  s += lob < 16 ? "0" + String(lob, HEX) : String(lob, HEX);
  return  s;
}

String BWC::getButtonName() {
  return ButtonNames[_CodeToButton(_dsp.getButton() )];
}

float BWC::_C2F(float c)
{
  return c*1.8+32;
}

float BWC::_F2C(float f)
{
  return (f-32)/1.8;
}