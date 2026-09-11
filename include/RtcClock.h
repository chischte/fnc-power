#pragma once
#include <stdint.h>

struct ClockTime {
    uint8_t year, month, day, weekday, hour, minute, second;
};

class RtcClock {
public:
    void begin();
    bool read(ClockTime& time);
    bool set(const ClockTime& time);
    static bool valid(const ClockTime& time);
private:
    void transfer(uint8_t address, uint8_t* bytes, uint8_t count, bool reading);
};
