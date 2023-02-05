#pragma once
#include <Arduino.h>
#include "enums.h"
#include "DSP_4W.h"

class DSP_54154 : public DSP_4W
{
    public:
        virtual ~DSP_54154(){};
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
        const String _MYMODEL = "NO54154";
        const bool _HASJETS = false;
        const bool _HASAIR = true;
        //WARNING: THIS DEVICE HAS DIFFERENT PINOUTS!!! CHECK BEFORE USING
        //Requested by @chunkysteveo
        const uint8_t JUMPTABLE[6][4] = {
        {3,0,1,2},
        {4,1,0,2},
        {5,2,0,1},
        {0,3,4,5},
        {1,4,3,5},
        {2,5,3,4}
        };
        //Bubbles, Jets, Pump, Heat
        const uint8_t ALLOWEDSTATES[6][4] = {
        {0,0,0,0},
        {0,0,1,0},
        {0,0,1,2},
        {1,0,0,0},  //the "2" means both heater elements
        {1,0,1,0},
        {1,0,1,1}
        };

        const uint8_t PUMPBITMASK =    B00010000;  //1 << 4;
        const uint8_t BUBBLESBITMASK =  B00100000;  //1 << 5;
        const uint8_t JETSBITMASK =   B00000000;  //0;  //no jets on this machine.
        const uint8_t HEATBITMASK1 =   B00000010;  //(1 << 1) "stage 1"
        const uint8_t HEATBITMASK2 =   B00001000;  //(1 << 3) "stage 2" (thanks @dietmar-1 for testing and reporting this)
        const uint8_t POWERBITMASK =   B00000001;  //1;

};