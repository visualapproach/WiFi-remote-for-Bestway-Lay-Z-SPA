#include "DSP_4W.h"
#include "util.h"

void DSP_4W::setup(int dsp_rx, int dsp_tx, int dummy, int dummy2)
{
    _dsp_serial.begin(9600, SWSERIAL_8N1, dsp_tx, dsp_rx, false, 63);
    _dsp_serial.setTimeout(20);
    _from_dsp_states.locked_change = 0;
    _from_dsp_states.power_change = 0;
    _from_dsp_states.unit_change = 0;
    // _from_dsp_states.error = 0;
    _from_dsp_states.pressed_button = NOBTN;
    _from_dsp_states.no_of_heater_elements_on = 0;
    _from_dsp_states.godmode = 0;
}

void DSP_4W::stop()
{
    _dsp_serial.stopListening();
}

sToggles DSP_4W::getStates()
{
    int msglen = 0;
    //check if display sent a message
    msglen = 0;
    if(!_dsp_serial.available()) return _from_dsp_states;
    uint8_t tempbuffer[PAYLOADSIZE];
    msglen = _dsp_serial.readBytes(tempbuffer, PAYLOADSIZE);
    if(msglen != PAYLOADSIZE) return _from_dsp_states;

    //discard message if checksum is wrong
    uint8_t calculatedChecksum;
    calculatedChecksum = tempbuffer[1]+tempbuffer[2]+tempbuffer[3]+tempbuffer[4];
    if(tempbuffer[DSP_CHECKSUMINDEX] != calculatedChecksum)
    {
        return _from_dsp_states;
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

    if(_to_dsp_states.godmode)
    {
        /*0 = no change, 1 = toggle for these fields*/
        _from_dsp_states.bubbles_change = _bubbles != bubbles;
        _from_dsp_states.heat_change = 0;
        _from_dsp_states.jets_change = _jets != jets;
        _from_dsp_states.locked_change = 0;
        _from_dsp_states.power_change = 0;
        _from_dsp_states.pump_change = _pump != pump;
        _from_dsp_states.unit_change = 0;
        /*Absolute values*/
        _from_dsp_states.no_of_heater_elements_on = 2;
        _from_dsp_states.pressed_button = NOBTN;
        _from_dsp_states.godmode = 1;
    }
    
    /*MAYBE toggle states if godmode and user pressed buttons on display which leads to changes in states*/
    _bubbles = (_from_DSP_buf[COMMANDINDEX] & getBubblesBitmask()) > 0;
    // _from_dsp_states.heatgrn = 2;                           //unknowable in antigodmode
    // _from_dsp_states.heatred = (_from_DSP_buf[COMMANDINDEX] & (getHeatBitmask1() | getHeatBitmask2())) > 0;
    _pump = (_from_DSP_buf[COMMANDINDEX] & getPumpBitmask()) > 0;
    _jets = (_from_DSP_buf[COMMANDINDEX] & getJetsBitmask()) > 0;
    // _from_dsp_states.no_of_heater_elements_on = (_from_DSP_buf[COMMANDINDEX] & getHeatBitmask2()) > 0;

    /*This is placed here to send messages at the same rate as the dsp.*/
    _dsp_serial.write(_to_DSP_buf, PAYLOADSIZE);
    return _from_dsp_states;
}

void DSP_4W::setStates(const sStates& to_dsp_states)
{
    _to_dsp_states = to_dsp_states;
    /* If godmode - generate payload, else send rawpayload*/
    if(to_dsp_states.godmode)
    {
        generatePayload();
    }
    else
    {
        if(PAYLOADSIZE != _raw_payload_to_dsp.size()) 
        {
            return;
        }
        for(int i = 0; i < PAYLOADSIZE; i++)
        {
            _to_DSP_buf[i] = _raw_payload_to_dsp[i];
        }
    }
}

void DSP_4W::generatePayload()
{
    int tempC;
    _to_dsp_states.unit ? tempC = _to_dsp_states.temperature : tempC = F2C(_to_dsp_states.temperature);

    _to_DSP_buf[0] = B10101010;
    _to_DSP_buf[1] = 2;
    _to_DSP_buf[2] = tempC;
    _to_DSP_buf[3] = _to_dsp_states.error;
    _to_DSP_buf[4] = 0;
    _to_DSP_buf[5] = _to_DSP_buf[1]+_to_DSP_buf[2]+_to_DSP_buf[3]+_to_DSP_buf[4];
    _to_DSP_buf[6] = B10101010;
}