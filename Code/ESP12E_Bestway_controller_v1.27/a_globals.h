#include <ArduinoJson.h>

#include "ESPDateTime.h"
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>
//used by ap config portal
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Ticker.h>

Ticker tickerMinute;
Ticker tickerSecond;
Ticker tickerDay;
bool filterOnFlag = false;
bool filterOffFlag = false;
bool heaterEnableFlag = false;
bool heaterDisableFlag = false;
bool saveAppdataFlag = false;

ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81
bool portalRunning = false;
File fsUploadFile;                 // a File variable to temporarily store the received file

struct Config {
  int   filteroffhour = 3;
  int   filteronhour = 9;
  int   timezone = 0;     //timezone (UTC + TZ = your local time) DST seems to not be implemented in the ESPDateTime.h library
  bool  heaterhours[24] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
  int   audio = 1;  //on = 1/off=0 may be added to in the future
  float price = 1.0; //in whatever currency
  bool  automode = true;   //use scheduled events
};
Config myConfig;
const char *cfgFileName = "/config.txt";
bool saveCfgFlag = false;

struct Appdata {
  uint32_t clts;
  uint32_t heattime;
  uint32_t uptime;
  uint32_t airtime;
  uint32_t filtertime;
  float cost;
};
Appdata appdata;
const char *appdataFileName = "/save.txt";

//********USER PARAMETERS*************
const char *OTAName = "BW-1.27";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266";       // Not used!


//Pins
const int CS_DSP_PIN = D3;
const int CLK_DSP_PIN = D4;
const int DATA_DSP_PIN = D5;
const int AUDIO_OUT_PIN = D6;

const int CS_CIO_PIN = D1;
const int CLK_CIO_PIN = D2;
const int DATA_CIO_PIN = D7;

//CIO variables
volatile uint8_t DSP_IN[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t DSP_valid[11];
volatile bool newData = false;
volatile uint16_t BTN_OUT = 0xD8D8; //no button
uint16_t virtualBTN = 0x1090; //init to up button to get set temp
uint16_t realBTN = 0xD8D8; //no button (pressed on display)
uint16_t prevBTN = 0xD8D8;
uint16_t autoBTN = 0xD8D8;
bool virtual_onoff = 0;
unsigned long BTN_timeout;
unsigned long target_timeout;
bool runOnce = false;

uint8_t DSP_BRT_IN = 0x01;//debug
const uint8_t DSP_RQ = 0x42;
const uint8_t DSP_STS1 = 0x80;
const uint8_t DSP_STS2 = 0x02;

volatile int bitCount = 0;
volatile int byteCount = 0;
volatile uint8_t receivedByte = 0;
volatile int sendBit = 16;
//volatile boolean chk1, chk2, chk3 = false;  //I should have an int counter instead of three booleans!
volatile int CIO_cmd_matches = 0;
volatile bool CS_CIO_active = false;
volatile bool CS_answermode = false;
volatile bool CIO_deselect = false;


//DSP variables
uint8_t DSP_OUT[11] = {0x03, 0xDF, 0xFF, 0xDB, 0xFF, 0xB3, 0xFF, B10010001, 0xFF, 0x80, 0xFF};
uint8_t brightness = 1; //Not fully used. We obey the CIO now, instead of changing LED brightness by our own.
//25% B00010001, 50% B00110001, 75% B01110001, 100% B1110001


//Display button codes (This is the values reported by DSP when asked)
const uint16_t LCK = 0x4000;
const uint16_t TMR = 0x8000;
const uint16_t AIR = 0xC000;
const uint16_t UNT = 0x0848;
const uint16_t HTR = 0x4848;
const uint16_t FLT = 0x8848;
const uint16_t DWN = 0xC848;
const uint16_t UP =  0x1090;
const uint16_t PWR = 0x0000;
const uint16_t NOBTN = 0xD8D8;

//Status variables
uint8_t cur_tmp_val = 0;
String cur_tmp_str = "0";
uint8_t set_tmp_val = 0;
bool locked_sts;
bool locked_cmd;
bool power_sts;
bool power_cmd;
bool air_sts;
bool air_cmd;
bool heater_red_sts;
bool heater_green_sts;
bool heater_cmd;
bool filter_sts;
bool filter_cmd;
bool celsius_sts;
bool celsius_cmd;
//there is no value to be read so we need to scan the display after UP/DOWN has been pressed.
//this is a counter to figure out exactly when to scan.
int fetchTargetTemp = 0;
//When the pump (filter) starts after autoshutdown, should we also start the heater?
//It updates every time you change state of the heater.
bool savedHeaterState = 0;


//CIO data adresses (see documentation in excel file on github)
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

uint32_t airStart, heaterStart, filterStart, uptimestamp;
int prevHour = 0;

//direct port manipulation memory adresses.
/*
  PROVIDE(PIN_OUT = 0x60000300);
  PROVIDE(PIN_OUT_SET = 0x60000304);
  PROVIDE(PIN_OUT_CLEAR = 0x60000308);

  PROVIDE(PIN_DIR = 0x6000030C);
  PROVIDE(PIN_DIR_OUTPUT = 0x60000310);
  PROVIDE(PIN_DIR_INPUT = 0x60000314);

  PROVIDE(PIN_IN = 0x60000318);
*/
#define PIN_OUT 0x60000300
#define PIN_OUT_SET 0x60000304
#define PIN_OUT_CLEAR 0x60000308

#define PIN_DIR 0x6000030C
#define PIN_DIR_OUTPUT 0x60000310
#define PIN_DIR_INPUT 0x60000314

#define PIN_IN 0x60000318
