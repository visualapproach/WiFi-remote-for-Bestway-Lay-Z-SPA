#include "CIO_BASE.h"

void CIO::setRawPayload(const std::vector<uint8_t>& pl)
{
    _raw_payload_to_cio = pl;

    // Serial.print("CIO:");
    // for(auto i : pl) Serial.printf(" %d",i);
    // Serial.println();
}

std::vector<uint8_t> CIO::getRawPayload()
{
    return _raw_payload_from_cio;
}
