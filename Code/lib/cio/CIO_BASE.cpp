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
    // s = "QLen: " + String(_button_que_len);
    for(unsigned int i = 0; i < _raw_payload_from_cio.size(); i++) s += ' ' + String(_raw_payload_from_cio[i]);
    s += F(" good pkt:") + String(good_packets_count);
    return s;
}
