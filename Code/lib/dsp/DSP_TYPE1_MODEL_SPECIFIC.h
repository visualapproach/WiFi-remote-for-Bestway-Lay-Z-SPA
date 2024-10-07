#pragma once
#include <Arduino.h>
#include "enums.h"
#include "DSP_TYPE1.h"

class DSP_PRE2021 : public DSP_6_TYPE1 
{
    public:
        DSP_PRE2021(){};
        Buttons buttonCodeToIndex(uint16_t code) override;
        bool getHasjets() {return _HASJETS;};
        String getModel() {return _MYMODEL;};

    private:
        String _MYMODEL = "PRE2021";
        uint16_t _button_codes[10] = { 0x1B1B, 0x0200, 0x0100, 0x0300, 0x1012, 0x1212, 0x1112, 0x1312, 0x0809, 0x0000 };
        bool _HASJETS = 0;
};

class DSP_2021_HJT : public DSP_6_TYPE1 
{
    public:
        String getModel() {return _MYMODEL;};
        Buttons buttonCodeToIndex(uint16_t code);
        bool getHasjets() {return _HASJETS;};

    private:
        String _MYMODEL = "2021HJT";
        uint16_t _button_codes[11] = { 0x1B1B, 0x0100, 0x0300, 0x1212, 0x0a09, 0x1012, 0x1312, 0x0809, 0x0200, 0x0000, 0x1112 };
        uint16_t _oldButtonCode = _button_codes[NOBTN];
        bool _HASJETS;
};

class DSP_2021 : public DSP_6_TYPE1 
{
    public:
        String getModel() {return _MYMODEL;};
        Buttons buttonCodeToIndex(uint16_t code);
        bool getHasjets() {return _HASJETS;};

    private:
        String _MYMODEL = "2021";
        uint16_t _button_codes[10] = { 0x1B1B, 0x0100, 0x0300, 0x1212, 0x0809, 0x1012, 0x1112, 0x1312, 0x0200, 0x0000 };
        uint16_t _oldButtonCode = _button_codes[NOBTN];
        bool _HASJETS;
};