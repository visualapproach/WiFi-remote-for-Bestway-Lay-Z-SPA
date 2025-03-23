#pragma once
#include <Arduino.h>
#include "enums.h"
#include "CIO_TYPE1.h"

class CIO_PRE2021 : public CIO_6_TYPE1
{
    public:
        String getModel(){return _MYMODEL;};
        uint16_t getButtonCode(Buttons button_index) override;
        Buttons getButton(uint16_t code) override;
        bool getHasjets() override {return _HASJETS;};
        bool getHasair() override {return _HASAIR;};

    private:
        const String _MYMODEL = "PRE2021";
        /*Binary representation of button presses on the display, like 0x1B1B.*/
        uint16_t button_codes[10] = { 0x1B1B, 0x0200, 0x0100, 0x0300, 0x1012, 0x1212, 0x1112, 0x1312, 0x0809, 0x0000 };
        const bool _HASJETS = false;
        const bool _HASAIR = true;

};
