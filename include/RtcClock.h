#pragma once
#include "ClockTime.h"
#include "I2cBus.h"

class RtcClock {
public:
    explicit RtcClock(I2cBus& bus) : bus_(bus) {}
    bool read(ClockTime& time);
    bool set(const ClockTime& time);
    static bool valid(const ClockTime& time) { return validClockTime(time); }
private:
    I2cBus& bus_;
};
