#ifndef BWC_8266_4W_GLOBALS_H
#define BWC_8266_4W_GLOBALS_H
#include <model.h>
//Commands to CIO
//  130 = bubbles
//  133 = filter pump
//  136 = Jets
//  138 = Jets and bubbles
//  181 = heating both elements with filter pump

//only declarations here please! Definitions belong in the cpp-file



enum States: byte
{
  LOCKEDSTATE,
  POWERSTATE,
  UNITSTATE,
  BUBBLESSTATE,
  HEATGRNSTATE,
  HEATREDSTATE,
  HEATSTATE,
  PUMPSTATE,
  TEMPERATURE,
  TARGET,
  CHAR1,
  CHAR2,
  CHAR3,
  JETSSTATE,
  ERROR
};


enum Commands: byte
{
  SETTARGET,
  SETUNIT,
  SETBUBBLES,
  SETHEATER,
  SETPUMP,
  RESETQ,
  REBOOTESP,
  GETTARGET,
  RESETTIMES,
  RESETCLTIMER,
  RESETFTIMER,
  SETJETS,
  SETGODMODE,
  SETFULLPOWER
};

enum ToggleButtons: byte
{
  BUBBLETOGGLE,
  JETSTOGGLE,
  PUMPTOGGLE,
  HEATTOGGLE
};

const int MAXCOMMANDS = 11;
const int MAXBUTTONS = 33;

const int IDLE_WATTS = 2;
const int PUMP_WATTS = 40;
const int HEATER_WATTS = 1900;
const int BUBBLES_WATTS = 800;
const int JETS_WATTS = 800;

#ifdef NO54173
/*combination matrix
  Heater1  Heater2  Bubbles  Jets  Pump
H1  -    1    1    0    1
H2      -    0    0    1
B          -    1    1
J              -    0
P
ten boolean values above stored in a word "comb_matrix"
Heater and pump values are still partly hardcoded in combination with the heatbitmasks.
Do not change the heatbitmasks below unless you really know what you are doing.
*/
// const uint16_t COMB_MATRIX = 0x3CF;  // B0000001111001111;

//what row in allowedstates to go to when pressing Bubbles, Jets, Pump, Heat (columns in that order)
//Example: We are in state zero (first row). If we press Bubbles (first column) then there is a 6
//meaning current state (row) is now 6. According to ALLOWEDSTATES table, we turn on Bubbles and keep
//everything else off. (1,0,0,0)
const uint8_t JUMPTABLE[][4] = {
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
const uint8_t ALLOWEDSTATES[][4] = {
  {0,0,0,0},
  {0,0,1,0},
  {1,0,1,1},
  {0,0,1,2},  //the "2" means both heater elements
  {0,1,0,0},
  {1,1,0,0},
  {1,0,0,0},
  {1,0,1,0}
};

//cio
const uint8_t TEMPINDEX = 2;
const uint8_t ERRORINDEX = 3;
const uint8_t CIO_CHECKSUMINDEX = 5;
//dsp
const uint8_t COMMANDINDEX = 2;
const uint8_t DSP_CHECKSUMINDEX = 5;

const uint8_t PAYLOADSIZE = 7;

const uint8_t PUMPBITMASK =   B00000101;  //5
const uint8_t BUBBLESBITMASK =   B00000010;  //2
const uint8_t JETSBITMASK =   B00001000;  //8
const uint8_t HEATBITMASK1 =   B00110000;  //48  heater stage 1 = 50%
const uint8_t HEATBITMASK2 =   B01000000;  //64  heater stage 2 = 100%
const uint8_t POWERBITMASK =   B10000000;  //128
const bool HASJETS = true;
const String MYMODEL = "NO54173";
#endif

#ifdef NO54138

//what row in allowedstates to go to when pressing Bubbles, Jets, Pump, Heat (columns in that order)
//Example: We are in state zero (first row). If we press Bubbles (first column) then there is a 6
//meaning current state (row) is now 6. According to ALLOWEDSTATES table, we turn on Bubbles and keep
//everything else off. (1,0,0,0)
const uint8_t JUMPTABLE[][4] = {
  {1,2,3,4},
  {0,2,3,4},
  {1,0,3,4},
  {1,2,0,4},
  {1,2,0,3}
};
//Bubbles, Jets, Pump, Heat
const uint8_t ALLOWEDSTATES[][4] = {
  {0,0,0,0},
  {1,0,0,0},
  {0,1,0,0},
  {0,0,1,0},
  {0,0,1,2}  //the "2" means both heater elements
};

//cio
const uint8_t TEMPINDEX = 2;
const uint8_t ERRORINDEX = 3;
const uint8_t CIO_CHECKSUMINDEX = 5;
//dsp
const uint8_t COMMANDINDEX = 2;
const uint8_t DSP_CHECKSUMINDEX = 5;

const uint8_t PAYLOADSIZE = 7;

const uint8_t PUMPBITMASK =   B00000101;  //5
const uint8_t BUBBLESBITMASK =   B00000010;  //2
const uint8_t JETSBITMASK =   B00001000;  //8
const uint8_t HEATBITMASK1 =   B00000000;  //0    heater stage 1 = off
const uint8_t HEATBITMASK2 =   B00110000;  //48  heater stage 2 = on
//lines below should be tested. It would be consistent with 54173 model.
//If heating is slow this is probably the cause but I don't want to change it before someone tests it.
//const uint8_t HEATBITMASK1 =   B00110000;  //48  heater stage 1 = 50%
//const uint8_t HEATBITMASK2 =   B01000000;  //64  heater stage 2 = 100%
const uint8_t POWERBITMASK =   B10000000;  //128
const bool HASJETS = true;
const String MYMODEL = "NO54138";
#endif

#ifdef NO54123
//WARNING: THIS DEVICE HAS DIFFERENT PINOUTS!!! CHECK BEFORE USING
//#NO54112 - @jenswalit: "this is the Version Vegas 4Pin Pump 1->5V+ 2->Data 3->Data 4->5V-"  
//what row in allowedstates to go to when pressing Bubbles, Jets, Pump, Heat (columns in that order)
//Example: We are in state zero (first row). If we press Bubbles (first column) then there is a 6
//meaning current state (row) is now 6. According to ALLOWEDSTATES table, we turn on Bubbles and keep
//everything else off. (1,0,0,0)
const uint8_t JUMPTABLE[][4] = {
  {1,0,2,3},
  {0,1,2,3},
  {1,2,0,3},
  {1,3,0,2}
};
//Bubbles, Jets, Pump, Heat
const uint8_t ALLOWEDSTATES[][4] = {
  {0,0,0,0},
  {1,0,0,0},
  {0,0,1,0},
  {0,0,1,2}  //the "2" means both heater elements
};

//cio
const uint8_t TEMPINDEX = 2;
const uint8_t ERRORINDEX = 3;
const uint8_t CIO_CHECKSUMINDEX = 5;
//dsp
const uint8_t COMMANDINDEX = 2;
const uint8_t DSP_CHECKSUMINDEX = 5;

const uint8_t PAYLOADSIZE = 7;

const uint8_t PUMPBITMASK =    B00010000;  //1 << 4;
const uint8_t BUBBLESBITMASK =  B00100000;  //1 << 5;
const uint8_t JETSBITMASK =   B00000000;  //0;  //no jets on this machine.
const uint8_t HEATBITMASK1 =   B00000010;  //(1 << 1) "stage 1"
const uint8_t HEATBITMASK2 =   B00001000;  //(1 << 3) "stage 2" (thanks @dietmar-1 for testing and reporting this)
const uint8_t POWERBITMASK =   B00000001;  //1;
const bool HASJETS = false;
const String MYMODEL = "NO54123";
#endif

#ifdef NO54154
//WARNING: THIS DEVICE HAS DIFFERENT PINOUTS!!! CHECK BEFORE USING
//Requested by @chunkysteveo
const uint8_t JUMPTABLE[][4] = {
  {3,0,1,2},
  {4,1,0,2},
  {5,2,0,1},
  {0,3,4,5},
  {1,4,3,5},
  {2,5,3,4}
};
//Bubbles, Jets, Pump, Heat
const uint8_t ALLOWEDSTATES[][4] = {
  {0,0,0,0},
  {0,0,1,0},
  {0,0,1,2},
  {1,0,0,0},  //the "2" means both heater elements
  {1,0,1,0},
  {1,0,1,1}
};

//cio
const uint8_t TEMPINDEX = 2;
const uint8_t ERRORINDEX = 3;
const uint8_t CIO_CHECKSUMINDEX = 5;
//dsp
const uint8_t COMMANDINDEX = 2;
const uint8_t DSP_CHECKSUMINDEX = 5;

const uint8_t PAYLOADSIZE = 7;

const uint8_t PUMPBITMASK =    B00010000;  //1 << 4;
const uint8_t BUBBLESBITMASK =  B00100000;  //1 << 5;
const uint8_t JETSBITMASK =   B00000000;  //0;  //no jets on this machine.
const uint8_t HEATBITMASK1 =   B00000010;  //(1 << 1) "stage 1"
const uint8_t HEATBITMASK2 =   B00001000;  //(1 << 3) "stage 2" (thanks @dietmar-1 for testing and reporting this)
const uint8_t POWERBITMASK =   B00000001;  //1;
const bool HASJETS = false;
const String MYMODEL = "NO54154";
#endif

#ifdef NO54144

//what row in allowedstates to go to when pressing Bubbles, Jets, Pump, Heat (columns in that order)
//Example: We are in state zero (first row). If we press Bubbles (first column) then there is a 6
//meaning current state (row) is now 6. According to ALLOWEDSTATES table, we turn on Bubbles and keep
//everything else off. (1,0,0,0)
const uint8_t JUMPTABLE[][4] = {
// b,j,p,h  
  {1,2,3,4},
  {0,2,3,4},
  {1,0,3,4},
  {1,2,0,4},
  {1,2,0,3}
};
//Bubbles, Jets, Pump, Heat
const uint8_t ALLOWEDSTATES[][4] = {
  {0,0,0,0},
  {1,0,0,0},
  {0,1,0,0},
  {0,0,1,0},
  {0,0,1,2}  //the "2" means both heater elements
};

//cio
const uint8_t TEMPINDEX = 2;
const uint8_t ERRORINDEX = 3;
const uint8_t CIO_CHECKSUMINDEX = 5;
//dsp
const uint8_t COMMANDINDEX = 2;
const uint8_t DSP_CHECKSUMINDEX = 5;

const uint8_t PAYLOADSIZE = 7;

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
const bool HASJETS = true;
const bool HASAIR = false;
const String MYMODEL = "NO54144";
#endif

#endif