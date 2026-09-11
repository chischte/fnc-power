#pragma once
#include "RtcClock.h"
#include <Arduino.h>

class SerialClock {
public:
    explicit SerialClock(RtcClock& clock) : clock_(clock) {}
    bool poll();
private:
    bool execute();
    RtcClock& clock_;
    char buffer_[32] = {};
    uint8_t length_ = 0;
    bool overflow_ = false;
};
