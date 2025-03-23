#pragma once
#include <Arduino.h>
#include "enums.h"
#include "DSP_TYPE2.h"

class DSP_54149E : public DSP_6_TYPE2
{
    public:
        Buttons buttonCodeToIndex(uint16_t code);
        bool getHasjets() {return _HASJETS;};
        String getModel() {return _MYMODEL;};

    private:
        String _MYMODEL = "2021HJT";
        uint16_t _button_codes[11] = { 0, 1<<7, 1<<6, 1<<5, 1<<4, 1<<3, 1<<2, 1<<1, 1<<0, 1<<8, 1<<9 };
        uint16_t _oldButtonCode = _button_codes[NOBTN];
        uint16_t _prevButtonCode = _button_codes[NOBTN];
        bool _HASJETS;
};