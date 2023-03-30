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

String CIO::debug()
{
    String s;
    for(int i = 0; i<7; i++) s += ' ' + String(_raw_payload_from_cio[i]);
    s += "msg count:" + String(good_packets_count);
    return s;
}
