#pragma once
#include "ClockTime.h"

inline bool parseClockTime(const char* text, ClockTime& time) {
    const char pattern[] = "2000-00-00 00:00:00";
    for (unsigned i = 0; i < sizeof(pattern) - 1; ++i) {
        const bool digit = pattern[i] == '0' && i != 1;
        if (digit ? text[i] < '0' || text[i] > '9' : text[i] != pattern[i]) return false;
    }
    if (text[sizeof(pattern) - 1] != '\0') return false;
    const auto pair = [text](unsigned i) -> uint8_t {
        return (text[i] - '0') * 10 + text[i + 1] - '0';
    };
    ClockTime parsed = {pair(2), pair(5), pair(8), 0, pair(11), pair(14), pair(17)};
    if (!validClockTime(parsed)) return false;
    // 2000-01-01 was Saturday; ClockTime uses Sunday = 0.
    unsigned days = 0;
    for (unsigned year = 0; year < parsed.year; ++year) days += year % 4 == 0 ? 366 : 365;
    const uint8_t monthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    for (unsigned month = 1; month < parsed.month; ++month)
        days += monthDays[month - 1] + (month == 2 && parsed.year % 4 == 0);
    parsed.weekday = (6 + days + parsed.day - 1) % 7;
    time = parsed;
    return true;
}
