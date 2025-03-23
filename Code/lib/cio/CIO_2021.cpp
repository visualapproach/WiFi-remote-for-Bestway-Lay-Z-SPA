#include "CIO_2021.h"

uint16_t CIO_2021::getButtonCode(Buttons index)
{
    if(index >= sizeof(button_codes)/sizeof(uint16_t)) return 0;
    return button_codes[index];
}

Buttons CIO_2021::getButton(uint16_t code)
{
    for(unsigned int i = 0; i < sizeof(button_codes)/sizeof(uint16_t); i++){
        if(_button_code == button_codes[i]) return (Buttons)i;
    }
    return (Buttons)0;
}
