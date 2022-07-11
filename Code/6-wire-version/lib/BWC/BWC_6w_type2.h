#ifndef BWC_TYPE2_H
#define BWC_TYPE2_H

#include "Arduino.h"
#include "BWC_const.h"

#ifdef PCB_V2
const int ciopins[] = {D1, D2, D3};
const int dsppins[] = {D4, D5, D6, D7};
#else
const int ciopins[] = {D7, D2, D1};
const int dsppins[] = {D5, D4, D3, D6};
#endif

const uint16_t ButtonCodes[] =
{
  0, 1<<7, 1<<6, 1<<5, 1<<4, 1<<3, 1<<2, 1<<1, 1<<0, 1<<8, 1<<9
};
const bool HASJETS = false;
const String MYMODEL = "MODEL54149E";

//LSB
const uint8_t CMD1 = B01000000;  //normal mode, auto+1 address
const uint8_t CMD2 = B11000000; //start address 00H
const uint8_t CMD3 = DSP_DIM_BASE | DSP_DIM_ON | 7;  //full brightness
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
const byte HJT_BIT = 4;  //wild guess

//8-segment codes. MSB-> .gfedcba <-LSB
const uint8_t CHARCODES[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x00, 0x40, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71,
  0x7D, 0x74, 0x76, 0x30, 0x0E, 0x70, 0x38, 0x00, 0x54, 0x5C, 0x73, 0x67, 0x50, 0x6D, 0x78, 0x1C, 0x3E, 0x00, 0x6E, 0x5B
};

class CIO {

  public:
    void begin(int cio_td_pin, int cio_clk_pin, int cio_ld_pin);
    void loop(void);
    void eopHandler(void);
    void packetHandler(void);
    void clkHandler(void);
    void stop(void);

    volatile bool newData = false;
    bool dataAvailable = false;
    bool stateChanged = false; //save states when true
    volatile uint16_t button = NOBTN;
    uint8_t payload[5];
    uint8_t states[14];
    uint16_t state_age[14];
    bool state_changed[14];
    uint8_t brightness;
    bool targetIsDisplayed = false;
    //uint16_t lastPressedButton;
    volatile uint32_t clk_per; //lowest time between clock edges. Should be ~20 us.


  private:
    volatile int _byteCount = 0;
    volatile int _bitCount = 0;
    volatile byte _receivedByte;
    volatile bool _packet = false;
    volatile int _sendBit = 8;
    volatile uint8_t _brightness;
    volatile uint8_t _payload[5];
    int _CIO_TD_PIN;
    int _CIO_CLK_PIN;
    int _CIO_LD_PIN;
    uint8_t _prevPayload[5];
    bool _prevUNT;
    bool _prevHTR;
    bool _prevFLT;
    uint8_t _received_cmd = 0;  //temporary storage of command message

    char _getChar(uint8_t value);
};

class DSP {

  public:
    uint8_t payload[5];

    void begin(int dsp_td_pin, int dsp_clk_pin, int dsp_ld_pin, int dsp_audio_pin);
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
    uint16_t _prevButton = ButtonCodes[NOBTN];
    //Pins
    int _DSP_TD_PIN;
    int _DSP_CLK_PIN;
    int _DSP_LD_PIN;
    int _DSP_AUDIO_PIN;
};


#endif
