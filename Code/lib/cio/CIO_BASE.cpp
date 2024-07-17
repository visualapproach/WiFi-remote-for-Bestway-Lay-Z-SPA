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
    s.reserve(128);
    // s = "QLen: " + String(_button_que_len);
    for(unsigned int i = 0; i < _raw_payload_from_cio.size(); i++) s += ' ' + String(_raw_payload_from_cio[i]);
    s += F(" cio msgs:");
    s += String(good_packets_count);
    return s;
}

int CIO::getHeaterPower()
{
    return
        (_heater_stages.stage1_on ? 1:0) * _power_levels.HEATERPOWER_STAGE1 +
        (_heater_stages.stage2_on ? 1:0) * _power_levels.HEATERPOWER_STAGE2;

}

void CIO::setPowerLevels(const std::optional<const Power>& power_levels) {
    if (power_levels.has_value()) {
        _power_levels = power_levels.value();
    } else {
        CIO::setPowerLevels(_default_power_levels);
    }
}
