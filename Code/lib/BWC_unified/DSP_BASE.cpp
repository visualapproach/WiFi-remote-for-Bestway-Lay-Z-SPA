#include "DSP_BASE.h"

void DSP::setRawPayload(const std::vector<uint8_t>& pl)
{
    rawPayloadToDSP = pl;
}

std::vector<uint8_t> DSP::getRawPayload()
{
    return rawPayloadFromDSP;
}
