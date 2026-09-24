#pragma once
#include "ClockTime.h"

namespace RtcRegisters {
constexpr uint8_t seconds = 0x00;
constexpr uint8_t control = 0x0e;
constexpr uint8_t status = 0x0f;
constexpr uint8_t oscillatorDisabled = 0x80;
constexpr uint8_t oscillatorStopped = 0x80;
inline uint8_t bcd(uint8_t value) { return (value / 10) * 16 + value % 10; }
inline uint8_t decimal(uint8_t value) { return (value >> 4) * 10 + (value & 15); }
inline bool validBcd(uint8_t value) { return (value & 15) <= 9 && (value >> 4) <= 9; }

inline bool decode(const uint8_t* bytes, ClockTime& time) {
    const bool twelveHour = (bytes[2] & 0x40) != 0;
    const uint8_t hour = bytes[2] & (twelveHour ? 0x1f : 0x3f);
    // Reject the century bit: the serial protocol supports 2000 through 2099.
    if ((bytes[0] & 0x80) || (bytes[1] & 0x80) || (bytes[2] & 0x80) ||
        (bytes[5] & 0xe0) || !validBcd(hour)) return false;
    for (uint8_t i = 0; i < 7; ++i) {
        if (i != 2 && !validBcd(bytes[i])) return false;
    }
    uint8_t hours = decimal(hour);
    if (twelveHour) {
        if (hours < 1 || hours > 12) return false;
        hours = hours % 12 + ((bytes[2] & 0x20) ? 12 : 0);
    }
    if (bytes[3] < 1 || bytes[3] > 7) return false;
    const ClockTime decoded = {decimal(bytes[6]), decimal(bytes[5]), decimal(bytes[4]),
                              uint8_t(bytes[3] - 1), hours, decimal(bytes[1]), decimal(bytes[0])};
    if (!validClockTime(decoded)) return false;
    time = decoded;
    return true;
}

inline void encode(const ClockTime& time, uint8_t* bytes) {
    bytes[0] = bcd(time.second);
    bytes[1] = bcd(time.minute);
    bytes[2] = bcd(time.hour);
    bytes[3] = time.weekday + 1;
    bytes[4] = bcd(time.day);
    bytes[5] = bcd(time.month);
    bytes[6] = bcd(time.year);
}
}
