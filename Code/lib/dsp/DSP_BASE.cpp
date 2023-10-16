#include "DSP_BASE.h"

void DSP::setRawPayload(const std::vector<uint8_t>& pl)
{
    _raw_payload_to_dsp = pl;

    // Serial.print("DSP:");
    // for(auto i : pl) Serial.printf(" %d",i);
    // Serial.println();
}

std::vector<uint8_t> DSP::getRawPayload()
{
    return _raw_payload_from_dsp;
}

String DSP::debug()
{
    String s;
    s.reserve(128);
    for(int i = 0; i<7; i++) s += ' ' + String(_raw_payload_from_dsp[i]);
    s += F("dsp msgs:");
    s += String(good_packets_count);
    return s;
}
