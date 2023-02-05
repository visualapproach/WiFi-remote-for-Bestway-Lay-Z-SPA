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

    newButton = buttonCodeToIndex(newButtonCode);
    _old_button = newButton;
    return newButton;
}

void DSP_6_TYPE1::setStates(const sStates& toDspStates)
{
    _to_dsp_states = toDspStates;
    //_from_dsp_states = toDspStates;
    
    if(toDspStates.text.length())
    {
        _payload[getDGT1_IDX()] = charTo7SegmCode(toDspStates.text[0]);
        toDspStates.text.length() > 1 ? _payload[getDGT2_IDX()] = charTo7SegmCode(toDspStates.text[1]) : _payload[getDGT2_IDX()] = 1;
        toDspStates.text.length() > 2 ? _payload[getDGT3_IDX()] = charTo7SegmCode(toDspStates.text[2]) : _payload[getDGT3_IDX()] = 1;
    }
    else
    {
        _payload[getDGT1_IDX()] = charTo7SegmCode(toDspStates.char1);
        _payload[getDGT2_IDX()] = charTo7SegmCode(toDspStates.char2);
        _payload[getDGT3_IDX()] = charTo7SegmCode(toDspStates.char3);
    }

    if(toDspStates.power)
    {
        _payload[getLCK_IDX()] &= ~(1<<getLCK_BIT());
        _payload[getLCK_IDX()] |= toDspStates.locked << getLCK_BIT();

        _payload[getTMRBTNLED_IDX()] &= ~(1<<getTMRBTNLED_BIT());
        _payload[getTMRBTNLED_IDX()] |= toDspStates.timerbuttonled << getTMRBTNLED_BIT();
//
        _payload[getTMR1_IDX()] &= ~(1<<getTMR1_BIT());
        _payload[getTMR1_IDX()] |= toDspStates.timerled1 << getTMR1_BIT();

        _payload[getTMR2_IDX()] &= ~(1<<getTMR2_BIT());
        _payload[getTMR2_IDX()] |= toDspStates.timerled2 << getTMR2_BIT();
//
        _payload[getREDHTR_IDX()] &= ~(1<<getREDHTR_BIT());
        _payload[getREDHTR_IDX()] |= toDspStates.heatred << getREDHTR_BIT();

        _payload[getGRNHTR_IDX()] &= ~(1<<getGRNHTR_BIT());
        _payload[getGRNHTR_IDX()] |= toDspStates.heatgrn << getGRNHTR_BIT();

        _payload[getAIR_IDX()] &= ~(1<<getAIR_BIT());
        _payload[getAIR_IDX()] |= toDspStates.bubbles << getAIR_BIT();

        _payload[getFLT_IDX()] &= ~(1<<getFLT_BIT());
        _payload[getFLT_IDX()] |= toDspStates.pump << getFLT_BIT();

        _payload[getC_IDX()] &= ~(1<<getC_BIT());
        _payload[getC_IDX()] |= toDspStates.unit << getC_BIT();

        _payload[getF_IDX()] &= ~(1<<getF_BIT());
        _payload[getF_IDX()] |= !toDspStates.unit << getF_BIT();

        _payload[getPWR_IDX()] &= ~(1<<getPWR_BIT());
        _payload[getPWR_IDX()] |= toDspStates.power << getPWR_BIT();

        _payload[getHJT_IDX()] &= ~(1<<getHJT_BIT());
        _payload[getHJT_IDX()] |= toDspStates.jets << getHJT_BIT();
    }
    else
    {
        clearpayload();
    }
    if(toDspStates.audiofrequency)
        tone(getAUDIO(), toDspStates.audiofrequency);
    else
        noTone(getAUDIO());

    uploadPayload(toDspStates.brightness);
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
    _sendBitsToDSP(DSP_CMD1_MODE6_11_7, 8);
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

