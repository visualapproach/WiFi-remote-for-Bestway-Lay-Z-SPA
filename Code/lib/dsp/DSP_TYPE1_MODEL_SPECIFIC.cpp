#include "DSP_TYPE1_MODEL_SPECIFIC.h"

Buttons DSP_PRE2021::buttonCodeToIndex(uint16_t code)
{
    for(unsigned int i = 0; i < sizeof(_button_codes)/sizeof(uint16_t); i++){
        if(code == _button_codes[i]) return (Buttons)i;
    }
    return (Buttons)0;
}

Buttons DSP_2021_HJT::buttonCodeToIndex(uint16_t code)
{
    for(unsigned int i = 0; i < sizeof(_button_codes)/sizeof(uint16_t); i++){
        if(code == _button_codes[i]) return (Buttons)i;
    }
    return (Buttons)0;
}

Buttons DSP_2021::buttonCodeToIndex(uint16_t code)
{
    for(unsigned int i = 0; i < sizeof(_button_codes)/sizeof(uint16_t); i++){
        if(code == _button_codes[i]) return (Buttons)i;
    }
    return (Buttons)0;
}