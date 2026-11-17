#pragma once
#include <stdint.h>

inline bool parseScheduleTime(const char* text, uint16_t& minutes) {
    if (!text) return false;
    for (unsigned i = 0; i < 5; ++i) {
        if (text[i] == 0) return false;
        if (i == 2) { if (text[i] != ':') return false; }
        else if (text[i] < '0' || text[i] > '9') return false;
    }
    if (text[5] != 0) return false;
    const int hour = (text[0] - '0') * 10 + text[1] - '0';
    const int minute = (text[3] - '0') * 10 + text[4] - '0';
    if (hour > 23 || minute > 59) return false;
    minutes = hour * 60 + minute;
    return true;
}
