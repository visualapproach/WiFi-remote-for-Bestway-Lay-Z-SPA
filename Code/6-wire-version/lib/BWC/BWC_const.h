#ifndef BWC_CONST_H
#define BWC_CONST_H
#include "Arduino.h"
#include "model.h"

const uint8_t DSP_DIM_BASE = 0x80;
const uint8_t DSP_DIM_ON = 0x8;

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
  "POWER",
  "HYDROJETS"
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
  SETBRIGHTNESS,
  SETBEEP
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