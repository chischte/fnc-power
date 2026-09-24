#pragma once
#include <stdint.h>

struct ClockTime {
    uint8_t year, month, day, weekday, hour, minute, second;
};

inline bool validClockTime(const ClockTime& time) {
    if (time.year > 99 || time.month < 1 || time.month > 12 || time.weekday > 6 ||
        time.hour > 23 || time.minute > 59 || time.second > 59) return false;
    const uint8_t monthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    const uint8_t days = monthDays[time.month - 1] + (time.month == 2 && time.year % 4 == 0);
    return time.day >= 1 && time.day <= days;
}
