#include "BWC_8266_4w.h"

void CIO::begin() {
  //Setup serial to CIO and DSP here according to chosen PCB.
  /*
    Devices are sending on their TX lines, so we read that with RX pins on the ESP
    Hence the "backwards" parameters
  */
  cio_serial.begin(9600, SWSERIAL_8N1, CIO_TX, CIO_RX, false, 63);
  cio_serial.setTimeout(20);
  dsp_serial.begin(9600, SWSERIAL_8N1, DSP_TX, DSP_RX, false, 63);
  dsp_serial.setTimeout(20);

  states[TARGET] = 20;
  //Not used. Here for compatibility reasons
  states[LOCKEDSTATE] = false;
  states[POWERSTATE] = true;
  states[UNITSTATE] = true;   //Celsius. can be changed by web gui
  states[CHAR1] = ' ';
  states[CHAR2] = ' ';
  states[CHAR3] = ' ';
  cio_tx_ok = false;
  dsp_tx_ok = false;
}

void CIO::loop(void) {
  digitalWrite(D4, LOW);  //LED off
  //check if CIO has sent a message
  int msglen = 0;
  if(cio_serial.available())
  {
    msglen = cio_serial.readBytes(from_CIO_buf, PAYLOADSIZE);
    //copy from_CIO_buf -> to_DSP_buf
    if(msglen == PAYLOADSIZE)
    {
      //discard message if checksum is wrong
      uint8_t calculatedChecksum;
      calculatedChecksum = from_CIO_buf[1]+from_CIO_buf[2]+from_CIO_buf[3]+from_CIO_buf[4];
      if(from_CIO_buf[CIO_CHECKSUMINDEX] == calculatedChecksum)
      {
        for(int i = 0; i < PAYLOADSIZE; i++){
          if(to_DSP_buf[i] != from_CIO_buf[i]) dataAvailable = true;
          to_DSP_buf[i] = from_CIO_buf[i];
        }

        uint8_t newtemp = from_CIO_buf[TEMPINDEX];
        if(newtemp != states[TEMPERATURE])
        {
          deltaTemp = newtemp - states[TEMPERATURE];
          states[TEMPERATURE] = from_CIO_buf[TEMPINDEX];
          state_changed[TEMPERATURE] = true;
        }
        states[ERROR] =       from_CIO_buf[ERRORINDEX];
        cio_tx_ok = true;  //show the user that this line works (appears to work)
        //check if cio send error msg
        states[CHAR1] = ' ';
        states[CHAR2] = ' ';
        states[CHAR3] = ' ';
        if(states[ERROR])
        {
          to_CIO_buf[COMMANDINDEX] = 0; //clear any commands
          GODMODE = false;
          states[CHAR1] = 'E';
          states[CHAR2] = (char)(48+(from_CIO_buf[ERRORINDEX]/10));
          states[CHAR3] = (char)(48+(from_CIO_buf[ERRORINDEX]%10));
        }
      }
    }
    else
    {
      digitalWrite(D4, HIGH);  //LED on indicates bad message
    }
    /* debug
    else
    {
      if(msglen)
      {
        dataAvailable = true;
        for(int i = 0; i < msglen; i++)
        {
        dismissed_from_CIO_buf[i] = from_CIO_buf[i];
        }
        dismissed_cio_len = msglen;
      }
    }
    */
    // Do stuff here if you want to alter the message
    // Send last good message to DSP
    dsp_serial.write(to_DSP_buf, PAYLOADSIZE);
  }
  //check if display sent a message
  msglen = 0;
  if(dsp_serial.available())
  {
    msglen = dsp_serial.readBytes(from_DSP_buf, PAYLOADSIZE);
    //copy from_DSP_buf -> to_CIO_buf
    if(msglen == PAYLOADSIZE)
    {
      //discard message if checksum is wrong
      uint8_t calculatedChecksum;
      calculatedChecksum = from_DSP_buf[1]+from_DSP_buf[2]+from_DSP_buf[3]+from_DSP_buf[4];
      if(from_DSP_buf[DSP_CHECKSUMINDEX] == calculatedChecksum)
      {
        for(int i = 0; i < PAYLOADSIZE; i++)
        {
          to_CIO_buf[i] = from_DSP_buf[i];
        }
        //Do stuff here to command the CIO
        if(GODMODE){
          updatePayload();
        } else {
          updateStates();
        }
        dsp_tx_ok = true;  //show the user that this line works (appears to work)
      }
    }
    else
    {
      digitalWrite(D4, HIGH);  //LED on indicates bad message
    }
    cio_serial.write(to_CIO_buf, PAYLOADSIZE);
  }
}

void CIO::updateStates(){
  static uint8_t prevchksum;
  //extract information from payload to a better format
  states[BUBBLESSTATE] = (from_DSP_buf[COMMANDINDEX] & BUBBLESBITMASK) > 0;
  states[HEATGRNSTATE] = 2;                           //unknowable in antigodmode
  bool state = (from_DSP_buf[COMMANDINDEX] & (HEATBITMASK1|HEATBITMASK2)) > 0;
  if(state != states[HEATREDSTATE])
  {
    states[HEATREDSTATE] = state;
    state_changed[HEATREDSTATE] = true;
  }
  states[HEATSTATE] = 2;                              //unknowable in antigodmode
  state = (from_DSP_buf[COMMANDINDEX] & PUMPBITMASK) > 0;
  if(state != states[PUMPSTATE])
  {
    states[PUMPSTATE] = state;
    state_changed[PUMPSTATE] = true;
  }
  states[JETSSTATE] = (from_DSP_buf[COMMANDINDEX] & JETSBITMASK) > 0;
  if(from_DSP_buf[DSP_CHECKSUMINDEX] != prevchksum) dataAvailable = true;
  prevchksum = from_DSP_buf[DSP_CHECKSUMINDEX];
}

void CIO::updatePayload(){
  //alter payload to CIO to reflect user setting (GODMODE)
  //this is a simple thermostat with hysteresis. Will heat until target+1 and then cool until target-1
  static uint8_t prevchksum;
  static uint8_t hysteresis = 0;
  if(states[HEATSTATE] && ( (states[TEMPERATURE] + hysteresis) <= states[TARGET]) ){
    states[HEATREDSTATE] = 1; //on
    hysteresis = 0;
  } else {
    states[HEATREDSTATE] = 0; //off
    hysteresis = 1;
  }

  //antifreeze
  if(states[TEMPERATURE] < 10) states[HEATREDSTATE] = 1;

  states[HEATGRNSTATE] = !states[HEATREDSTATE] && states[HEATSTATE];
  to_CIO_buf[COMMANDINDEX] =  (states[HEATREDSTATE] * heatbitmask)    |
                              (states[JETSSTATE] * JETSBITMASK)       |
                              (states[PUMPSTATE] * PUMPBITMASK)       |
                              (states[BUBBLESSTATE] * BUBBLESBITMASK) |
                              (states[PUMPSTATE] * PUMPBITMASK);
  if(to_CIO_buf[COMMANDINDEX] > 0) to_CIO_buf[COMMANDINDEX] |= POWERBITMASK;

  //calc checksum -> byte5
  //THIS NEEDS TO BE IMPROVED IF OTHER CHECKSUMS IS USED (FOR OTHER BYTES in different models)
  to_CIO_buf[CIO_CHECKSUMINDEX] = to_CIO_buf[1] + to_CIO_buf[2] + to_CIO_buf[3] + to_CIO_buf[4];
  if(to_CIO_buf[CIO_CHECKSUMINDEX] != prevchksum) dataAvailable = true;
  prevchksum = to_CIO_buf[CIO_CHECKSUMINDEX];
}


void BWC::begin(void){
  _cio.begin();
  //_startNTP(); this is done from main.cpp
  LittleFS.begin();
  cio_tx_ok = false;
  dsp_tx_ok = false;
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
  _audio = 1;
  _tttt = 0;
  _tttt_calculated = 0;
  _tttt_time0 = DateTime.now()-3600;
  _tttt_time1 = DateTime.now();
  _tttt_temp0 = 20;
  _tttt_temp1 = 20;
  _loadSettings();
  _loadCommandQueue();
  _saveRebootInfo();
  saveSettingsTimer.attach(3600.0, std::bind(&BWC::saveSettingsFlag, this));

  _ambient_temp = 20;
  _virtualTempFix = -99;

}

void BWC::loop(){
  //feed the dog
  ESP.wdtFeed();
  ESP.wdtDisable();
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
  cio_tx_ok = _cio.cio_tx_ok;
  dsp_tx_ok = _cio.dsp_tx_ok;

  _handleStateChanges();
  _calcVirtualTemp();

}

/*  virtual temp functions  */

void BWC::_handleStateChanges()
{
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
  for(int i = 0; i < 15; i++)
  {
    if(_cio.state_changed[i])
    {
      _cio.state_changed[i] = false;
      _cio.state_age[i] = 0;
      //delayedAction[i] = true;
    }
  }
}

// return how many hours until pool is ready
float BWC::_estHeatingTime()
{
  if(_virtualTemp > _cio.states[TARGET]) return -2;

  float degAboveAmbient = _virtualTemp - _ambient_temp;
  float fraction = 1.0 - (degAboveAmbient - floor(degAboveAmbient));
  int tempdiff = _cio.states[TARGET] - _virtualTemp;

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
  for(int i = 1; i <= tempdiff; i++)
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
  else
    return -1;
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
  //startup init
  if(_virtualTempFix < -10)
  {
    _virtualTempFix = _cio.states[TEMPERATURE];
    _virtualTemp = _virtualTempFix;
    _virtualTempFix_age = 0;
    return;
  }

  //Do not process if temperature changed > 1 degree (reading spikes)
  if(abs(_cio.deltaTemp) != 1) return;

  //readings are only valid if pump is running and has been running for 5 min.
  if(!_cio.states[PUMPSTATE] || (_cio.state_age[PUMPSTATE] < 5*60000)) return;

  _virtualTemp = _cio.states[TEMPERATURE];
  _virtualTempFix = _cio.states[TEMPERATURE];
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
  // no check for bubbles in this version (as in 6-wire)
  if(_cio.states[HEATREDSTATE]) return;
  if(_cio.deltaTemp > 0 && _virtualTemp > _ambient_temp) return; //temp is rising when it should be falling. Bail out
  if(_cio.deltaTemp < 0 && _virtualTemp < _ambient_temp) return; //temp is falling when it should be rising. Bail out
  float degAboveAmbient = _virtualTemp - _ambient_temp;
  // can't calibrate if ambient ~ virtualtemp
  if(abs(degAboveAmbient) <= 1) return;
  R_COOLING = (_cio.state_age[TEMPERATURE]/3600000.0) / log((degAboveAmbient) / ((degAboveAmbient + _cio.deltaTemp)));
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

/*  end virtual temp functions*/

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

void BWC::_handleCommandQ(void) {
  bool restartESP = false;
  if(_qCommandLen > 0) {
  //cmp time with xtime. If more, then execute (adding buttons to buttonQ).

    if (_timestamp >= _commandQ[0][2]){
      switch (_commandQ[0][0]) {

        case SETTARGET:
          _cio.states[TARGET] = _commandQ[0][1];
          _cio.dataAvailable = true;
          if(_cio.states[TARGET] > 40) _cio.states[TARGET] = 40;  //don't cook anyone
          break;

        case SETUNIT:
          _cio.states[UNITSTATE] = _commandQ[0][1];
          _cio.dataAvailable = true;
          break;

        case SETBUBBLES:
          if(_cio.states[BUBBLESSTATE] == _commandQ[0][1]) break;  //no change required
          _cio.dataAvailable = true;
          _currentStateIndex = JUMPTABLE[_currentStateIndex][BUBBLETOGGLE];
          _cio.states[BUBBLESSTATE] = ALLOWEDSTATES[_currentStateIndex][BUBBLETOGGLE];
          _cio.states[JETSSTATE] = ALLOWEDSTATES[_currentStateIndex][JETSTOGGLE];
          _cio.states[PUMPSTATE] = ALLOWEDSTATES[_currentStateIndex][PUMPTOGGLE];
          _cio.heatbitmask = HEATBITMASK1;
          _cio.states[HEATSTATE] = ALLOWEDSTATES[_currentStateIndex][HEATTOGGLE]>0;
          if(ALLOWEDSTATES[_currentStateIndex][3] == 2) {
            qCommand(SETFULLPOWER, 1, _timestamp + 10, 0);
          }
          // _cio.states[BUBBLESSTATE] = _commandQ[0][1];
          // if(_commandQ[0][1]){
          // //bubbles is turned on. Limit H1, H2 and pump according to matrix
          //   _cio.states[PUMPSTATE] = _cio.states[PUMPSTATE] && (COMB_MATRIX & 1<<1);
          //   //lower the heating power (or turn it off if heatbitmask1 == 0)
          //   _cio.heatbitmask = (HEATBITMASK1 * ((COMB_MATRIX & 1<<8)>0)) | (HEATBITMASK2 * ((COMB_MATRIX & 1<<5)>0));
          //   //Keep jets state if allowed by the matrix (won't be turned on again automatically)
          //   _cio.states[JETSSTATE] = _cio.states[JETSSTATE] && (COMB_MATRIX & 1<<2);
          //   //check and limit H1, H2 and pump if Jets are on

          // } else {
          //   //bubbles is turned off
          //   _cio.heatbitmask = HEATBITMASK1 | HEATBITMASK2; //set full heating power
          // }
          break;

        case SETHEATER:
          if(_cio.states[HEATSTATE] == _commandQ[0][1]) break;  //no change required
          _cio.dataAvailable = true;
          _currentStateIndex = JUMPTABLE[_currentStateIndex][HEATTOGGLE];
          _cio.states[BUBBLESSTATE] = ALLOWEDSTATES[_currentStateIndex][BUBBLETOGGLE];
          _cio.states[JETSSTATE] = ALLOWEDSTATES[_currentStateIndex][JETSTOGGLE];
          _cio.states[PUMPSTATE] = ALLOWEDSTATES[_currentStateIndex][PUMPTOGGLE];
          _cio.heatbitmask = HEATBITMASK1;
          _cio.states[HEATSTATE] = ALLOWEDSTATES[_currentStateIndex][HEATTOGGLE]>0;
          if(ALLOWEDSTATES[_currentStateIndex][HEATTOGGLE] == 2) {
            qCommand(SETFULLPOWER, 1, _timestamp + 10, 0);
          }

          // _cio.states[HEATSTATE] = _commandQ[0][1];
          // if(_commandQ[0][1]) {
          //   if(_commandQ[0][1] == 1) {
          //     //start first heater element
          //     _cio.heatbitmask = HEATBITMASK1;
          //     //after 10 s start the other element
          //     qCommand(SETHEATER, 2, _timestamp + 10, 0);
          //     //mandatory ON regardless of the matrix allows it or not
          //     _cio.states[PUMPSTATE] = true;
          //     //Keeps states if allowed by the matrix
          //     _cio.states[BUBBLESSTATE] = _cio.states[BUBBLESSTATE] && (COMB_MATRIX & 1<<8);
          //     _cio.states[JETSSTATE] = _cio.states[JETSSTATE] && (COMB_MATRIX & 1<<7);
          //   }
          //   if(_commandQ[0][1] == 2) {
          //     _cio.heatbitmask |= HEATBITMASK2;
          //     //mandatory ON regardless of the matrix allows it or not
          //     _cio.states[PUMPSTATE] = true;
          //     //Keeps states if allowed by the matrix
          //     _cio.states[BUBBLESSTATE] = _cio.states[BUBBLESSTATE] && (COMB_MATRIX & 1<<5);
          //     _cio.states[JETSSTATE] &= _cio.states[JETSSTATE] && (COMB_MATRIX & 1<<4);
          //   }
          // }
          break;

        case SETPUMP:
          if(_cio.states[PUMPSTATE] == _commandQ[0][1]) break;  //no change required
          //let pump run a bit to cool element
          _cio.dataAvailable = true;
          if(_cio.states[HEATSTATE] && !_commandQ[0][1]) {
            qCommand(SETHEATER, 0, 0, 0);
            qCommand(SETPUMP, 0, _timestamp + 10, 0);
          } else {
            _currentStateIndex = JUMPTABLE[_currentStateIndex][PUMPTOGGLE];
            _cio.states[BUBBLESSTATE] = ALLOWEDSTATES[_currentStateIndex][BUBBLETOGGLE];
            _cio.states[JETSSTATE] = ALLOWEDSTATES[_currentStateIndex][JETSTOGGLE];
            _cio.states[PUMPSTATE] = ALLOWEDSTATES[_currentStateIndex][PUMPTOGGLE];
            _cio.heatbitmask = HEATBITMASK1;
            _cio.states[HEATSTATE] = ALLOWEDSTATES[_currentStateIndex][HEATTOGGLE]>0;
            if(ALLOWEDSTATES[_currentStateIndex][HEATTOGGLE] == 2) {
              qCommand(SETFULLPOWER, 1, _timestamp + 10, 0);
            }
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
          _jettime = 0;
          _uptime_ms = 0;
          _pumptime_ms = 0;
          _heatingtime_ms = 0;
          _airtime_ms = 0;
          _jettime_ms = 0;
          _cost = 0;
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
          if(_cio.states[JETSSTATE] == _commandQ[0][1]) break;  //no change required
          _cio.dataAvailable = true;
          _currentStateIndex = JUMPTABLE[_currentStateIndex][JETSTOGGLE];
          _cio.states[BUBBLESSTATE] = ALLOWEDSTATES[_currentStateIndex][BUBBLETOGGLE];
          _cio.states[JETSSTATE] = ALLOWEDSTATES[_currentStateIndex][JETSTOGGLE];
          _cio.states[PUMPSTATE] = ALLOWEDSTATES[_currentStateIndex][PUMPTOGGLE];
          _cio.heatbitmask = HEATBITMASK1;
          _cio.states[HEATSTATE] = ALLOWEDSTATES[_currentStateIndex][HEATTOGGLE]>0;
          if(ALLOWEDSTATES[_currentStateIndex][HEATTOGGLE] == 2) {
            qCommand(SETFULLPOWER, 1, _timestamp + 10, 0);
          }
          break;

        case SETGODMODE:
          _cio.GODMODE = _commandQ[0][1];
          _cio.states[HEATGRNSTATE] = 0;
          _cio.states[HEATREDSTATE] = 0;
          _cio.states[HEATSTATE] = 0;
          _cio.dataAvailable = true;
          break;

        case SETFULLPOWER:
          if(_commandQ[0][1]){
            if(ALLOWEDSTATES[_currentStateIndex][HEATTOGGLE] == 2) {
              _cio.heatbitmask = HEATBITMASK1 | HEATBITMASK2;
            }
          }
          else
            _cio.heatbitmask = HEATBITMASK1;
          break;
        case RESETDAILY:
          _energyDaily = 0;
          break;
        case SETAMBIENTF:
          setAmbientTemperature(_commandQ[0][1], false);
          break;
        case SETAMBIENTC:
          setAmbientTemperature(_commandQ[0][1], true);
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
        Serial.println("saving settings");
        saveSettings();
        delay(3000);
        Serial.println("restarting");
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
  doc["TGT"] = _cio.states[TARGET];
  doc["TMP"] = _cio.states[TEMPERATURE];
  doc["CH1"] = _cio.states[CHAR1];
  doc["CH2"] = _cio.states[CHAR2];
  doc["CH3"] = _cio.states[CHAR3];
  doc["JET"] = _cio.states[JETSSTATE];
  doc["ERR"] = _cio.states[ERROR];
  doc["GOD"] = _cio.GODMODE;
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
  doc["KWH"] = _energyTotal;
  doc["KWHD"] = _energyDaily;
  doc["WATT"] = _energyPower;
  doc["FINT"] = _finterval;
  doc["CLINT"] = _clinterval;
  doc["TTTT"] = _tttt;
  float t2r = _estHeatingTime();
  String t2r_string = String(t2r);
  if(t2r == -2) t2r_string = F("Already");
  if(t2r == -1) t2r_string = F("Never");
  doc["T2R"] = t2r_string;

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

String BWC::encodeBufferToString(uint8_t buf[7]){
  String str = String();
  for (unsigned long i = 0; i < 7; i++) {
    str += String(buf[i], HEX);
    str += " ";
  }
  return str;
}

String BWC::getSerialBuffers(){
  ESP.wdtFeed();
  DynamicJsonDocument doc(512);

  // Set the values in the document
  doc["CONTENT"] = "DEBUG";
  doc["TIME"] = _timestamp;
  doc["FROMDSP"] = encodeBufferToString(_cio.from_DSP_buf);
  doc["TOCIO"] = encodeBufferToString(_cio.to_CIO_buf);
  doc["FROMCIO"] = encodeBufferToString(_cio.from_CIO_buf);
  doc["TODSP"] = encodeBufferToString(_cio.to_DSP_buf);
  doc["REBOOTINFO"] = ESP.getResetReason();
  doc["REBOOTTIME"] = DateTime.getBootTime();
  /* debug
  doc["FROMDSPFAIL"] = encodeBufferToString(_cio.dismissed_from_DSP_buf);
  doc["LENDSP"] = _cio.dismissed_dsp_len;
  doc["FROMCIOFAIL"] = encodeBufferToString(_cio.dismissed_from_CIO_buf);
  doc["LENCIO"] = _cio.dismissed_cio_len;
  */
  // Serialize JSON to string
  String json;
  if (serializeJson(doc, json) == 0) {
    json = "{\"error\": \"Failed to serialize message\"}";
  }
  return json;
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
    if (c++ > 3) break;
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
  _jettime = doc["JETTIME"];
  _timezone = doc["TIMEZONE"];
  _price = doc["PRICE"];
  _finterval = doc["FINT"];
  _clinterval = doc["CLINT"];
  _audio = doc["AUDIO"];
  _energyTotal = doc["KWH"];
  _energyDaily = doc["KWHD"];
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
  ESP.wdtFeed();
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
  doc["R"] = R_COOLING;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write json to settings.txt"));
  }
  file.close();
  //update clock
  //DateTime.setTimeZone(_timezone); //deprecated
  //DateTime.begin();   //removed to lower risk of wdt reset.
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
  ESP.wdtFeed();
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
  ESP.wdtFeed();
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
  int elapsedtime_ms = now-prevtime;
  prevtime = now;
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
  //(some of) these age-counters resets when the state changes
  for(unsigned int i = 0; i < sizeof(_cio.state_age)/sizeof(uint32_t); i++)
  {
      _cio.state_age[i] += elapsedtime_ms;
  }
  _virtualTempFix_age += elapsedtime_ms;

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
}

uint8_t BWC::getState(int state){
  return _cio.states[state];
}

void BWC::reloadCommandQueue(){
    _loadCommandQueue();
    return;
}

float BWC::_C2F(float c)
{
  return c*1.8+32;
}

float BWC::_F2C(float f)
{
  return (f-32)/1.8;
}