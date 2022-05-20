#ifndef BWC_TYPE1_H
#define BWC_TYPE1_H

#include "Arduino.h"
#include "BWC_const.h"

#ifdef PCB_V2
const int ciopins[] = {D1, D2, D3};
const int dsppins[] = {D4, D5, D6, D7};
#else
const int ciopins[] = {D7, D2, D1};
const int dsppins[] = {D5, D4, D3, D6};
#endif

//LSB
const uint8_t DSP_CMD2_DATAREAD = 0x42;
const uint8_t DSP_CMD1_MODE6_11_7 = 0x01; //real CIO is sending 0x01 which is illegal according to datasheet
const uint8_t DSP_CMD2_DATAWRITE = 0x40;

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
const byte HJT_IDX = 9;
const byte HJT_BIT = 5;

//7-segment codes. MSB always 1
const uint8_t CHARCODES[] = {
  0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF, 0x01, 0x81, 0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3,
  0xFB, 0xE9, 0xED, 0x61, 0x1D, 0xE1, 0x71, 0x01, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB, 0xF1, 0x39, 0x7D, 0x01, 0xDD, 0xB7
};

#if defined(PRE2021)
const uint16_t ButtonCodes[] =
{
  0x1B1B, 0x0200, 0x0100, 0x0300, 0x1012, 0x1212, 0x1112, 0x1312, 0x0809, 0x0000
};
const bool HASJETS = false;
const String MYMODEL = "PRE2021";

#elif defined(MIAMI2021)
const uint16_t ButtonCodes[] =
{
  0x1B1B, 0x0100, 0x0300, 0x1212, 0x0809, 0x1012, 0x1112, 0x1312, 0x0200, 0x0000
};
const bool HASJETS = false;
const String MYMODEL = "MIAMI2021";

#elif defined(MALDIVES2021)
const uint16_t ButtonCodes[] =
{
  0x1B1B, 0x0100, 0x0300, 0x1212, 0x0a09, 0x1012, 0x1312, 0x0809, 0x0200, 0x0000, 0x1112
};
const bool HASJETS = true;
const String MYMODEL = "MALDIVES2021";

#else
//Make compiler happy. Will not be used.
const uint16_t ButtonCodes[] =
{
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};
const bool HASJETS = false;
const String MYMODEL = "ISCREWEDUP";

#endif


class CIO {

  public:
    void begin(int cio_data_pin, int cio_clk_pin, int cio_cs_pin);
    void loop(void);
    void eopHandler(void);
    void packetHandler(void);
    void clkHandler(void);
    void stop(void);

    volatile bool newData = false;
    bool dataAvailable = false;
    bool stateChanged = false; //save states when true
    volatile uint16_t button = NOBTN;
    uint8_t payload[11];
    uint8_t states[14];
    uint8_t brightness;
    bool targetIsDisplayed = false;
    //uint16_t lastPressedButton;
    volatile uint32_t clk_per; //lowest time between clock edges. Should be ~20 us.


  private:
    volatile int _byteCount = 0;
    volatile int _bitCount = 0;
    volatile bool _dataIsOutput = false;
    volatile byte _receivedByte;
    volatile int _CIO_cmd_matches = 0;
    volatile bool _packet = false;
    volatile int _sendBit = 8;
    volatile uint8_t _brightness;
    volatile uint8_t _payload[11];
    int _CS_PIN;
    int _CLK_PIN;
    int _DATA_PIN;
    uint8_t _prevPayload[11];
    bool _prevUNT;
    bool _prevHTR;
    bool _prevFLT;

    char _getChar(uint8_t value);
};

class DSP {

  public:
    uint8_t payload[11] = {0xC0, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x0};

    void begin(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int dsp_audio_pin);
    uint16_t getButton(void);
    void updateDSP(uint8_t brightness);
    void textOut(String txt);
    void LEDshow();
    void playIntro();
    void beep();
    void beep2();

  private:
    void _sendBitsToDSP(uint32_t outBits, int bitsToSend);
    uint16_t _receiveBitsFromDSP();
    char _getCode(char value);

    unsigned long _dspLastRefreshTime = 0;
    unsigned long _dspLastGetButton = 0;
    uint16_t _oldButton = ButtonCodes[NOBTN];
    //Pins
    int _CS_PIN;
    int _CLK_PIN;
    int _DATA_PIN;
    int _AUDIO_PIN;
};


#endif
