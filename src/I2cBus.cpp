#include "I2cBus.h"

bool I2cBus::read(uint8_t address, uint8_t* data, size_t count) {
    if (wire_.requestFrom(address, count) != count) return false;
    for (size_t i = 0; i < count; ++i) data[i] = wire_.read();
    return true;
}

bool I2cBus::write(uint8_t address, const uint8_t* data, size_t count) {
    wire_.beginTransmission(address);
    wire_.write(data, count);
    return wire_.endTransmission() == 0;
}

bool I2cBus::readRegisters(uint8_t address, uint8_t first, uint8_t* data, size_t count) {
    wire_.beginTransmission(address);
    wire_.write(first);
    if (wire_.endTransmission(false) != 0) return false;
    return read(address, data, count);
}

bool I2cBus::writeRegisters(uint8_t address, uint8_t first, const uint8_t* data, size_t count) {
    wire_.beginTransmission(address);
    wire_.write(first);
    wire_.write(data, count);
    return wire_.endTransmission() == 0;
}
