#ifndef CANDOTIMING_H
#define CANDOTIMING_H

#include <stdint.h>
#include "cando.h"

class CandoTiming
{
public:
    CandoTiming(
        uint32_t baseClk,
        uint32_t bitrate,
        uint32_t samplePoint,
        uint32_t brp,
        uint32_t phase_seg1,
        uint32_t phase_seg2
    );

    uint32_t getBaseClk() const;
    uint32_t getBitrate() const;
    uint32_t getSamplePoint() const;
    cando_bittiming_t getTiming() const;

private:
    uint32_t _baseClk;
    uint32_t _bitrate;
    uint32_t _samplePoint;
    cando_bittiming_t _timing;
};

#endif // CandoTiming_H
