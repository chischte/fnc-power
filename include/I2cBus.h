#pragma once
#include <Wire.h>

class I2cBus {
public:
    explicit I2cBus(TwoWire& wire) : wire_(wire) {}
    bool read(uint8_t address, uint8_t* data, size_t count);
    bool write(uint8_t address, const uint8_t* data, size_t count);
    bool readRegisters(uint8_t address, uint8_t first, uint8_t* data, size_t count);
    bool writeRegisters(uint8_t address, uint8_t first, const uint8_t* data, size_t count);
private:
    TwoWire& wire_;
};
