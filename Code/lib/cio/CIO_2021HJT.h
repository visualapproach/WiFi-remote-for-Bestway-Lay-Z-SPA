#pragma once
#include <Arduino.h>
#include "enums.h"
#include "CIO_TYPE1.h"

class CIO_2021_HJT: public CIO_6_TYPE1
{
    /*These must be declared for the API to work*/
    public:
        String getModel(){return _MYMODEL;};

    /*internal use*/
    public:
        /*Only use for debugging*/
        uint16_t getButtonCode(Buttons button_index) override;
        Buttons getButton(uint16_t code) override;
        bool getHasjets() override {return _HASJETS;};
        bool getHasair() override {return _HASAIR;};

    /*These must be declared for the API to work*/
    public:
        const String _MYMODEL = "MALDIVES2021";
        const bool _HASJETS = true;
        const bool _HASAIR = true;

    private:
        /*Binary representation of button presses on the display, like 0x1B1B.*/
        uint16_t button_codes[11] = { 0x1B1B, 0x0100, 0x0300, 0x1212, 0x0a09, 0x1012, 0x1312, 0x0809, 0x0200, 0x0000, 0x1112 };
};
