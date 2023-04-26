#pragma once
#include <Arduino.h>
#include "enums.h"
#include "DSP_4W.h"

class DSP_54144 : public DSP_4W
{
    public:
        virtual ~DSP_54144(){};
        uint8_t getPumpBitmask() override {return PUMPBITMASK;} ;
        uint8_t getBubblesBitmask() override {return BUBBLESBITMASK;} ;
        uint8_t getJetsBitmask() override {return JETSBITMASK;} ;
        uint8_t getHeatBitmask1() override {return HEATBITMASK1;} ;
        uint8_t getHeatBitmask2() override {return HEATBITMASK2;} ;
        uint8_t getPowerBitmask() override {return POWERBITMASK;} ;
        uint8_t getJumptable(int row, int col) override {return JUMPTABLE[row][col];};
        uint8_t getAllowedstates(int row, int col) override {return ALLOWEDSTATES[row][col];};
        String getModel(){return _MYMODEL;};
        bool getHasjets() override {return _HASJETS;};
        bool getHasair() override {return _HASAIR;};

    private:
        const String _MYMODEL = "NO54144";
        const bool _HASJETS = true;
        const bool _HASAIR = false;
        //WARNING: THIS DEVICE HAS DIFFERENT PINOUTS!!! CHECK BEFORE USING
        //Requested by @chunkysteveo
        const uint8_t JUMPTABLE[5][4] = {
        // b,j,p,h
        {1,2,3,4},
        {0,2,3,4},
        {1,0,3,4},
        {1,2,0,4},
        {1,2,0,3}
        };
        //Bubbles, Jets, Pump, Heat
        const uint8_t ALLOWEDSTATES[5][4] = {
        {0,0,0,0},
        {1,0,0,0},
        {0,1,0,0},
        {0,0,1,0},
        {0,0,1,2}  //the "2" means both heater elements
        };

        const uint8_t PUMPBITMASK =   B00000101;  //5
        const uint8_t BUBBLESBITMASK =   B00000010;  //2
        const uint8_t JETSBITMASK =   B00001000;  //8
        // const uint8_t HEATBITMASK1 =   B00000000;  //0    heater stage 1 = off
        // const uint8_t HEATBITMASK2 =   B00110000;  //48  heater stage 2 = on
        //lines below should be tested. It would be consistent with 54173 model.
        //If heating is slow this is probably the cause but I don't want to change it before someone tests it.
        const uint8_t HEATBITMASK1 =   B00110000;  //48  heater stage 1 = 50%
        const uint8_t HEATBITMASK2 =   B01000000;  //64  heater stage 2 = 100%
        const uint8_t POWERBITMASK =   B10000000;  //128

};