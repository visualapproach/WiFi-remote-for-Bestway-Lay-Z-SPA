#pragma once
#include <Arduino.h>
#include "enums.h"
#include "DSP_6W.h"

class DSP_6_TYPE1 : public DSP_6W
{
    public:
        DSP_6_TYPE1(){};
        virtual ~DSP_6_TYPE1(){};
        void setup(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int dsp_audio_pin);
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
        uint8_t getDGT1_IDX() override {return DGT1_IDX;}
        uint8_t getDGT2_IDX() override {return DGT2_IDX;}
        uint8_t getDGT3_IDX() override {return DGT3_IDX;}
        uint8_t getTMR2_IDX() override {return TMR2_IDX;}
        uint8_t getTMR2_BIT() override {return TMR2_BIT;}
        uint8_t getTMR1_IDX() override {return TMR1_IDX;}
        uint8_t getTMR1_BIT() override {return TMR1_BIT;}
        uint8_t getLCK_IDX() override {return LCK_IDX;}
        uint8_t getLCK_BIT() override {return LCK_BIT;}
        uint8_t getTMRBTNLED_IDX() override {return TMRBTNLED_IDX;}
        uint8_t getTMRBTNLED_BIT() override {return TMRBTNLED_BIT;}
        uint8_t getREDHTR_IDX() override {return REDHTR_IDX;}
        uint8_t getREDHTR_BIT() override {return REDHTR_BIT;}
        uint8_t getGRNHTR_IDX() override {return GRNHTR_IDX;}
        uint8_t getGRNHTR_BIT() override {return GRNHTR_BIT;}
        uint8_t getAIR_IDX() override {return AIR_IDX;}
        uint8_t getAIR_BIT() override {return AIR_BIT;}
        uint8_t getFLT_IDX() override {return FLT_IDX;}
        uint8_t getFLT_BIT() override {return FLT_BIT;}
        uint8_t getC_IDX() override {return C_IDX;}
        uint8_t getC_BIT() override {return C_BIT;}
        uint8_t getF_IDX() override {return F_IDX;}
        uint8_t getF_BIT() override {return F_BIT;}
        uint8_t getPWR_IDX() override {return PWR_IDX;}
        uint8_t getPWR_BIT() override {return PWR_BIT;}
        uint8_t getHJT_IDX() override {return HJT_IDX;}
        uint8_t getHJT_BIT() override {return HJT_BIT;}
        uint8_t getCS() override {return _CS_PIN;}
        uint8_t getCLK() override {return _CLK_PIN;}
        uint8_t getDATA() override {return _DATA_PIN;}
        uint8_t getAUDIO() override {return _AUDIO_PIN;}
        Buttons getPressedButton() override;

    protected:
        unsigned long _dsp_last_refreshtime = 0;
        unsigned long _dsp_getbutton_last_time = 0;
        int _CS_PIN;
        int _CLK_PIN;
        int _DATA_PIN;
        int _AUDIO_PIN;
        const uint8_t DSP_DIM_BASE = 0x80;
        const uint8_t DSP_DIM_ON = 0x8;
        /*Real CIO is sending 0x01 which is illegal according to datasheet.
        Static to work in switch/case.*/
        /*Needs to be static to work in switch/case.*/
        static const uint8_t DSP_CMD1_MODE6_11_7 = 0x01; //new model send 0x05
        static const uint8_t DSP_CMD1_MODE6_11_7_P05504 = 0x05;
        /*Needs to be static to work in switch/case.*/
        static const uint8_t DSP_CMD2_DATAREAD = 0x42;
        /*Needs to be static to work in switch/case.*/
        static const uint8_t DSP_CMD2_DATAWRITE = 0x40;
        //7-segment codes. LSB can be 0 or 1 depending on pump. Not used AFAIK.
        //Payload byte index and bit numbers (see documentation in excel file on github)
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
        /*Bit 0 (LSB) is the not used or enable? bit for the whole byte. We leave it on to mimic type2 which don't have enable bits.*/
        const uint8_t CHARCODES[38] = {
            0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF, 0x01, 0x81, 0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3, 0xFB,
            0xE9, 0xED, 0x61, 0x1D, 0xE1, 0x71, 0x01, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB, 0xF1, 0x39, 0x7D, 0x01, 0xDD, 0xB7
        };

    private:
        Buttons _old_button = NOBTN;
        uint8_t _payload[11] = {0xC0, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x0};
};
