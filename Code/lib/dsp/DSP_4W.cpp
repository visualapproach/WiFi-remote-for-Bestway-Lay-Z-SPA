#include "DSP_4W.h"
#include "util.h"

void DSP_4W::setup(int dsp_tx, int dsp_rx, int dummy, int dummy2)
{
    HeapSelectIram ephemeral;
    _dsp_serial = new EspSoftwareSerial::UART;
    _dsp_serial->begin(9600, SWSERIAL_8N1, dsp_tx, dsp_rx, false, 24);
    _dsp_serial->setTimeout(20);
    dsp_toggles.locked_pressed = 0;
    dsp_toggles.power_change = 0;
    dsp_toggles.unit_change = 0;
    // dsp_toggles.error = 0;
    dsp_toggles.pressed_button = NOBTN;
    dsp_toggles.no_of_heater_elements_on = 2;
    dsp_toggles.godmode = 0;

    _dsp_serial->write(_to_DSP_buf, PAYLOADSIZE); //test
}

void DSP_4W::stop()
{
    _dsp_serial->stopListening();
    delete _dsp_serial;
    _dsp_serial = nullptr;
}

void DSP_4W::pause_all(bool action)
{
    if(action)
    {
        _dsp_serial->stopListening();
    } else
    {
        _dsp_serial->listen();
    }
}

void DSP_4W::updateToggles()
{
    /*We don't need a message from dsp to update these. Moved here from line81*/
    dsp_toggles.godmode = dsp_states.godmode;
    dsp_toggles.target = dsp_states.target;
    dsp_toggles.no_of_heater_elements_on = dsp_states.no_of_heater_elements_on;

    int msglen = 0;
    //check if display sent a message
    msglen = 0;
    if(!_dsp_serial->available()) return;
    uint8_t tempbuffer[PAYLOADSIZE];
    msglen = _dsp_serial->readBytes(tempbuffer, PAYLOADSIZE);
    if(msglen != PAYLOADSIZE) return;

    //discard message if checksum is wrong
    uint8_t calculatedChecksum;
    calculatedChecksum = tempbuffer[1]+tempbuffer[2]+tempbuffer[3]+tempbuffer[4];
    if(tempbuffer[DSP_CHECKSUMINDEX] != calculatedChecksum)
    {
        bad_packets_count++;
        return;
    }
    /*message is good if we get here. Continue*/

    good_packets_count++;
    /* Copy tempbuffer into _from_DSP_buf */
    for(int i = 0; i < PAYLOADSIZE; i++)
        {
            _from_DSP_buf[i] = tempbuffer[i];
            _raw_payload_from_dsp[i] = tempbuffer[i];
        }

    uint8_t bubbles = (_from_DSP_buf[COMMANDINDEX] & getBubblesBitmask()) > 0;
    uint8_t pump = (_from_DSP_buf[COMMANDINDEX] & getPumpBitmask()) > 0;
    uint8_t jets = (_from_DSP_buf[COMMANDINDEX] & getJetsBitmask()) > 0;

    /* TODO: Try without this if statement. Should keep godmode somewhat in sync with display setting. */
    if(dsp_states.godmode)
    {
        /*0 = no change, 1 = toggle for these fields*/
        dsp_toggles.bubbles_change = _bubbles != bubbles;
        dsp_toggles.heat_change = 0;
        dsp_toggles.jets_change = _jets != jets;
        dsp_toggles.locked_pressed = 0;
        dsp_toggles.power_change = 0;
        dsp_toggles.pump_change = _pump != pump;
        dsp_toggles.unit_change = 0;
        /*Absolute values*/
        dsp_toggles.pressed_button = NOBTN;
        // dsp_toggles.godmode = 1;
        // dsp_toggles.target = dsp_states.target;
    }
    
    _bubbles = bubbles;
    // dsp_toggles.heatgrn = 2;                           //unknowable in antigodmode
    // dsp_toggles.heatred = (_from_DSP_buf[COMMANDINDEX] & (getHeatBitmask1() | getHeatBitmask2())) > 0;
    _pump = pump;
    _jets = jets;
    // dsp_toggles.no_of_heater_elements_on = (_from_DSP_buf[COMMANDINDEX] & getHeatBitmask2()) > 0;

    /*This is placed here to send messages at the same rate as the dsp.*/
    // _dsp_serial->write(_to_DSP_buf, PAYLOADSIZE);
    _serialreceived = true;
    return;
}

void DSP_4W::handleStates()
{
    static unsigned long lastmillis = millis();
    int elapsedtime = millis() - lastmillis;
    lastmillis += elapsedtime;
    _time_since_last_transmission_ms += elapsedtime;
    /* If godmode - generate payload, else send rawpayload*/
    if(dsp_states.godmode)
    {
        generatePayload();
    }
    else
    {
        if(PAYLOADSIZE > _raw_payload_to_dsp.size()) 
        {
            return;
        }
        for(int i = 0; i < PAYLOADSIZE; i++)
        {
            _to_DSP_buf[i] = _raw_payload_to_dsp[i];
        }
    }

    if(_readyToTransmit || (_time_since_last_transmission_ms > _max_time_between_transmissions_ms))
    {
        _readyToTransmit = false;
        _time_since_last_transmission_ms = 0;
        _dsp_serial->write(_to_DSP_buf, PAYLOADSIZE);
        write_msg_count++;
    }

}

/* bwc can send data to cio */
bool DSP_4W::getSerialReceived()
{
    bool result = _serialreceived;
    _serialreceived = false;
    return result;
}

/* bwc is telling us that it's okay by cio to transmit */
void DSP_4W::setSerialReceived(bool txok)
{
    /* Don't forget to reset after transmitting */
    _readyToTransmit = txok;
}

void DSP_4W::generatePayload()
{
    /* 2023-06-11 edit: revert to copying unknown values instead of generating a fixed value */
    int tempC;
    dsp_states.unit ? tempC = dsp_states.temperature : tempC = F2C(dsp_states.temperature);

    _to_DSP_buf[0] = _raw_payload_to_dsp[0]; //SoF
    _to_DSP_buf[1] = _raw_payload_to_dsp[1]; //Unknown, usually 2
    _to_DSP_buf[2] = tempC;
    _to_DSP_buf[3] = dsp_states.error;
    _to_DSP_buf[4] = _raw_payload_to_dsp[4]; //Ready flags?
    _to_DSP_buf[5] = _to_DSP_buf[1]+_to_DSP_buf[2]+_to_DSP_buf[3]+_to_DSP_buf[4]; //Checksum
    _to_DSP_buf[6] = _raw_payload_to_dsp[6]; //EoF
}