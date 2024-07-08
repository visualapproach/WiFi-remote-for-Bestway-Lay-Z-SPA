#include "DSP_TYPE1.h"

void DSP_6_TYPE1::setup(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int dsp_audio_pin)
{
    _DATA_PIN = dsp_data_pin;
    _CLK_PIN = dsp_clk_pin;
    _CS_PIN = dsp_cs_pin;
    _AUDIO_PIN = dsp_audio_pin;
    pinMode(_CS_PIN, OUTPUT);
    pinMode(_DATA_PIN, INPUT);
    pinMode(_CLK_PIN, OUTPUT);
    pinMode(_AUDIO_PIN, OUTPUT);
    /*Active LOW*/
    digitalWrite(_CS_PIN, HIGH);
    /*Shift on falling, latch on rising edges*/
    digitalWrite(_CLK_PIN, HIGH);
    digitalWrite(_AUDIO_PIN, LOW);
}

void DSP_6_TYPE1::stop()
{
    noTone(_AUDIO_PIN);
}

uint8_t DSP_6_TYPE1::charTo7SegmCode(char c)
{
    for (unsigned int index = 0; index < sizeof(CHARS); index++) {
        if (c == CHARS[index])
        {
            return CHARCODES[index];
        }
    }
    return 0x00;  //no match, return 'space'
}

void DSP_6_TYPE1::_sendBitsToDSP(uint32_t outBits, int bitsToSend)
{
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

uint16_t DSP_6_TYPE1::_receiveBitsFromDSP()
{
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

void DSP_6_TYPE1::clearpayload()
{
    for(unsigned int i = 1; i < sizeof(_payload); i++)
    {
        _payload[i] = 0;
    }
}

Buttons DSP_6_TYPE1::getPressedButton()
{
    if(millis() - _dsp_getbutton_last_time < 90)
        return _old_button;

    uint16_t newButtonCode = 0;
    Buttons newButton;

    _dsp_getbutton_last_time = millis();
    /*Send request*/
    /*start of packet*/
    digitalWrite(_CS_PIN, LOW);
    delayMicroseconds(50);
    /*Request button presses*/
    _sendBitsToDSP(DSP_CMD2_DATAREAD, 8);
    newButtonCode = _receiveBitsFromDSP();
    /*End of packet*/
    digitalWrite(_CS_PIN, HIGH);
    delayMicroseconds(30);
    if(newButtonCode != 0xFFFF) good_packets_count++;
    newButton = buttonCodeToIndex(newButtonCode);
    _old_button = newButton;
    _raw_payload_from_dsp[0] = newButtonCode >> 8;
    _raw_payload_from_dsp[1] = newButtonCode & 0xFF;
    return newButton;
}

void DSP_6_TYPE1::handleStates()
{   
    if(text.length())
    {
        _payload[getDGT1_IDX()] = charTo7SegmCode(text[0]);
        text.length() > 1 ? _payload[getDGT2_IDX()] = charTo7SegmCode(text[1]) : _payload[getDGT2_IDX()] = 1;
        text.length() > 2 ? _payload[getDGT3_IDX()] = charTo7SegmCode(text[2]) : _payload[getDGT3_IDX()] = 1;
    }
    else
    {
        _payload[getDGT1_IDX()] = charTo7SegmCode(dsp_states.char1);
        _payload[getDGT2_IDX()] = charTo7SegmCode(dsp_states.char2);
        _payload[getDGT3_IDX()] = charTo7SegmCode(dsp_states.char3);
    }

    if(dsp_states.power)
    {
        _payload[getLCK_IDX()] &= ~(1<<getLCK_BIT());
        _payload[getLCK_IDX()] |= dsp_states.locked << getLCK_BIT();

        _payload[getTMRBTNLED_IDX()] &= ~(1<<getTMRBTNLED_BIT());
        _payload[getTMRBTNLED_IDX()] |= dsp_states.timerbuttonled << getTMRBTNLED_BIT();
//
        _payload[getTMR1_IDX()] &= ~(1<<getTMR1_BIT());
        _payload[getTMR1_IDX()] |= dsp_states.timerled1 << getTMR1_BIT();

        _payload[getTMR2_IDX()] &= ~(1<<getTMR2_BIT());
        _payload[getTMR2_IDX()] |= dsp_states.timerled2 << getTMR2_BIT();
//
        _payload[getREDHTR_IDX()] &= ~(1<<getREDHTR_BIT());
        _payload[getREDHTR_IDX()] |= dsp_states.heatred << getREDHTR_BIT();

        _payload[getGRNHTR_IDX()] &= ~(1<<getGRNHTR_BIT());
        _payload[getGRNHTR_IDX()] |= dsp_states.heatgrn << getGRNHTR_BIT();

        _payload[getAIR_IDX()] &= ~(1<<getAIR_BIT());
        _payload[getAIR_IDX()] |= dsp_states.bubbles << getAIR_BIT();

        _payload[getFLT_IDX()] &= ~(1<<getFLT_BIT());
        _payload[getFLT_IDX()] |= dsp_states.pump << getFLT_BIT();

        _payload[getC_IDX()] &= ~(1<<getC_BIT());
        _payload[getC_IDX()] |= dsp_states.unit << getC_BIT();

        _payload[getF_IDX()] &= ~(1<<getF_BIT());
        _payload[getF_IDX()] |= !dsp_states.unit << getF_BIT();

        _payload[getPWR_IDX()] &= ~(1<<getPWR_BIT());
        _payload[getPWR_IDX()] |= dsp_states.power << getPWR_BIT();

        _payload[getHJT_IDX()] &= ~(1<<getHJT_BIT());
        _payload[getHJT_IDX()] |= dsp_states.jets << getHJT_BIT();
    }
    else
    {
        clearpayload();
    }
    if(audiofrequency)
        tone(getAUDIO(), audiofrequency);
    else
        noTone(getAUDIO());

    uploadPayload(dsp_states.brightness);
}

/*Send _payload[] to display*/
void DSP_6_TYPE1::uploadPayload(uint8_t brightness)
{
    //refresh display with ~20Hz
    if(millis() -_dsp_last_refreshtime < 90) return;

    _dsp_last_refreshtime = millis();
    uint8_t enableLED = 0;
    if(brightness > 0)
    {
        enableLED = DSP_DIM_ON;
        brightness -= 1;
    }
    delayMicroseconds(30);
    digitalWrite(_CS_PIN, LOW); //start of packet
    _sendBitsToDSP(DSP_CMD1_MODE6_11_7_P05504, 8); //This should work for all models... Else change to DSP_CMD1_MODE6_11_7
    digitalWrite(_CS_PIN, HIGH); //end of packet

    delayMicroseconds(50);
    digitalWrite(_CS_PIN, LOW);//start of packet
    _sendBitsToDSP(DSP_CMD2_DATAWRITE, 8);
    digitalWrite(_CS_PIN, HIGH);//end of packet

    //_payload
    delayMicroseconds(50);
    digitalWrite(_CS_PIN, LOW);//start of packet
    for (int i = 0; i < 11; i++)
    _sendBitsToDSP(_payload[i], 8);
    digitalWrite(_CS_PIN, HIGH);//end of packet

    delayMicroseconds(50);
    digitalWrite(_CS_PIN, LOW);//start of packet
    _sendBitsToDSP(DSP_DIM_BASE|enableLED|brightness, 8);
    digitalWrite(_CS_PIN, HIGH);//end of packet
    delayMicroseconds(50);
}

