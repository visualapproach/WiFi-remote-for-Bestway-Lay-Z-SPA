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
        virtual sToggles getStates() = 0;
        virtual void setStates(const sStates& display_states) = 0;
        void setRawPayload(const std::vector<uint8_t>& pl);
        std::vector<uint8_t> getRawPayload();
        String debug();

    protected:
        std::vector<uint8_t> _raw_payload_to_dsp = {0,0,0,0,0,0,0};
        std::vector<uint8_t> _raw_payload_from_dsp = {0,0,0,0,0,0,0};
        uint32_t good_packets_count = 0;
};
