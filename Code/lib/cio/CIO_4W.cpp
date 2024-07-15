#include "CIO_4W.h"
#include "util.h"

void CIO_4W::setup(int cio_rx, int cio_tx, int dummy)
{
    //Setup serial to CIO and DSP here according to chosen PCB.
    /*
        Devices are sending on their TX lines, so we read that with RX pins on the ESP
        Hence the "backwards" parameters (cio_tx goes to ESP serial RX)
    */
    HeapSelectIram ephemeral;
    _cio_serial = new EspSoftwareSerial::UART;

    _cio_serial->begin(9600, SWSERIAL_8N1, cio_tx, cio_rx, false, 24);
    _cio_serial->setTimeout(20);
    cio_states.target = 20;
    cio_states.locked = false;
    cio_states.power = true;
    cio_states.unit = true;
    cio_states.char1 = ' ';
    cio_states.char2 = ' ';
    cio_states.char3 = ' ';
    
    _currentStateIndex = 0;
    _cio_serial->write(_to_CIO_buf, PAYLOADSIZE);  //test
}

void CIO_4W::stop()
{
    _cio_serial->stopListening();
    delete _cio_serial;
    _cio_serial = nullptr;
}

void CIO_4W::pause_all(bool action)
{
    if(action)
    {
        _cio_serial->stopListening();
    } else
    {
        _cio_serial->listen();
    }
}

void CIO_4W::handleToggles()
{
    uint32_t elapsed_time_ms = 0;
    elapsed_time_ms = millis() - _prev_ms;
    _prev_ms = millis();
    _time_since_last_transmission_ms += elapsed_time_ms;

    cio_states.target = cio_toggles.target;

    if(_heater2_countdown_ms > 0) _heater2_countdown_ms -= elapsed_time_ms;
    if(_cool_heater_countdown_ms > 0) _cool_heater_countdown_ms -= elapsed_time_ms;
    /*After count down we turn off pump just once*/
    if(_cool_heater_countdown_ms <= 0 && _turn_off_pump_flag)
    {
        _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
        togglestates();
        _turn_off_pump_flag = false;
    }

    if(!cio_toggles.godmode)
    {
        /*Copy raw payload to CIO*/
        for(unsigned int i = 0; i < sizeof(_to_CIO_buf); i++)
            _to_CIO_buf[i] = _raw_payload_to_cio[i];
        // _cio_serial->write(_to_CIO_buf, PAYLOADSIZE); //this is done in updateStates()
        cio_states.godmode = false;

        HeaterStages heater_stages {
            .stage1_on = (_to_CIO_buf[COMMANDINDEX] & getHeatBitmask1()) == getHeatBitmask1(),
            .stage2_on = (_to_CIO_buf[COMMANDINDEX] & getHeatBitmask2()) == getHeatBitmask2(),
        };
        setHeaterStages(heater_stages);

        if(_readyToTransmit)
        {
            _readyToTransmit = false;
            _cio_serial->write(_to_CIO_buf, PAYLOADSIZE);
        }
        return;
    } else {
        cio_states.godmode = true;
    }

    if(cio_toggles.unit_change)
    {
        cio_states.unit = !cio_states.unit;
        /*requested target is converted in bwc.cpp*/
        // cio_states.unit ? cio_states.target = round(F2C(cio_states.target)) : cio_states.target = round(C2F(cio_states.target));
        cio_states.unit ? cio_states.temperature = round(F2C(cio_states.temperature)) : cio_states.temperature = round(C2F(cio_states.temperature));
    }

    if(cio_toggles.heat_change)
    {
        _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
        togglestates();
    }

    if(cio_toggles.bubbles_change && getHasair())
    {
            _currentStateIndex = getJumptable(_currentStateIndex, BUBBLETOGGLE);
            togglestates();
    }

    if(cio_toggles.pump_change)
    {
        if(!cio_states.pump)
        {
            _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
            togglestates();
        } 
        else
        {
            /*Pump turning OFF -> turn off heaters first, and start countdown*/
            if(cio_states.heat)
            {
                _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
                togglestates();
                _cool_heater_countdown_ms = _HEATERCOOLING_DELAY_MS;
                _turn_off_pump_flag = true;
            }
            else
            {
                _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
                togglestates();
            }
        }
    }

    if(cio_toggles.jets_change && getHasjets())
    {
        _currentStateIndex = getJumptable(_currentStateIndex, JETSTOGGLE);
        togglestates();
    }

    if(cio_toggles.no_of_heater_elements_on < 2)
    {
        _heat_bitmask = getHeatBitmask1();
    }
    cio_states.no_of_heater_elements_on = cio_toggles.no_of_heater_elements_on;

    regulateTemp();
    antifreeze();
    antiboil();
    generatePayload();
    if(_readyToTransmit || (_time_since_last_transmission_ms > _max_time_between_transmissions_ms))
    {
        _readyToTransmit = false;
        _time_since_last_transmission_ms = 0;
        _cio_serial->write(_to_CIO_buf, PAYLOADSIZE);
        write_msg_count++;
    }
}

void CIO_4W::generatePayload()
{
    /* 2023-06-11 edit: Revert back to just copying the unknowns instead of generating a fixed value */
    _to_CIO_buf[0] = _raw_payload_to_cio[0]; //Start of file
    _to_CIO_buf[1] = _raw_payload_to_cio[1]; //Unknown, usually 1
    cio_states.heatgrn = !cio_states.heatred && cio_states.heat;
    _to_CIO_buf[COMMANDINDEX] =  (cio_states.heatred * _heat_bitmask)    |
                                (cio_states.jets * getJetsBitmask())       |
                                (cio_states.bubbles * getBubblesBitmask()) |
                                (cio_states.pump * getPumpBitmask());
    if(_to_CIO_buf[COMMANDINDEX] > 0) _to_CIO_buf[COMMANDINDEX] |= getPowerBitmask();
    _to_CIO_buf[3] = _raw_payload_to_cio[3]; //Unknown, usually 48
    _to_CIO_buf[4] = _raw_payload_to_cio[4]; //Unknown, usually 0

    //calc checksum -> byte5
    //THIS NEEDS TO BE IMPROVED IF OTHER CHECKSUMS IS USED (FOR OTHER BYTES in different models)
    _to_CIO_buf[CIO_CHECKSUMINDEX] = _to_CIO_buf[1] + _to_CIO_buf[2] + _to_CIO_buf[3] + _to_CIO_buf[4];

    _to_CIO_buf[6] = _raw_payload_to_cio[6]; //End of file
}

void CIO_4W::updateStates()
{
    int msglen = 0;
    //check if CIO has sent a message
    if(!_cio_serial->available()) return;
    uint8_t tempbuffer[PAYLOADSIZE];
    msglen = _cio_serial->readBytes(tempbuffer, PAYLOADSIZE);
    if(msglen != PAYLOADSIZE) return;
    uint8_t calculatedChecksum;
    calculatedChecksum = tempbuffer[1]+tempbuffer[2]+tempbuffer[3]+tempbuffer[4];
    if(tempbuffer[CIO_CHECKSUMINDEX] != calculatedChecksum)
    {
        bad_packets_count++;
        return;
    }
    /*message is good if we get here. Continue*/

    good_packets_count++;
    /* show the user that this line works (appears to work) */
    // cio_tx_ok = true;
    /* Copy tempbuffer into _from_CIO_buf */
    for(int i = 0; i < PAYLOADSIZE; i++)
    {
        _from_CIO_buf[i] = tempbuffer[i];
        _raw_payload_from_cio[i] = tempbuffer[i];
    }

    cio_states.temperature = _from_CIO_buf[TEMPINDEX];
    if(!cio_states.unit) cio_states.temperature = C2F(cio_states.temperature);
    cio_states.error = _from_CIO_buf[ERRORINDEX];
    /*Show temp in web UI display*/
    cio_states.char1 = 48+(cio_states.temperature)/100;
    cio_states.char2 = 48+(cio_states.temperature % 100)/10;
    cio_states.char3 = 48+(cio_states.temperature % 10);
    //check if cio send error msg
    if(cio_states.error)
    {
        _to_CIO_buf[COMMANDINDEX] = 0; //clear any commands
        cio_states.godmode = false;
        cio_states.char1 = 'E';
        cio_states.char2 = (char)(48+(_from_CIO_buf[ERRORINDEX]/10));
        cio_states.char3 = (char)(48+(_from_CIO_buf[ERRORINDEX]%10));
    }
    /*This is placed here so we send messages at the same rate as the cio.*/
    // _cio_serial->write(_to_CIO_buf, PAYLOADSIZE);
    _serialreceived = true;
    return;
}


/* bwc can send data to dsp */
bool CIO_4W::getSerialReceived()
{
    bool result = _serialreceived;
    _serialreceived = false;
    return result;
}

/* bwc is telling us that it's okay by dsp to transmit */
void CIO_4W::setSerialReceived(bool txok)
{
    /* Don't forget to reset after transmitting */
    _readyToTransmit = txok;
}

void CIO_4W::togglestates()
{
    cio_states.bubbles = getAllowedstates(_currentStateIndex, BUBBLETOGGLE);
    cio_states.jets = getAllowedstates(_currentStateIndex, JETSTOGGLE);
    cio_states.pump = getAllowedstates(_currentStateIndex, PUMPTOGGLE);
    cio_states.heat = getAllowedstates(_currentStateIndex, HEATTOGGLE)>0;
}

void CIO_4W::regulateTemp()
{
    //this is a simple thermostat with hysteresis. Will heat until target+1 and then cool until target-1
    static uint8_t hysteresis = 0;
    if(!cio_states.heat)
    {
        cio_states.heatred = 0;
        return;
    }

    if( (cio_states.temperature + hysteresis) <= cio_states.target)
    {
        if(!cio_states.heatred)
        {
            _heat_bitmask = getHeatBitmask1(); //half power at start
            cio_states.heatred = 1;   //on
            _heater2_countdown_ms = _HEATER2_DELAY_MS;
            setHeaterStages({
                .stage1_on = true,
                .stage2_on = false,
            });
        }
        hysteresis = 0;
    }
    else
    {
        cio_states.heatred = 0; //off
        hysteresis = 1;
    }

    /*Start 2nd heater element*/
    if((_heater2_countdown_ms <= 0) && (cio_states.no_of_heater_elements_on == 2))
    {
        _heat_bitmask = getHeatBitmask1() | getHeatBitmask2();
        setHeaterStages({
            .stage1_on = true,
            .stage2_on = true,
        });
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
    int tempC = cio_states.temperature;
    int targetC;
    if(!cio_states.unit) tempC = F2C(tempC);
    if(tempC < 10)
    {
        if(!cio_states.pump)
        {
            _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
            togglestates();
        } 
        if(!cio_states.heat)
        {
            _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
            togglestates();
        } 
        targetC = 10;
        Serial.println(F("antifrz active"));
        if(cio_states.unit) cio_states.target = targetC;
        else cio_states.target = C2F(targetC);
    }
}

void CIO_4W::antiboil()
{
    /*
        Anti overtemp
    */
    int tempC = cio_states.temperature;
    if(!cio_states.unit) tempC = F2C(tempC);

    if(tempC > 41)
    {
        /*Turn ON pump, and OFF heat*/
        if(!cio_states.pump)
        {
            _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
            togglestates();
        }
        if(cio_states.heat)
        {
            _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
            togglestates();
        } 
        Serial.println(F("antiboil active"));
    }

    /*
    // Alternative solution to both antifreeze and antiboil; Hands off if temperature goes extreme.
    if((states[TEMPERATURE] < 10) || (states[TEMPERATURE] > 41))
        bwc->qCommand(SETGODMODE, 0, 0, 0);
    */
}
