#pragma once
#include "ClockTime.h"

class NetworkClock {
public:
    bool poll(bool needed, bool connected, ClockTime& time);
private:
    bool started_ = false;
    bool synchronized_ = false;
};
