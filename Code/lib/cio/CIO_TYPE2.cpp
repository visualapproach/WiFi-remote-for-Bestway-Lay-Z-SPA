#include "CIO_TYPE2.h"
#include "ports.h"

CIO_6_TYPE2 *pointerToClassCIO62;

static void IRAM_ATTR isr_LEDdatapin(void) {
    pointerToClassCIO62->LED_Handler();
}
static void IRAM_ATTR isr_clk_type2(void) {
    pointerToClassCIO62->clkHandler();
}

CIO_6_TYPE2::CIO_6_TYPE2()
{
    _byte_count = 0;
    _bit_count = 0;
    _received_byte = 0;
    _send_bit = 8;
    _brightness = 7;
    _received_cmd = 0;
}

void CIO_6_TYPE2::setup(int cio_td_pin, int cio_clk_pin, int cio_ld_pin)
{
    pointerToClassCIO62 = this;
    _CIO_TD_PIN = cio_td_pin;
    _CIO_CLK_PIN = cio_clk_pin;
    _CIO_LD_PIN = cio_ld_pin;
    _button_code = getButtonCode(NOBTN);
    pinMode(_CIO_LD_PIN, INPUT);
    pinMode(_CIO_TD_PIN, OUTPUT);
    pinMode(_CIO_CLK_PIN, INPUT);
    digitalWrite(_CIO_TD_PIN, 1); //idle high
    attachInterrupt(digitalPinToInterrupt(_CIO_LD_PIN), isr_LEDdatapin, CHANGE);
    attachInterrupt(digitalPinToInterrupt(_CIO_CLK_PIN), isr_clk_type2, CHANGE); //Write on falling edge and read on rising edge
}

void CIO_6_TYPE2::stop(){
    detachInterrupt(digitalPinToInterrupt(_CIO_LD_PIN));
    detachInterrupt(digitalPinToInterrupt(_CIO_CLK_PIN));
}

void CIO_6_TYPE2::pause_all(bool action)
{
    if(action)
    {
        /*pause*/
        detachInterrupt(digitalPinToInterrupt(_CIO_LD_PIN));
        detachInterrupt(digitalPinToInterrupt(_CIO_CLK_PIN));
    } else
    {
        /*resume*/
        attachInterrupt(digitalPinToInterrupt(_CIO_LD_PIN), isr_LEDdatapin, CHANGE);
        attachInterrupt(digitalPinToInterrupt(_CIO_CLK_PIN), isr_clk_type2, CHANGE); //Write on falling edge and read on rising edge
    }
}

void CIO_6_TYPE2::updateStates()
{
    /*update all states*/

    //newdata is true when a data packet has arrived from cio
    if(!_new_packet_available) return;
    _new_packet_available = false;
    // if(_packet_error)
    // {
    //     _packet_error = false;
    //     return cio_states;
    // }
    static uint32_t buttonReleaseTime;
    enum Readmode: int {readtemperature, uncertain, readtarget};
    static Readmode capturePhase = readtemperature;

    // //require two consecutive messages to be equal before registering
    // static uint8_t prev_checksum = 0;
    // uint8_t checksum = 0;
    // for(int i = 0; i < 11; i++){
    //     checksum += _payload[i];
    // }
    // if(checksum != prev_checksum) {
    //     prev_checksum = checksum;
    //     return cio_states;
    // }

    //copy private array to public array
    for(unsigned int i = 0; i < sizeof(_payload); i++){
        _raw_payload_from_cio[i] = _payload[i];
    }

    good_packets_count++;
    brightness = _brightness & 7; //extract only the brightness bits (0-7)
    cio_states.locked = (_raw_payload_from_cio[LCK_IDX] & (1 << LCK_BIT)) > 0;
    cio_states.power = 1; //(_raw_payload_from_cio[PWR_IDX] & (1 << PWR_BIT)) > 0;
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

//CIO comm
//packet start/stop
void IRAM_ATTR CIO_6_TYPE2::LED_Handler(void) {
    //Check START/END condition: _LD_PIN change when _CLK_PIN is high.
    if (READ_PERI_REG(PIN_IN) & (1 << _CIO_CLK_PIN)) {
        _byte_count = 0;
        _bit_count = 0;
        _received_cmd = 0;
        _new_packet_available = (READ_PERI_REG(PIN_IN) & (1 << _CIO_LD_PIN)) > 0;
        _packet_transm_active = !_new_packet_available;
    }
}

void IRAM_ATTR CIO_6_TYPE2::clkHandler(void) {
    //read data on _cio_ld_pin and write to _cio_td_pin (LSBF)

    uint16_t td_bitnumber = _bit_count % 10;
    uint16_t ld_bitnumber = _bit_count % 8;
    uint16_t buttonwrapper = (B11111110 << 8) | (_button_code<<1); //startbit @ bit0, stopbit @ bit9

    //rising or falling edge?
    bool risingedge = READ_PERI_REG(PIN_IN) & (1 << _CIO_CLK_PIN);
    if(risingedge){
        //clk rising edge
        _byte_count = _bit_count / 8;
        if(_byte_count == 0){
        _received_cmd |= ((READ_PERI_REG(PIN_IN) & (1 << _CIO_LD_PIN))>0) << ld_bitnumber;
        }
        else if( (_byte_count<6) && (_received_cmd == CMD2) ){ //only write to _raw_payload_from_cio after CMD2. Also protect from buffer overflow
        //overwrite the old _raw_payload_from_cio bit with new bit
        _payload[_byte_count-1] = (_payload[_byte_count-1] & ~(1 << ld_bitnumber)) | ((READ_PERI_REG(PIN_IN) & (1 << _CIO_LD_PIN))>0) << ld_bitnumber;
        }
        //store brightness in cio local variable. It is not used, but put here in case we want to obey the pump.
        if(_bit_count == 7 && (_received_cmd & B11000000) == B10000000) _brightness = _received_cmd;
        _bit_count++;
    } else {
        //clk falling edge
        //first and last bit is a dummy start/stop bit (0/1), then 8 data bits in btwn
        if (buttonwrapper & (1 << td_bitnumber)) {
        WRITE_PERI_REG( PIN_OUT_SET, 1 << _CIO_TD_PIN);
        } else {
        WRITE_PERI_REG( PIN_OUT_CLEAR, 1 << _CIO_TD_PIN);
        }
    }
}

char CIO_6_TYPE2::_getChar(uint8_t value)
{
    for (unsigned int index = 0; index < sizeof(CHARCODES); index++) {
        if (value == CHARCODES[index]) {
        return CHARS[index];
        }
    }
    return '*';
}