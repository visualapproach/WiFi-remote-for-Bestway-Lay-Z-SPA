//only declarations here please! Definitions belong in the cpp-file
#include "model.h"

#ifndef BWC54149E_8266_globals_H
#define BWC54149E_8266_globals_H

//LSB
const uint8_t DSP_DIM_BASE = 0x80;
const uint8_t DSP_DIM_ON = 0x8;
const uint8_t CMD1 = B01000000;	//normal mode, auto+1 address
const uint8_t CMD2 = B11000000; //start address 00H
const uint8_t CMD3 = DSP_DIM_BASE | DSP_DIM_ON | 7;	//full brightness
const uint16_t CLKPW = 50; //clock pulse period in us. clockfreq = 1/2*CLKPW


//Payload byte index and bit numbers  (see documentation in excel file on github)
//LSB first
const byte DGT1_IDX = 0;
const byte DGT2_IDX = 1;
const byte DGT3_IDX = 2;
const byte TMR2_IDX = 3;
const byte TMR2_BIT = 7;
const byte TMR1_IDX = 3;
const byte TMR1_BIT = 6;
const byte LCK_IDX = 3;
const byte LCK_BIT = 5;
const byte TMRBTNLED_IDX = 3;
const byte TMRBTNLED_BIT = 4;
const byte REDHTR_IDX = 3;
const byte REDHTR_BIT = 2;
const byte GRNHTR_IDX = 3;
const byte GRNHTR_BIT = 3;
const byte AIR_IDX = 3;
const byte AIR_BIT = 1;
const byte FLT_IDX = 4;
const byte FLT_BIT = 2;
const byte C_IDX = 4;
const byte C_BIT = 0;
const byte F_IDX = 4;
const byte F_BIT = 1;
const byte PWR_IDX = 4; //not used. Always considered ON
const byte PWR_BIT = 3;
const byte HJT_IDX = 4; //wild guess if it exists on any model
const byte HJT_BIT = 4;	//wild guess

//8-segment codes. MSB-> .gfedcba <-LSB
const uint8_t CHARCODES[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x00, 0x40, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71,
  0x7D, 0x74, 0x76, 0x30, 0x0E, 0x70, 0x38, 0x00, 0x54, 0x5C, 0x73, 0x67, 0x50, 0x6D, 0x78, 0x1C, 0x3E, 0x00, 0x6E, 0x5B
};
const uint8_t CHARS[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '-', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'H', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'x', 'y', 'z'
};

enum Buttons: byte
{
	NOBTN,
	LOCK,
	TIMER,
	BUBBLES,
	UNIT,
	HEAT,
	PUMP,
	DOWN,
	UP,
	POWER,
	HYDROJETS
};

//set to zero to disable display buttons. Order as above.
//Example: to disable UNIT and TIMER set 1,1,0,1,0,1,1,1,1,1,1
const uint8_t EnabledButtons[] = {1,1,1,1,1,1,1,1,1,1,1};
const String ButtonNames[] = {
	"NOBTN",
	"LOCK",
	"TIMER",
	"BUBBLES",
	"UNIT",
	"HEAT",
	"PUMP",
	"DOWN",
	"UP",
	"POWER",		//not existing
	"HYDROJETS"		//not existing
};

#ifdef MODEL54149E
const uint16_t ButtonCodes[] =
{
	0, 1<<7, 1<<6, 1<<5, 1<<4, 1<<3, 1<<2, 1<<1, 1<<0, 1<<8, 1<<9
};
const bool HASJETS = false;
#else //keep compiler happy with this dummy
const uint16_t ButtonCodes[] =
{
	0, 1<<7, 1<<6, 1<<5, 1<<4, 1<<3, 1<<2, 1<<1, 1<<0, 1<<8, 1<<9
};
const bool HASJETS = false;
#endif

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
	JETSSTATE
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
	SETBRIGHTNESS
	//play song
};

const int MAXCOMMANDS = 11;
const int MAXBUTTONS = 200;


//direct port manipulation memory adresses.
#define PIN_OUT 0x60000300
#define PIN_OUT_SET 0x60000304
#define PIN_OUT_CLEAR 0x60000308

#define PIN_DIR 0x6000030C
#define PIN_DIR_OUTPUT 0x60000310
#define PIN_DIR_INPUT 0x60000314

#define PIN_IN 0x60000318


#endif
