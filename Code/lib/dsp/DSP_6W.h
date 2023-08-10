#pragma once
#include <Arduino.h>
#include "enums.h"
#include "DSP_BASE.h"

class DSP_6W : public DSP
{
    /*These must be declared for the API to work*/
    public:
        DSP_6W(){};
        virtual ~DSP_6W(){}
        virtual void setup(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int dsp_audio_pin) = 0;
        virtual void stop() = 0;
        void pause_all(bool action) override {}
        void updateToggles();
        virtual void handleStates() = 0;

        /*Internal use*/
    protected:
        virtual bool getHasjets() = 0;
        virtual uint8_t getDGT1_IDX() = 0;
        virtual uint8_t getDGT2_IDX() = 0;
        virtual uint8_t getDGT3_IDX() = 0;
        virtual uint8_t getTMR2_IDX() = 0;
        virtual uint8_t getTMR2_BIT() = 0;
        virtual uint8_t getTMR1_IDX() = 0;
        virtual uint8_t getTMR1_BIT() = 0;
        virtual uint8_t getLCK_IDX() = 0;
        virtual uint8_t getLCK_BIT() = 0;
        virtual uint8_t getTMRBTNLED_IDX() = 0;
        virtual uint8_t getTMRBTNLED_BIT() = 0;
        virtual uint8_t getREDHTR_IDX() = 0;
        virtual uint8_t getREDHTR_BIT() = 0;
        virtual uint8_t getGRNHTR_IDX() = 0;
        virtual uint8_t getGRNHTR_BIT() = 0;
        virtual uint8_t getAIR_IDX() = 0;
        virtual uint8_t getAIR_BIT() = 0;
        virtual uint8_t getFLT_IDX() = 0;
        virtual uint8_t getFLT_BIT() = 0;
        virtual uint8_t getC_IDX() = 0;
        virtual uint8_t getC_BIT() = 0;
        virtual uint8_t getF_IDX() = 0;
        virtual uint8_t getF_BIT() = 0;
        virtual uint8_t getPWR_IDX() = 0;
        virtual uint8_t getPWR_BIT() = 0;
        virtual uint8_t getHJT_IDX() = 0;
        virtual uint8_t getHJT_BIT() = 0;
        virtual uint8_t getCS() = 0;
        virtual uint8_t getCLK() = 0;
        virtual uint8_t getDATA() = 0;
        virtual uint8_t getAUDIO() = 0;
        virtual uint8_t charTo7SegmCode(char c) = 0;
        /*Send payload to display*/
        virtual void uploadPayload(uint8_t brightness) = 0;
        virtual Buttons getPressedButton() = 0;

    protected:

    private:
        uint8_t _brightness;
        Buttons _prev_btn;
};
