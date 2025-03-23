#pragma once
#include <Arduino.h>
#include "enums.h"
#include "CIO_TYPE2.h"

class CIO_54149E: public CIO_6_TYPE2
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
        const String _MYMODEL = "MODEL54149E";
        const bool _HASJETS = false;
        const bool _HASAIR = true;

    private:
        /*Binary representation of button presses on the display, like 0x1B1B.*/
        uint16_t button_codes[11] = { 0, 1<<7, 1<<6, 1<<5, 1<<4, 1<<3, 1<<2, 1<<1, 1<<0, 1<<8, 1<<9 };
};
