#include "Dm4Hardware.h"

bool Dm4Hardware::begin() {
    ready_ = false;
    if (!dac_.begin(Dm4::dacAddress, &wire_)) return false;
    // Match KinCony's ESPHome settings. Never store ramp values in DAC EEPROM.
    for (uint8_t channel = 0; channel < Dm4::outputChannels; ++channel) {
        if (!dac_.setChannelValue(static_cast<MCP4728_channel_t>(channel), 0,
                MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X)) return false;
    }
    const uint8_t inputs = 0xff; // PCF8574 pins must be high to act as inputs.
    if (!bus_.write(Dm4::inputAddress, &inputs, 1)) return false;
    lastOutput_ = 0;
    ready_ = true;
    return true;
}

bool Dm4Hardware::readButton(bool& pressed) {
    uint8_t inputs;
    if (!ready_ || !bus_.read(Dm4::inputAddress, &inputs, 1)) return false;
    constexpr uint8_t buttonMask = 1 << Dm4::buttonInputPin;
    pressed = (inputs & buttonMask) == 0;
    return true;
}

bool Dm4Hardware::writeOutput(uint16_t value) {
    if (!ready_ || value > Dm4::dacMaximum) return false;
    if (value == lastOutput_) return true;
    if (!dac_.setChannelValue(MCP4728_CHANNEL_D, value,
            MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X)) return false;
    lastOutput_ = value;
    return true;
}
