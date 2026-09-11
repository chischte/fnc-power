#include "SerialClock.h"
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
    // Fixed-width parsing rejects incomplete dates, signs and trailing garbage.
    const char pattern[] = "TIME 2000-00-00 00:00:00";
    if (length_ != sizeof(pattern) - 1) {
        Serial.println(F("ERR use TIME YYYY-MM-DD HH:MM:SS or TIME?"));
        return false;
    }
    for (uint8_t i = 0; i < length_; ++i) {
        const bool digit = pattern[i] == '0' && i != 6;
        if ((digit && (buffer_[i] < '0' || buffer_[i] > '9')) ||
            (!digit && buffer_[i] != pattern[i])) {
            Serial.println(F("ERR invalid TIME format (2000-2099)"));
            return false;
        }
    }
    const auto pair = [this](uint8_t i) -> uint8_t {
        return (buffer_[i] - '0') * 10 + buffer_[i + 1] - '0';
    };
    const ClockTime time = {pair(7), pair(10), pair(13), 0, pair(16), pair(19), pair(22)};
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
