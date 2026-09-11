#pragma once
#include <stdint.h>

namespace Config {
constexpr uint32_t debounceMs = 30;
constexpr uint32_t longPressMs = 2000;
constexpr uint32_t serviceMs = 10UL * 60 * 1000;
constexpr uint32_t fadeMs = 3000;
constexpr uint8_t nightPercent = 17;
constexpr uint8_t startHour = 20;
constexpr uint8_t endHour = 8;
constexpr uint8_t fullOutput = 255;
constexpr uint8_t nightOutput = (fullOutput * uint16_t(nightPercent) + 50) / 100;
}

enum class ButtonEvent { none, shortPress, longPress };

class Button {
public:
    ButtonEvent update(bool pressed, uint32_t now) {
        if (pressed != raw_) {
            raw_ = pressed;
            changedAt_ = now;
        }
        if (raw_ != stable_ && uint32_t(now - changedAt_) >= Config::debounceMs) {
            stable_ = raw_;
            if (stable_) {
                pressedAt_ = now;
                longReported_ = false;
            } else if (!longReported_) {
                return ButtonEvent::shortPress;
            }
        }
        if (stable_ && raw_ && !longReported_ &&
            uint32_t(now - pressedAt_) >= Config::longPressMs) {
            longReported_ = true;
            return ButtonEvent::longPress;
        }
        return ButtonEvent::none;
    }
private:
    bool raw_ = false;
    bool stable_ = false;
    bool longReported_ = false;
    uint32_t changedAt_ = 0;
    uint32_t pressedAt_ = 0;
};

class LightControl {
public:
    explicit LightControl(bool disabled = false) : disabled_(disabled) {}
    void handle(ButtonEvent event, uint32_t now) {
        if (event == ButtonEvent::longPress) {
            disabled_ = !disabled_;
            service_ = false;
            fading_ = true;
            startedAt_ = now;
        } else if (event == ButtonEvent::shortPress && !disabled_ && !fading_) {
            service_ = true;
            serviceAt_ = now;
        }
    }
    uint8_t output(uint32_t now, uint8_t hour, bool clockValid) {
        if (fading_) {
            const uint32_t elapsed = now - startedAt_;
            if (elapsed <= Config::fadeMs) {
                const uint8_t level = elapsed * Config::fullOutput / Config::fadeMs;
                return disabled_ ? Config::fullOutput - level : level;
            }
            fading_ = false;
        }
        if (disabled_) return 0;
        if (service_ && uint32_t(now - serviceAt_) >= Config::serviceMs) service_ = false;
        if (service_) return Config::fullOutput;
        if (!clockValid || hour > 23) return 0;
        return hour >= Config::startHour || hour < Config::endHour ? Config::nightOutput : 0;
    }
    bool disabled() const { return disabled_; }
private:
    bool disabled_;
    bool service_ = false;
    bool fading_ = false;
    uint32_t startedAt_ = 0;
    uint32_t serviceAt_ = 0;
};
