#include "DSP_BASE.h"

void DSP::setRawPayload(const std::vector<uint8_t>& pl)
{
    _raw_payload_to_dsp = pl;
}

std::vector<uint8_t> DSP::getRawPayload()
{
    return _raw_payload_from_dsp;
}
