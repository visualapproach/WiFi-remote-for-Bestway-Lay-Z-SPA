#pragma once
#include <Arduino.h>
#include "enums.h"
#include "CIO_6W.h"

class CIO_6_TYPE1: public CIO_6W
{
    public:
        CIO_6_TYPE1();
        virtual ~CIO_6_TYPE1(){};
        void setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin) override;
        void stop() override;
        void pause_all(bool action) override;
        void IRAM_ATTR isr_packetHandler();
        void IRAM_ATTR isr_clkHandler();
        void updateStates();

    protected:
        virtual uint16_t getButtonCode(Buttons button_index) = 0;
        virtual Buttons getButton(uint16_t code) = 0;
        virtual bool getHasjets() = 0;
        virtual bool getHasair() = 0;

    private:
        char _getChar(uint8_t value);
        void IRAM_ATTR eopHandler();

    private:
        volatile int _byte_count;
        volatile int _bit_count;
        volatile int _CIO_cmd_matches;
        volatile int _send_bit;
        int _CS_PIN;
        int _CLK_PIN;
        int _DATA_PIN;
        /*Real CIO is sending 0x01 which is illegal according to datasheet.*/
        /*Needs to be static to work in switch/case.*/
        static const uint8_t DSP_CMD1_MODE6_11_7 = 0x01; //new model send 0x05
        static const uint8_t DSP_CMD1_MODE6_11_7_P05504 = 0x05;
        /*Needs to be static to work in switch/case.*/
        static const uint8_t DSP_CMD2_DATAREAD = 0x42;
        static const uint8_t DSP_CMD2_DATAWRITE = 0x40;

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
        //7-segment codes. MSB (as seen by the CIO) is always 1 (Enable bit)
        const uint8_t CHARCODES[38] = {
            0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF, 0x01, 0x81, 0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3, 0xFB,
            0xE9, 0xED, 0x61, 0x1D, 0xE1, 0x71, 0x01, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB, 0xF1, 0x39, 0x7D, 0x01, 0xDD, 0xB7
        };

        volatile byte _received_byte;
        volatile uint8_t _brightness;
        volatile uint8_t _payload[11];
        uint8_t _prev_payload[11];
        //bit 0 is "bit count error" flag. bit 1 is "too many bytes error" flag, bit 2 is "too few bytes error" flag.
        volatile uint8_t _packet_error;
        volatile bool _data_is_output;

};
