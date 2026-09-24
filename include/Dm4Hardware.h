#pragma once
#include <Adafruit_MCP4728.h>
#include "Dm4Board.h"
#include "I2cBus.h"

class Dm4Hardware {
public:
    Dm4Hardware(TwoWire& wire, I2cBus& bus) : wire_(wire), bus_(bus) {}
    bool begin();
    bool readButton(bool& pressed);
    bool writeOutput(uint16_t value);
private:
    TwoWire& wire_;
    I2cBus& bus_;
    Adafruit_MCP4728 dac_;
    uint16_t lastOutput_ = UINT16_MAX;
    bool ready_ = false;
};
