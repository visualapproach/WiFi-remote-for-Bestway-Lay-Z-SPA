#pragma once
#include <Arduino.h>
#include "enums.h"
#include "CIO_4W.h"

class CIO_54123: public CIO_4W
{
    public:
        String getModel(){return _MYMODEL;};
        uint8_t getPumpBitmask() override {return PUMPBITMASK;} ;
        uint8_t getBubblesBitmask() override {return BUBBLESBITMASK;} ;
        uint8_t getJetsBitmask() override {return JETSBITMASK;} ;
        uint8_t getHeatBitmask1() override {return HEATBITMASK1;} ;
        uint8_t getHeatBitmask2() override {return HEATBITMASK2;} ;
        uint8_t getPowerBitmask() override {return POWERBITMASK;} ;
        uint8_t getJumptable(int row, int col) override {return JUMPTABLE[row][col];};
        uint8_t getAllowedstates(int row, int col) override {return ALLOWEDSTATES[row][col];};
        bool getHasjets() override {return _HASJETS;};
        bool getHasair() override {return _HASAIR;};

    private:
        const String _MYMODEL = "NO54123";
        //WARNING: THIS DEVICE HAS DIFFERENT PINOUTS!!! CHECK BEFORE USING
        //#NO54112 - @jenswalit: "this is the Version Vegas 4Pin Pump 1->5V+ 2->Data 3->Data 4->5V-"
        //what row in allowedstates to go to when pressing Bubbles, Jets, Pump, Heat (columns in that order)
        //Example: We are in state zero (first row). If we press Bubbles (first column) then there is a 6
        //meaning current state (row) is now 6. According to ALLOWEDSTATES table, we turn on Bubbles and keep
        //everything else off. (1,0,0,0)
        const uint8_t JUMPTABLE[4][4] = {
        {1,0,2,3},
        {0,1,2,3},
        {1,2,0,3},
        {1,3,0,2}
        };
        //Bubbles, Jets, Pump, Heat
        const uint8_t ALLOWEDSTATES[4][4] = {
        {0,0,0,0},
        {1,0,0,0},
        {0,0,1,0},
        {0,0,1,2}  //the "2" means both heater elements
        };

        const uint8_t PUMPBITMASK =    B00010000;  //1 << 4;
        const uint8_t BUBBLESBITMASK =  B00100000;  //1 << 5;
        const uint8_t JETSBITMASK =   B00000000;  //0;  //no jets on this machine.
        const uint8_t HEATBITMASK1 =   B00000010;  //(1 << 1) "stage 1"
        const uint8_t HEATBITMASK2 =   B00001000;  //(1 << 3) "stage 2" (thanks @dietmar-1 for testing and reporting this)
        const uint8_t POWERBITMASK =   B00000001;  //1;
        const bool _HASJETS = false;
        const bool _HASAIR = true;
};

class CIO_54138: public CIO_4W
{
    public:
        String getModel(){return _MYMODEL;};
        uint8_t getPumpBitmask() override {return PUMPBITMASK;} ;
        uint8_t getBubblesBitmask() override {return BUBBLESBITMASK;} ;
        uint8_t getJetsBitmask() override {return JETSBITMASK;} ;
        uint8_t getHeatBitmask1() override {return HEATBITMASK1;} ;
        uint8_t getHeatBitmask2() override {return HEATBITMASK2;} ;
        uint8_t getPowerBitmask() override {return POWERBITMASK;} ;
        uint8_t getJumptable(int row, int col) override {return JUMPTABLE[row][col];};
        uint8_t getAllowedstates(int row, int col) override {return ALLOWEDSTATES[row][col];};
        bool getHasjets() override {return _HASJETS;};
        bool getHasair() override {return _HASAIR;};

    private:
        const String _MYMODEL = "NO54138";
        //what row in allowedstates to go to when pressing Bubbles, Jets, Pump, Heat (columns in that order)
        //Example: We are in state zero (first row). If we press Bubbles (first column) then there is a 6
        //meaning current state (row) is now 6. According to ALLOWEDSTATES table, we turn on Bubbles and keep
        //everything else off. (1,0,0,0)
        const uint8_t JUMPTABLE[5][4] = {
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
        const uint8_t HEATBITMASK1 =   B00110000;  //48 heater stage 1 = 50%
        const uint8_t HEATBITMASK2 =   B01000000;  //64 heater stage 2 = 100%
        //lines below should be tested. It would be consistent with 54173 model.
        //If heating is slow this is probably the cause but I don't want to change it before someone tests it.
        //const uint8_t HEATBITMASK1 =   B00000000;  //00  heater stage 1 = off
        //const uint8_t HEATBITMASK2 =   B00110000;  //48  heater stage 2 = on
        const uint8_t POWERBITMASK =   B10000000;  //128
        const bool _HASJETS = true;
        const bool _HASAIR = true;
};

class CIO_54144: public CIO_4W
{
    public:
        String getModel(){return _MYMODEL;};
        uint8_t getPumpBitmask() override {return PUMPBITMASK;} ;
        uint8_t getBubblesBitmask() override {return BUBBLESBITMASK;} ;
        uint8_t getJetsBitmask() override {return JETSBITMASK;} ;
        uint8_t getHeatBitmask1() override {return HEATBITMASK1;} ;
        uint8_t getHeatBitmask2() override {return HEATBITMASK2;} ;
        uint8_t getPowerBitmask() override {return POWERBITMASK;} ;
        uint8_t getJumptable(int row, int col) override {return JUMPTABLE[row][col];};
        uint8_t getAllowedstates(int row, int col) override {return ALLOWEDSTATES[row][col];};
        bool getHasjets() override {return _HASJETS;};
        bool getHasair() override {return _HASAIR;};

    private:
        const String _MYMODEL = "NO54144";
        //what row in allowedstates to go to when pressing Bubbles, Jets, Pump, Heat (columns in that order)
        //Example: We are in state zero (first row). If we press Bubbles (first column) then there is a 6
        //meaning current state (row) is now 6. According to ALLOWEDSTATES table, we turn on Bubbles and keep
        //everything else off. (1,0,0,0)
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
        const bool _HASJETS = true;
        const bool _HASAIR = false;
};

class CIO_54154: public CIO_4W
{
    public:
        String getModel(){return _MYMODEL;};
        uint8_t getPumpBitmask() override {return PUMPBITMASK;} ;
        uint8_t getBubblesBitmask() override {return BUBBLESBITMASK;} ;
        uint8_t getJetsBitmask() override {return JETSBITMASK;} ;
        uint8_t getHeatBitmask1() override {return HEATBITMASK1;} ;
        uint8_t getHeatBitmask2() override {return HEATBITMASK2;} ;
        uint8_t getPowerBitmask() override {return POWERBITMASK;} ;
        uint8_t getJumptable(int row, int col) override {return JUMPTABLE[row][col];};
        uint8_t getAllowedstates(int row, int col) override {return ALLOWEDSTATES[row][col];};
        bool getHasjets() override {return _HASJETS;};
        bool getHasair() override {return _HASAIR;};

    private:
        const String _MYMODEL = "NO54154";
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
        const bool _HASJETS = false;
        const bool _HASAIR = true;
};

class CIO_54173: public CIO_4W
{
    public:
        String getModel(){return _MYMODEL;}
        uint8_t getPumpBitmask() override {return PUMPBITMASK;} 
        uint8_t getBubblesBitmask() override {return BUBBLESBITMASK;} 
        uint8_t getJetsBitmask() override {return JETSBITMASK;} 
        uint8_t getHeatBitmask1() override {return HEATBITMASK1;} 
        uint8_t getHeatBitmask2() override {return HEATBITMASK2;} 
        uint8_t getPowerBitmask() override {return POWERBITMASK;} 
        uint8_t getJumptable(int row, int col) override {return JUMPTABLE[row][col];}
        uint8_t getAllowedstates(int row, int col) override {return ALLOWEDSTATES[row][col];}
        bool getHasjets() override {return _HASJETS;}
        bool getHasair() override {return _HASAIR;}

    private:
        const String _MYMODEL = "NO54173";
        //what row in allowedstates to go to when pressing Bubbles, Jets, Pump, Heat (columns in that order)
        //Example: We are in state zero (first row). If we press Bubbles (first column) then there is a 6
        //meaning current state (row) is now 6. According to ALLOWEDSTATES table, we turn on Bubbles and keep
        //everything else off. (1,0,0,0)
        const uint8_t JUMPTABLE[8][4] = {
        {6,4,1,3},
        {7,4,0,3},
        {3,5,6,7},
        {2,4,0,1},
        {5,0,1,3},
        {4,6,1,3},
        {0,5,7,2},
        {1,5,6,2}
        };
        //Bubbles, Jets, Pump, Heat
        const uint8_t ALLOWEDSTATES[8][4] = {
        {0,0,0,0},
        {0,0,1,0},
        {1,0,1,1},
        {0,0,1,2},  //the "2" means both heater elements
        {0,1,0,0},
        {1,1,0,0},
        {1,0,0,0},
        {1,0,1,0}
        };

        const uint8_t PUMPBITMASK =   B00000101;  //5
        const uint8_t BUBBLESBITMASK =   B00000010;  //2
        const uint8_t JETSBITMASK =   B00001000;  //8
        const uint8_t HEATBITMASK1 =   B00110000;  //48  heater stage 1 = 50%
        const uint8_t HEATBITMASK2 =   B01000000;  //64  heater stage 2 = 100%
        const uint8_t POWERBITMASK =   B10000000;  //128
        const bool _HASJETS = true;
        const bool _HASAIR = true;
};