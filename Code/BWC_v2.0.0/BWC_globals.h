//only declarations here please! Definitions belong in the cpp-file


#ifndef BWC_globals_H
#define BWC_globals_H

const uint8_t DSP_RQ = 0x42;
const uint8_t DSP_STS1 = 0x80;
const uint8_t DSP_STS2 = 0x02;

//Payload byte index and bit numbers  (see documentation in excel file on github)
const byte DGT1_IDX = 1;
const byte DGT2_IDX = 3;
const byte DGT3_IDX = 5;
const byte TMR2_IDX = 7;
const byte TMR2_BIT = 6;
const byte TMR1_IDX = 7;
const byte TMR1_BIT = 5;
const byte LCK_IDX = 7;
const byte LCK_BIT = 4;
const byte TMRBTNLED_IDX = 7;
const byte TMRBTNLED_BIT = 3;
const byte REDHTR_IDX = 7;
const byte REDHTR_BIT = 2;
const byte GRNHTR_IDX = 7;
const byte GRNHTR_BIT = 1;
const byte AIR_IDX = 7;
const byte AIR_BIT = 0;
const byte FLT_IDX = 9;
const byte FLT_BIT = 6;
const byte C_IDX = 9;
const byte C_BIT = 5;
const byte F_IDX = 9;
const byte F_BIT = 4;
const byte PWR_IDX = 9;
const byte PWR_BIT = 3;

//7-segment codes. MSB always 1
const uint8_t CHARCODES[] = {
  0xFE, 0xB0, 0xED, 0xF9, 0xB3, 0xDB, 0xDF, 0xF0, 0xFF, 0xFB, 0x80, 0x81, 0xF7, 0x9F, 0xCE, 0xBD, 0xCF, 0xC7,
  0xDF, 0x97, 0x86, 0xB8, 0x87, 0x8E, 0x80, 0x95, 0x9D, 0xE7, 0xF3, 0x85, 0xDB, 0x8F, 0x9C, 0xBE, 0x80, 0xBB, 0xED
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
	POWER
};

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
	TARGET,
	TEMPERATURE,
	CHAR1,
	CHAR2,
	CHAR3
};

const uint16_t ButtonCodes[] = 
{
	0xD8D8, 0x4000, 0x8000, 0xC000, 0x0848, 0x4848, 0x8848, 0xC848, 0x1090, 0x0000
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
	RESETCLTIMER
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
