#include "CIO_4W.h"
#include "util.h"

void CIO_4W::setup(int cio_rx, int cio_tx, int dummy)
{
    //Setup serial to CIO and DSP here according to chosen PCB.
    /*
        Devices are sending on their TX lines, so we read that with RX pins on the ESP
        Hence the "backwards" parameters (cio_tx goes to ESP serial RX)
    */
    _cio_serial.begin(9600, SWSERIAL_8N1, cio_tx, cio_rx, false, 63);
    _cio_serial.setTimeout(20);

    _actual_states.target = 20;
    _actual_states.locked = false;
    _actual_states.power = true;
    _actual_states.unit = true;
    _actual_states.char1 = ' ';
    _actual_states.char2 = ' ';
    _actual_states.char3 = ' ';
    
    _currentStateIndex = 0;
}

void CIO_4W::stop()
{
    _cio_serial.stopListening();
}

void CIO_4W::setStates(const sToggles& requested_toggles)
{
    uint64_t elapsed_time_ms = 0;
    uint64_t prev_ms = millis();
    elapsed_time_ms = millis() - prev_ms;
    prev_ms = millis();
    _actual_states.target = requested_toggles.target;

    if(_heater2_countdown_ms > 0) _heater2_countdown_ms -= elapsed_time_ms;
    if(_cool_heater_countdown_ms > 0) _cool_heater_countdown_ms -= elapsed_time_ms;
    /*After count down we turn off pump just once*/
    if(_cool_heater_countdown_ms <= 0 && _turn_off_heater_flag)
    {
        _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
        togglestates();
        _turn_off_heater_flag = false;
    }

    for(unsigned int i = 0; i < sizeof(_from_CIO_buf); i++)
        _raw_payload_from_cio[i] = _from_CIO_buf[i];

    if(!requested_toggles.godmode)
    {
        /*Copy raw payload to CIO*/
        for(unsigned int i = 0; i < sizeof(_to_CIO_buf); i++)
            _to_CIO_buf[i] = _raw_payload_to_cio[i];
        // _cio_serial.write(_to_CIO_buf, PAYLOADSIZE); //this is done in getStates()
        _actual_states.godmode = false;
        return;
    } else {
        _actual_states.godmode = true;
    }

    if(requested_toggles.unit_change)
    {
        _actual_states.unit = !_actual_states.unit;
        _actual_states.unit ? _actual_states.target = F2C(_actual_states.target) : _actual_states.target = C2F(_actual_states.target);
    }

    if(requested_toggles.heat_change)
    {
        _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
        togglestates();
    }

    if(requested_toggles.bubbles_change && getHasair())
    {
            _currentStateIndex = getJumptable(_currentStateIndex, BUBBLETOGGLE);
            togglestates();
    }

    if(requested_toggles.pump_change)
    {
        if(!_actual_states.pump)
        {
            _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
            togglestates();
        } 
        else
        {
            /*Pump turning OFF -> turn off heaters first, and start countdown*/
            if(_actual_states.heat)
            {
                _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
                togglestates();
                _cool_heater_countdown_ms = _HEATERCOOLING_DELAY_MS;
                _turn_off_heater_flag = true;
            }
        }
    }

    if(requested_toggles.jets_change && getHasjets())
    {
        _currentStateIndex = getJumptable(_currentStateIndex, JETSTOGGLE);
        togglestates();
    }

    if(requested_toggles.no_of_heater_elements_on < 2)
    {
        _heat_bitmask = getHeatBitmask1();
    }
    _actual_states.no_of_heater_elements_on = requested_toggles.no_of_heater_elements_on;

    regulateTemp();
    antifreeze();
    antiboil();
    generatePayload();
}

void CIO_4W::generatePayload()
{
    _to_CIO_buf[0] = B01010101; //Start of file
    _to_CIO_buf[1] = 1;
    _to_CIO_buf[3] = 48;
    _to_CIO_buf[4] = 0;
    _to_CIO_buf[6] = B10101010; //End of file
    _actual_states.heatgrn = !_actual_states.heatred && _actual_states.heat;
    _to_CIO_buf[COMMANDINDEX] =  (_actual_states.heatred * _heat_bitmask)    |
                                (_actual_states.jets * getJetsBitmask())       |
                                (_actual_states.bubbles * getBubblesBitmask()) |
                                (_actual_states.pump * getPumpBitmask());
    if(_to_CIO_buf[COMMANDINDEX] > 0) _to_CIO_buf[COMMANDINDEX] |= getPowerBitmask();

    //calc checksum -> byte5
    //THIS NEEDS TO BE IMPROVED IF OTHER CHECKSUMS IS USED (FOR OTHER BYTES in different models)
    _to_CIO_buf[CIO_CHECKSUMINDEX] = _to_CIO_buf[1] + _to_CIO_buf[2] + _to_CIO_buf[3] + _to_CIO_buf[4];
}

sStates CIO_4W::getStates()
{
    int msglen = 0;
    //check if CIO has sent a message
    if(!_cio_serial.available()) return _actual_states;
    uint8_t tempbuffer[PAYLOADSIZE];
    msglen = _cio_serial.readBytes(tempbuffer, PAYLOADSIZE);
    if(msglen != PAYLOADSIZE) return _actual_states;
    uint8_t calculatedChecksum;
    calculatedChecksum = tempbuffer[1]+tempbuffer[2]+tempbuffer[3]+tempbuffer[4];
    if(tempbuffer[CIO_CHECKSUMINDEX] != calculatedChecksum)
    {
        //badCIO_checksum++;
        return _actual_states;
    }
    /*message is good if we get here. Continue*/

    /* show the user that this line works (appears to work) */
    // cio_tx_ok = true;
    /* Copy tempbuffer into _from_CIO_buf */
    for(int i = 0; i < PAYLOADSIZE; i++)
        _from_CIO_buf[i] = tempbuffer[i];

    _actual_states.temperature = _from_CIO_buf[TEMPINDEX];
    if(!_actual_states.unit) _actual_states.temperature = C2F(_actual_states.temperature);
    _actual_states.error = _from_CIO_buf[ERRORINDEX];
    /*Show temp in web UI display*/
    _actual_states.char1 = 48+(_actual_states.temperature)/100;
    _actual_states.char2 = 48+(_actual_states.temperature % 100)/10;
    _actual_states.char3 = 48+(_actual_states.temperature % 10);
    //check if cio send error msg
    if(_actual_states.error)
    {
        _to_CIO_buf[COMMANDINDEX] = 0; //clear any commands
        _actual_states.godmode = false;
        _actual_states.char1 = 'E';
        _actual_states.char2 = (char)(48+(_from_CIO_buf[ERRORINDEX]/10));
        _actual_states.char3 = (char)(48+(_from_CIO_buf[ERRORINDEX]%10));
    }
    /*This is placed here so we send messages at the same rate as the cio.*/
    _cio_serial.write(_to_CIO_buf, PAYLOADSIZE);
    return _actual_states;
}

void CIO_4W::togglestates()
{
    _actual_states.bubbles = getAllowedstates(_currentStateIndex, BUBBLETOGGLE);
    _actual_states.jets = getAllowedstates(_currentStateIndex, JETSTOGGLE);
    _actual_states.pump = getAllowedstates(_currentStateIndex, PUMPTOGGLE);
    _actual_states.heat = getAllowedstates(_currentStateIndex, HEATTOGGLE)>0;
}

void CIO_4W::regulateTemp()
{
    //this is a simple thermostat with hysteresis. Will heat until target+1 and then cool until target-1
    static uint8_t hysteresis = 0;
    if(!_actual_states.heat)
    {
        _actual_states.heatred = 0;
        return;
    }

    if( (_actual_states.temperature + hysteresis) <= _actual_states.target)
    {
        if(!_actual_states.heatred)
        {
            _heat_bitmask = getHeatBitmask1(); //half power at start
            _actual_states.heatred = 1;   //on
            _heater2_countdown_ms = _HEATER2_DELAY_MS;
        }
        hysteresis = 0;
    }
    else
    {
        _actual_states.heatred = 0; //off
        hysteresis = 1;
    }

    /*Start 2nd heater element*/
    if((_heater2_countdown_ms <= 0) && (_actual_states.no_of_heater_elements_on == 2))
    {
        _heat_bitmask = getHeatBitmask1() | getHeatBitmask2();
    }
}

void CIO_4W::antifreeze()
{
    /*
        Antifreeze. This will only run in GODMODE.
        In normal mode the pump should behave as from factory.
        - Will start pump and heater and set target temperature to 10.
        - Pump will run until "manually" turned off. (From Panel, Web UI or MQTT)
    */
    int tempC = _actual_states.temperature;
    int targetC;
    if(!_actual_states.unit) tempC = F2C(tempC);
    if(tempC < 10)
    {
        if(!_actual_states.pump)
        {
            _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
            togglestates();
        } 
        if(!_actual_states.heat)
        {
            _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
            togglestates();
        } 
        targetC = 10;
        Serial.printf("antifrz active\n");
        if(_actual_states.unit) _actual_states.target = targetC;
        else _actual_states.target = C2F(targetC);
    }
}

void CIO_4W::antiboil()
{
    /*
        Anti overtemp
    */
    int tempC = _actual_states.temperature;
    if(!_actual_states.unit) tempC = F2C(tempC);

    if(tempC > 41)
    {
        /*Turn ON pump, and OFF heat*/
        if(!_actual_states.pump)
        {
            _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
            togglestates();
        }
        if(_actual_states.heat)
        {
            _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
            togglestates();
        } 
        Serial.printf("antiboil active\n");
    }

    /*
    // Alternative solution to both antifreeze and antiboil; Hands off if temperature goes extreme.
    if((states[TEMPERATURE] < 10) || (states[TEMPERATURE] > 41))
        bwc->qCommand(SETGODMODE, 0, 0, 0);
    */
}
