//only declarations here please! Definitions belong in the cpp-file


#ifndef BWC_8266_globals_H
#define BWC_8266_globals_H

//uncomment your model and comment out the rest
#define PRE2021			//the older one, no hydrojets
//#define MIAMI2021		//no hydrojets
//#define MALDIVES2021	//hydrojets


//LSB
const uint8_t DSP_CMD2_DATAREAD = 0x42;
const uint8_t DSP_CMD1_MODE6_11_7 = 0x01; //real CIO is sending 0x01 which is illegal according to datasheet
const uint8_t DSP_CMD2_DATAWRITE = 0x40;
const uint8_t DSP_DIM_BASE = 0x80;
const uint8_t DSP_DIM_ON = 0x8;

//Payload byte index and bit numbers  (see documentation in excel file on github)
//LSB first
const byte DGT1_IDX = 1;
const byte DGT2_IDX = 3;
const byte DGT3_IDX = 5;
const byte TMR2_IDX = 7;
const byte TMR2_BIT = 1;
const byte TMR1_IDX = 7;
const byte TMR1_BIT = 2;
const byte LCK_IDX = 7;
const byte LCK_BIT = 3;
const byte TMRBTNLED_IDX = 7;
const byte TMRBTNLED_BIT = 4;
const byte REDHTR_IDX = 7;
const byte REDHTR_BIT = 5;
const byte GRNHTR_IDX = 7;
const byte GRNHTR_BIT = 6;
const byte AIR_IDX = 7;
const byte AIR_BIT = 7;
const byte FLT_IDX = 9;
const byte FLT_BIT = 1;
const byte C_IDX = 9;
const byte C_BIT = 2;
const byte F_IDX = 9;
const byte F_BIT = 3;
const byte PWR_IDX = 9;
const byte PWR_BIT = 4;
const byte HJT_IDX = 9;	//still to be verified. This is an educated guess
const byte HJT_BIT = 5;	//if correct the web page should show correct states on everything

//7-segment codes. MSB always 1
const uint8_t CHARCODES[] = {
  0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF, 0x01, 0x81, 0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3,
  0xFB, 0xE9, 0x61, 0x1D, 0xE1, 0x71, 0x01, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB, 0xF1, 0x39, 0x7D, 0x01, 0xDD, 0xB7
};
const uint8_t CHARS[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '-', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'x', 'y', 'z'
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

#ifdef PRE2021
const uint16_t ButtonCodes[] = 
{
	0x1B1B, 0x0200, 0x0100, 0x0300, 0x1012, 0x1212, 0x1112, 0x1312, 0x0809, 0x0000
};
const bool HASJETS = false;

#elif defined(MIAMI2021)
const uint16_t ButtonCodes[] = 
{
	0x1B1B, 0x0100, 0x0300, 0x1212, 0x0809, 0x1012, 0x1112, 0x1312, 0x0200, 0x0000
};
const bool HASJETS = false;

#else 
const uint16_t ButtonCodes[] = 
{
	0x1B1B, 0x0100, 0x0300, 0x1212, 0x0a09, 0x1012, 0x1312, 0x0809, 0x0200, 0x0000, 0x1112
};
const bool HASJETS = true;
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
	SETJETS
	//play song
};

const int MAXCOMMANDS = 11;
const int MAXBUTTONS = 33;


//direct port manipulation memory adresses.
#define PIN_OUT 0x60000300
#define PIN_OUT_SET 0x60000304
#define PIN_OUT_CLEAR 0x60000308

#define PIN_DIR 0x6000030C
#define PIN_DIR_OUTPUT 0x60000310
#define PIN_DIR_INPUT 0x60000314

#define PIN_IN 0x60000318


#endif
