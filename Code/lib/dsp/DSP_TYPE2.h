#pragma once
#include <Arduino.h>
#include "enums.h"
#include "DSP_6W.h"

class DSP_6_TYPE2 : public DSP_6W
{
    public:
        DSP_6_TYPE2(){};
        virtual ~DSP_6_TYPE2(){};
        void setup(int dsp_td_pin, int dsp_clk_pin, int dsp_ld_pin, int dsp_audio_pin);
        void stop();
        void handleStates() override;

    protected:
        void uploadPayload(uint8_t brightness) override;
        uint8_t charTo7SegmCode(char c) override;
        virtual Buttons buttonCodeToIndex(uint16_t code) = 0;

    protected:
        void _sendBitsToDSP(uint32_t out_bits, int bit_count);
        uint16_t _receiveBitsFromDSP();
        void clearpayload();
        uint8_t getDGT1_IDX() override {return DGT1_IDX;};
        uint8_t getDGT2_IDX() override {return DGT2_IDX;};
        uint8_t getDGT3_IDX() override {return DGT3_IDX;};
        uint8_t getTMR2_IDX() override {return TMR2_IDX;};
        uint8_t getTMR2_BIT() override {return TMR2_BIT;};
        uint8_t getTMR1_IDX() override {return TMR1_IDX;};
        uint8_t getTMR1_BIT() override {return TMR1_BIT;};
        uint8_t getLCK_IDX() override {return LCK_IDX;};
        uint8_t getLCK_BIT() override {return LCK_BIT;};
        uint8_t getTMRBTNLED_IDX() override {return TMRBTNLED_IDX;};
        uint8_t getTMRBTNLED_BIT() override {return TMRBTNLED_BIT;};
        uint8_t getREDHTR_IDX() override {return REDHTR_IDX;};
        uint8_t getREDHTR_BIT() override {return REDHTR_BIT;};
        uint8_t getGRNHTR_IDX() override {return GRNHTR_IDX;};
        uint8_t getGRNHTR_BIT() override {return GRNHTR_BIT;};
        uint8_t getAIR_IDX() override {return AIR_IDX;};
        uint8_t getAIR_BIT() override {return AIR_BIT;};
        uint8_t getFLT_IDX() override {return FLT_IDX;};
        uint8_t getFLT_BIT() override {return FLT_BIT;};
        uint8_t getC_IDX() override {return C_IDX;};
        uint8_t getC_BIT() override {return C_BIT;};
        uint8_t getF_IDX() override {return F_IDX;};
        uint8_t getF_BIT() override {return F_BIT;};
        uint8_t getPWR_IDX() override {return PWR_IDX;};
        uint8_t getPWR_BIT() override {return PWR_BIT;};
        uint8_t getHJT_IDX() override {return HJT_IDX;};
        uint8_t getHJT_BIT() override {return HJT_BIT;};
        uint8_t getCS() override {return _DSP_TD_PIN;};
        uint8_t getCLK() override {return _DSP_CLK_PIN;};
        uint8_t getDATA() override {return _DSP_LD_PIN;};
        uint8_t getAUDIO() override {return _DSP_AUDIO_PIN;};
        Buttons getPressedButton() override;

    protected:
        unsigned long _dsp_last_refreshtime = 0;
        unsigned long _dsp_getbutton_last_time = 0;
        //Pins
        int _DSP_TD_PIN;
        int _DSP_CLK_PIN;
        int _DSP_LD_PIN;
        int _DSP_AUDIO_PIN;
        const uint16_t CLKPW = 50; //clock pulse period in us. clockfreq = 1/2*CLKPW
        const uint8_t DSP_DIM_BASE = 0x80;
        const uint8_t DSP_DIM_ON = 0x8;

        //LSB
        const uint8_t CMD1 = B01000000;  //normal mode, auto+1 address
        const uint8_t CMD2 = B11000000; //start address 00H
        const uint8_t CMD3 = DSP_DIM_BASE | DSP_DIM_ON | 7;  //full brightness

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
        const uint8_t CHARCODES[38] = {
            0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x00, 0x40, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x7D,
            0x74, 0x76, 0x30, 0x0E, 0x70, 0x38, 0x00, 0x54, 0x5C, 0x73, 0x67, 0x50, 0x6D, 0x78, 0x1C, 0x3E, 0x00, 0x6E, 0x5B
        };
        // const uint8_t CHARS[38] = {
        //     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '-', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
        //     'h', 'H', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'x', 'y', 'z'
        // };

    private:
        const String MYDSPMODEL = "MODEL54149E";
        Buttons _old_button = NOBTN;
        Buttons _prev_button = NOBTN;
        uint8_t _payload[5];
};
