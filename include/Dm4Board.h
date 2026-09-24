#pragma once
#include <stdint.h>

namespace Dm4 {
constexpr uint8_t sda = 8;
constexpr uint8_t scl = 18;
constexpr uint32_t i2cFrequency = 100000;
constexpr uint16_t i2cTimeoutMs = 20;
constexpr uint8_t inputAddress = 0x24;
constexpr uint8_t buttonInputPin = 3; // IN4 maps to PCF8574 P3.
constexpr uint8_t dacAddress = 0x60;
constexpr uint8_t rtcAddress = 0x68;
constexpr uint16_t dacMaximum = 4095;
constexpr uint8_t outputChannels = 4;
constexpr int ethernetClock = 1;
constexpr int ethernetMosi = 2;
constexpr int ethernetMiso = 41;
constexpr int ethernetCs = 42;
constexpr int ethernetIrq = 43;
constexpr int ethernetReset = 44;
}
