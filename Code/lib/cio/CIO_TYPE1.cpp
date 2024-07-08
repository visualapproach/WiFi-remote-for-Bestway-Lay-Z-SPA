#include "CIO_TYPE1.h"
#include "ports.h"

CIO_6_TYPE1 *pointerToClassCIO61;

static void IRAM_ATTR isr_cs_type1(void) {
    pointerToClassCIO61->isr_packetHandler();
}
static void IRAM_ATTR isr_clk_type1(void) {
    pointerToClassCIO61->isr_clkHandler();
}

CIO_6_TYPE1::CIO_6_TYPE1()
{
    _byte_count = 0;
    _bit_count = 0;
    _data_is_output = false;
    _received_byte = 0;
    _CIO_cmd_matches = 0;
    _new_packet_available = false;
    _send_bit = 8;
    _brightness = 7;
    _packet_error = 0;
}

void CIO_6_TYPE1::setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin)
{
    pointerToClassCIO61 = this;
    _DATA_PIN = cio_data_pin;
    _CLK_PIN = cio_clk_pin;
    _CS_PIN = cio_cs_pin;
    _button_code = getButtonCode(NOBTN);
    pinMode(_CS_PIN, INPUT);
    pinMode(_DATA_PIN, INPUT);
    pinMode(_CLK_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(_CS_PIN), isr_cs_type1, CHANGE);
    //Write on falling edge and read on rising edge
    attachInterrupt(digitalPinToInterrupt(_CLK_PIN), isr_clk_type1, CHANGE);
}

void CIO_6_TYPE1::stop(){
    detachInterrupt(digitalPinToInterrupt(_CS_PIN));
    detachInterrupt(digitalPinToInterrupt(_CLK_PIN));
}

void CIO_6_TYPE1::pause_all(bool action)
{
    if(action)
    {
        /*pause*/
        detachInterrupt(digitalPinToInterrupt(_CS_PIN));
        detachInterrupt(digitalPinToInterrupt(_CLK_PIN));
    } else
    {
        /*resume*/
        attachInterrupt(digitalPinToInterrupt(_CS_PIN), isr_cs_type1, CHANGE);
        attachInterrupt(digitalPinToInterrupt(_CLK_PIN), isr_clk_type1, CHANGE);
    }
}

void CIO_6_TYPE1::updateStates()
{
    /*update all states*/

    //_new_packet_available is true when a data packet has arrived from cio
    if(!_new_packet_available) return;
    _new_packet_available = false;
    if(_packet_error)
    {
        bad_packets_count++;
        packet_error = _packet_error;
        _packet_error = 0;
        return;
    }
    static uint32_t buttonReleaseTime;
    enum Readmode: int {readtemperature, uncertain, readtarget};
    static Readmode capturePhase = readtemperature;

    //require two consecutive messages to be equal before registering
    #if FILTER_6W_SPIKES==1
    static uint8_t prev_checksum = 0;
    uint8_t checksum = 0;
    for(int i = 0; i < 11; i++){
        checksum += _payload[i];
    }
    if(checksum != prev_checksum) {
        prev_checksum = checksum;
        return;
    }
    #endif
    //copy private array to public array
    for(unsigned int i = 0; i < sizeof(_payload); i++){
        _raw_payload_from_cio[i] = _payload[i];
    }
    good_packets_count++;
    brightness = _brightness & 7; //extract only the brightness bits (0-7)
    cio_states.locked = (_raw_payload_from_cio[LCK_IDX] & (1 << LCK_BIT)) > 0;
    cio_states.power = (_raw_payload_from_cio[PWR_IDX] & (1 << PWR_BIT)) > 0;
    /*If both leds are out, don't change (When TIMER is pressed)*/
    if(_raw_payload_from_cio[C_IDX] & (1 << C_BIT) || _raw_payload_from_cio[F_IDX] & (1 << F_BIT))
        cio_states.unit = (_raw_payload_from_cio[C_IDX] & (1 << C_BIT)) > 0;
    cio_states.bubbles = (_raw_payload_from_cio[AIR_IDX] & (1 << AIR_BIT)) > 0;
    cio_states.heatgrn = (_raw_payload_from_cio[GRNHTR_IDX] & (1 << GRNHTR_BIT)) > 0;
    cio_states.heatred = (_raw_payload_from_cio[REDHTR_IDX] & (1 << REDHTR_BIT)) > 0;
    cio_states.timerled1 = (_raw_payload_from_cio[TMR1_IDX] & (1 << TMR1_BIT)) > 0;
    cio_states.timerled2 = (_raw_payload_from_cio[TMR2_IDX] & (1 << TMR2_BIT)) > 0;
    cio_states.timerbuttonled = (_raw_payload_from_cio[TMRBTNLED_IDX] & (1 << TMRBTNLED_BIT)) > 0;
    cio_states.heat = cio_states.heatgrn || cio_states.heatred;
    cio_states.pump = (_raw_payload_from_cio[FLT_IDX] & (1 << FLT_BIT)) > 0;
    cio_states.char1 = (uint8_t)_getChar(_raw_payload_from_cio[DGT1_IDX]);
    cio_states.char2 = (uint8_t)_getChar(_raw_payload_from_cio[DGT2_IDX]);
    cio_states.char3 = (uint8_t)_getChar(_raw_payload_from_cio[DGT3_IDX]);
    if(getHasjets()) 
        cio_states.jets = (_raw_payload_from_cio[HJT_IDX] & (1 << HJT_BIT)) > 0;
    else 
        cio_states.jets = 0;

    //Determine if display is showing target temp or actual temp or anything else.
    //Unreadable characters - exit
    if(cio_states.char1 == '*' || cio_states.char2 == '*' || cio_states.char3 == '*') return;
    //Error or user plays with timer button - exit (error notification can be dealt with in main.cpp or elsewhere)
    if(cio_states.char1 == 'e')
    {
        String errornumber;
        errornumber = (char)cio_states.char2;
        errornumber += (char)cio_states.char3;
        cio_states.error = (uint8_t)(errornumber.toInt());
        return;
    } 
    if(cio_states.char3 == 'H' || cio_states.char3 == ' ') return;

    /* Reset error state */
    cio_states.error = 0;

    //capture TARGET after UP/DOWN has been pressed...
    if ((_button_code == getButtonCode(UP)) || (_button_code == getButtonCode(DOWN)))
    {
        buttonReleaseTime = millis(); //updated as long as buttons are pressed
        if(cio_states.power && !cio_states.locked) capturePhase = readtarget;
    }

    //Stop expecting target temp after timeout
    if((millis()-buttonReleaseTime) > 2000) capturePhase = uncertain;
    if((millis()-buttonReleaseTime) > 6000) capturePhase = readtemperature;
    //convert text on display to a value if the chars are recognized
    String tempstring = String((char)cio_states.char1)+String((char)cio_states.char2)+String((char)cio_states.char3);
    uint8_t parsedValue = tempstring.toInt();
    //capture target temperature only if showing plausible values (not blank screen while blinking)
    if( (capturePhase == readtarget) && (parsedValue > 19) ) 
    {
        cio_states.target = parsedValue;
    }
    //wait 6 seconds after UP/DOWN is released to be sure that actual temp is shown
    if(capturePhase == readtemperature)
    {
        if(cio_states.temperature != parsedValue)
        {
        cio_states.temperature = parsedValue;
        }
    }

    return;
}


/*End Of Packet.*/
void IRAM_ATTR CIO_6_TYPE1::eopHandler(void) {
//process latest data and enter corresponding mode (like listen for DSP_STS or send BTN_OUT)
//pinMode(_DATA_PIN, INPUT);
    WRITE_PERI_REG( PIN_DIR_INPUT, 1 << _DATA_PIN);
    if(_byte_count != 11 && _byte_count != 0) _packet_error |= 2;
    if(_bit_count != 0) _packet_error |= 1;
    _byte_count = 0;
    _bit_count = 0;
    uint8_t msg = _received_byte;

    switch (msg) {
        case DSP_CMD1_MODE6_11_7:
            _CIO_cmd_matches = 1;
            break;
        case DSP_CMD1_MODE6_11_7_P05504:
            _CIO_cmd_matches = 1;
            break;
        case DSP_CMD2_DATAWRITE:
            if (_CIO_cmd_matches == 1) {
                _CIO_cmd_matches = 2;
            } else {
                //reset - DSP_CMD1_MODE6_11_7 must be followed by DSP_CMD2_DATAWRITE to activate command
                _CIO_cmd_matches = 0;
            }
            break;
        default:
            if (_CIO_cmd_matches == 3) {
                _brightness = msg;
                _CIO_cmd_matches = 0;
                _new_packet_available = true;
            }
            if (_CIO_cmd_matches == 2) {
                _CIO_cmd_matches = 3;
            }
            break;
    }
}

//CIO comm
//packet start
//arduino core 3.0.1+ should work with digitalWrite() now.
//CS line toggles
void IRAM_ATTR CIO_6_TYPE1::isr_packetHandler() {
    #ifdef ESP8266
    if ((READ_PERI_REG(PIN_IN) & (1 << _CS_PIN))) {
    #else
    if(digitalRead(_CS_PIN)) {
    #endif
        //end of packet (CS is idle at high)
        _packet_transm_active = false;
        _data_is_output = false;
        eopHandler();
    }
    else {
        //packet start (CS active low)
        _packet_transm_active = true;
    }
}

//CIO comm
//Read incoming bits, and take action after a complete byte
//CLK line toggles
void IRAM_ATTR CIO_6_TYPE1::isr_clkHandler(void) {
    //sanity check on clock signal

    if (!_packet_transm_active) return;
    //CS line is active, so send/receive bits on DATA line

    #ifdef ESP8266
    bool clockstate = READ_PERI_REG(PIN_IN) & (1 << _CLK_PIN);
    #else
    bool clockstate = digitalRead(_CLK_PIN);
    #endif
    //shift out bits on low clock (falling edge)
    if (!clockstate & _data_is_output) {
        //send BTN_OUT
        if (_button_code & (1 << _send_bit)) {
            //digitalWrite(_DATA_PIN, HIGH);
            #ifdef ESP8266
            WRITE_PERI_REG( PIN_OUT_SET, 1 << _DATA_PIN);
            #else
            digitalWrite(_DATA_PIN, 1);
            #endif
        }
        else {
            //digitalWrite(_DATA_PIN, LOW);
            #ifdef ESP8266
            WRITE_PERI_REG( PIN_OUT_CLEAR, 1 << _DATA_PIN);
            #else
            digitalWrite(_DATA_PIN, 0);
            #endif
        }
        _send_bit++;
        if(_send_bit > 15) _send_bit = 0;
    }

    //read bits on high clock (rising edge)
    if (clockstate & !_data_is_output) {
        //read data pin to a byte
        //_received_byte = (_received_byte << 1) | digitalRead(_DATA_PIN);
        //_received_byte = (_received_byte << 1) | ( ( (READ_PERI_REG(PIN_IN) & (1 << _DATA_PIN)) ) > 0);
        //_received_byte = (_received_byte >> 1) | digitalRead(_DATA_PIN) << 7;
    #ifdef ESP8266
        _received_byte = (_received_byte >> 1) | ( ( (READ_PERI_REG(PIN_IN) & (1 << _DATA_PIN)) ) > 0) << 7;
    #else
        _received_byte = (_received_byte >> 1) | digitalRead(_DATA_PIN) << 7;
    #endif
        _bit_count++;
        if (_bit_count == 8) {
            _bit_count = 0;
            //We have received the header for 11 data bytes to come
            if (_CIO_cmd_matches == 2)
            {
                if(_byte_count < 11)
                {
                _payload[_byte_count] = _received_byte;
                _byte_count++;
                }
                else
                {
                        _packet_error |= 4; //too many bytes received
                }
            }
            //We have received request for button pressed
            else if (_received_byte == DSP_CMD2_DATAREAD)
            {
                _send_bit = 8;
                _data_is_output = true;
                //pinMode(_DATA_PIN, OUTPUT);
            #ifdef ESP8266
                WRITE_PERI_REG( PIN_DIR_OUTPUT, 1 << _DATA_PIN);
            #else
                pinMode(_DATA_PIN, OUTPUT);
            #endif
            }
            CIO_CMD_LOG[CIO_CMD_LOG_index++] = (uint8_t)_received_byte;
            CIO_CMD_LOG_index %= 64;
        }
    }
}

char CIO_6_TYPE1::_getChar(uint8_t value)
{
    for (unsigned int index = 0; index < sizeof(CHARCODES); index++) {
        /* Mask out the LSB due to some pumps differ and that bit is not used anyway. */
        if ((value & 0xFE) == (CHARCODES[index] & 0xFE)) {
        return CHARS[index];
        }
    }
    return '*';
}