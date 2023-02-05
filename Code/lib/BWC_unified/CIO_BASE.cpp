#include "CIO_BASE.h"

void CIO::setRawPayload(const std::vector<uint8_t>& pl)
{
    _raw_payload_to_cio = pl;
}

std::vector<uint8_t> CIO::getRawPayload()
{
    return _raw_payload_from_cio;
}
