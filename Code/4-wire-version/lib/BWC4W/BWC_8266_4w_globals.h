#ifndef BWC_8266_4W_GLOBALS_H
#define BWC_8266_4W_GLOBALS_H
#include <model.h>
//Commands to CIO
//	130 = bubbles
//	133 = filter pump
//	136 = Jets
//	138 = Jets and bubbles
//	181 = heating both elements with filter pump

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
	SETGODMODE
};

const int MAXCOMMANDS = 11;
const int MAXBUTTONS = 33;

#ifdef NO54173
//cio
const uint8_t TEMPINDEX = 2;
const uint8_t ERRORINDEX = 3;
const uint8_t CIO_CHECKSUMINDEX = 5;
//dsp
const uint8_t COMMANDINDEX = 2;
const uint8_t DSP_CHECKSUMINDEX = 5;

const uint8_t PAYLOADSIZE = 7;

const uint8_t PUMPBITMASK = 	B00000101;	//5
const uint8_t BUBBLESBITMASK = 	B00000010;	//2
const uint8_t JETSBITMASK = 	B00001000;	//8
const uint8_t HEATBITMASK1 = 	B00110000;	//48	heater stage 1 = 50%
const uint8_t HEATBITMASK2 = 	B01000000;	//64	heater stage 2 = 100%
const uint8_t POWERBITMASK = 	B10000000;	//128
#endif

#ifdef NO54138
//cio
const uint8_t TEMPINDEX = 2;
const uint8_t ERRORINDEX = 3;
const uint8_t CIO_CHECKSUMINDEX = 5;
//dsp
const uint8_t COMMANDINDEX = 2;
const uint8_t DSP_CHECKSUMINDEX = 5;

const uint8_t PAYLOADSIZE = 7;

const uint8_t PUMPBITMASK = 	B00000101;	//5
const uint8_t BUBBLESBITMASK = 	B00000010;	//2
const uint8_t JETSBITMASK = 	B00001000;	//8
const uint8_t HEATBITMASK1 = 	B00000000;	//0		heater stage 1 = off
const uint8_t HEATBITMASK2 = 	B00110000;	//48	heater stage 2 = on
const uint8_t POWERBITMASK = 	B10000000;	//128
#endif

#ifdef NO54123
//WARNING: THIS DEVICE HAS DIFFERENT PINOUTS!!! CHECK BEFORE USING
//NOT TESTED BY ME!!
//cio
const uint8_t TEMPINDEX = 2;
const uint8_t ERRORINDEX = 3;
const uint8_t CIO_CHECKSUMINDEX = 5;
//dsp
const uint8_t COMMANDINDEX = 2;
const uint8_t DSP_CHECKSUMINDEX = 5;

const uint8_t PAYLOADSIZE = 7;

//following is how I interpret the slides. (LSB = bit0)
//It may very well be the opposite (MSB = bit0)
const uint8_t PUMPBITMASK =		B00010000;	// 1 << 4;
const uint8_t BUBBLESBITMASK =	B00100000;	//1 << 5;
const uint8_t JETSBITMASK = 	B00000000;	//0;  //no jets on this machine.
const uint8_t HEATBITMASK1 = 	B00000000;	//(1 << 1) | (1 << 3); 	this is the "stage 1" setting (no heat)
const uint8_t HEATBITMASK2 = 	B00001010;	//0; 					I don't know if there is one or more heater elements, so this is the "stage 2" setting
const uint8_t POWERBITMASK = 	B00000001;	//1;
#endif

#endif