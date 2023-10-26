#pragma once
#include <Arduino.h>
#include "enums.h"
#include "CIO_6W.h"

class CIO_6_TYPE2: public CIO_6W
{
    public:
        CIO_6_TYPE2();
        virtual ~CIO_6_TYPE2(){};
        void setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin) override;
        void stop() override;
        void pause_all(bool action) override;
        void IRAM_ATTR LED_Handler();
        void IRAM_ATTR clkHandler();
        void updateStates();

    protected:
        virtual uint16_t getButtonCode(Buttons button_index) = 0;
        virtual Buttons getButton(uint16_t code) = 0;
        virtual bool getHasjets() = 0;
        virtual bool getHasair() = 0;

    private:
        char _getChar(uint8_t value);

    private:
        volatile int _byte_count = 0;
        volatile int _bit_count = 0;
        volatile int _send_bit = 8;
        int _CIO_TD_PIN;
        int _CIO_CLK_PIN;
        int _CIO_LD_PIN;
        //LSB
        const uint16_t CLKPW = 50; //clock pulse period in us. clockfreq = 1/2*CLKPW
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

        volatile byte _received_byte;
        volatile uint8_t _brightness;
        volatile uint8_t _payload[5];
        uint8_t _prev_payload[5];
        uint8_t _received_cmd;  //temporary storage of command message
        volatile bool _packet = false;

};
