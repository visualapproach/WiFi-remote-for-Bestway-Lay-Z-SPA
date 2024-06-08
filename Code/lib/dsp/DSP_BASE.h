#pragma once
#include <Arduino.h>
#include "enums.h"

class DSP
{
    public:
        DSP(){};
        virtual ~DSP(){};
        virtual void setup(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int dsp_audio_pin) = 0;
        virtual void stop() = 0;
        virtual void pause_all(bool action) = 0;
        virtual void updateToggles() = 0;
        virtual void handleStates() = 0;
        void setRawPayload(const std::vector<uint8_t>& pl);
        std::vector<uint8_t> getRawPayload();
        virtual bool getSerialReceived() {return false;} //"overridden" in DSP 4W
        virtual void setSerialReceived(bool txok) {}     //"overridden" in DSP 4W  
        String debug();

    public:
        sToggles dsp_toggles;
        sStates dsp_states;
        String text = "";
        std::vector<uint8_t> _raw_payload_to_dsp = {0,0,0,0,0,0,0,0,0,0,0};
        int audiofrequency = 0;
        uint32_t good_packets_count = 0;
        uint32_t bad_packets_count = 0;
        int write_msg_count = 0;

        /*
        Set to zero to disable chosen buttons.
        Order: NOBTN,LOCK,TIMER,BUBBLES,UNIT,HEAT,PUMP,DOWN,UP,POWER,HYDROJETS
        Example: to disable UNIT and TIMER set to 1,1,0,1,0,1,1,1,1,1,1
        or use Buttons enum - "EnabledButtons[UNIT] = 0"
        */
        bool EnabledButtons[11] = {1,1,1,1,1,1,1,1,1,1,1};

    protected:
        std::vector<uint8_t> _raw_payload_from_dsp = {0,0,0,0,0,0,0,0,0,0,0};

};
