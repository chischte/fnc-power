#include "SerialClock.h"
#include "ClockTimeText.h"
#include <string.h>

bool SerialClock::execute() {
    if (strcmp(buffer_, "TIME?") == 0) {
        ClockTime time;
        if (!clock_.read(time)) {
            Serial.println(F("ERR RTC invalid; set TIME"));
            return false;
        }
        char result[24];
        snprintf(result, sizeof(result), "20%02u-%02u-%02u %02u:%02u:%02u",
                 time.year, time.month, time.day, time.hour, time.minute, time.second);
        Serial.println(result);
        return false;
    }
    ClockTime time;
    if (strncmp(buffer_, "TIME ", 5) != 0 || !parseClockTime(buffer_ + 5, time)) {
        Serial.println(F("ERR use TIME YYYY-MM-DD HH:MM:SS (2000-2099) or TIME?"));
        return false;
    }
    const bool success = clock_.set(time);
    Serial.println(success ? F("OK TIME") : F("ERR invalid date or RTC write failed"));
    return success;
}

bool SerialClock::poll() {
    // Limit work per loop so continuous serial traffic cannot starve the button.
    for (uint8_t consumed = 0; consumed < 32 && Serial.available(); ++consumed) {
        const char value = Serial.read();
        if (value == '\r') continue;
        if (value == '\n') {
            buffer_[length_] = '\0';
            bool changed = false;
            if (overflow_) Serial.println(F("ERR command too long"));
            else if (length_) changed = execute();
            length_ = 0;
            overflow_ = false;
            return changed;
        }
        if (length_ < sizeof(buffer_) - 1) buffer_[length_++] = value;
        else overflow_ = true;
    }
    return false;
}
