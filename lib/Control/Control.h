#pragma once
#include <stdint.h>
#include <PowerConfig.h>

namespace Config {
constexpr uint32_t debounceMs = 30;
constexpr uint32_t longPressMs = 1500;
constexpr uint32_t serviceMs = 10UL * 60 * 1000;
constexpr uint32_t fadeMs = 5000;
constexpr uint32_t automaticFadeMs = 60000;
constexpr uint16_t minutesPerHour = 60;
constexpr uint8_t outputRange = 255;
constexpr uint8_t percentOutput(int percent, int maximum) {
    return (outputRange * uint16_t(percent < maximum ? percent : maximum) + 50) / 100;
}
constexpr uint8_t fullOutput = percentOutput(PowerConfig::maxBrightnessPercent, 100);
constexpr uint8_t nightOutput = percentOutput(PowerConfig::brightnessPercent,
                                             PowerConfig::maxBrightnessPercent);
constexpr uint16_t onTime = PowerConfig::onHour * minutesPerHour + PowerConfig::onMinute;
constexpr uint16_t offTime = PowerConfig::offHour * minutesPerHour + PowerConfig::offMinute;

constexpr bool isScheduledOn(uint16_t time, uint16_t on, uint16_t off) {
    return on < off ? time >= on && time < off
                    : on > off && (time >= on || time < off);
}
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

class AutomaticLight {
public:
    uint8_t output(uint32_t now, uint8_t hour, bool clockValid, uint8_t minute = 0) {
        if (!clockValid || hour > 23 || minute > 59) {
            initialized_ = false;
            return 0;
        }
        const uint16_t time = hour * Config::minutesPerHour + minute;
        const uint8_t target = Config::isScheduledOn(time, Config::onTime, Config::offTime)
            ? Config::nightOutput : 0;
        if (!initialized_) {
            initialized_ = true;
            from_ = target_ = target;
            startedAt_ = now;
        }
        if (target != target_) {
            from_ = level(now);
            target_ = target;
            startedAt_ = now;
        }
        return level(now);
    }
private:
    uint8_t level(uint32_t now) const {
        if (from_ == target_) return target_;
        const uint32_t elapsed = now - startedAt_;
        if (elapsed >= Config::automaticFadeMs) return target_;
        const int32_t difference = int32_t(target_) - from_;
        return from_ + difference * int32_t(elapsed) / int32_t(Config::automaticFadeMs);
    }
    bool initialized_ = false;
    uint8_t from_ = 0;
    uint8_t target_ = 0;
    uint32_t startedAt_ = 0;
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
            service_ = !service_ || uint32_t(now - serviceAt_) >= Config::serviceMs;
            if (service_) serviceAt_ = now;
        }
    }
    uint8_t output(uint32_t now, uint8_t hour, bool clockValid, uint8_t minute = 0) {
        // Keep the schedule progressing while service light or manual feedback overrides it.
        const uint8_t automaticOutput = automatic_.output(now, hour, clockValid, minute);
        if (fading_) {
            const uint32_t elapsed = now - startedAt_;
            if (elapsed >= Config::fadeMs) {
                fading_ = false;
                return disabled_ ? 0 : Config::fullOutput;
            }
            const uint8_t level = elapsed * Config::fullOutput / Config::fadeMs;
            return disabled_ ? Config::fullOutput - level : level;
        }
        if (disabled_) return 0;
        if (service_ && uint32_t(now - serviceAt_) >= Config::serviceMs) service_ = false;
        if (service_) return Config::fullOutput;
        return automaticOutput;
    }
    bool disabled() const { return disabled_; }
private:
    AutomaticLight automatic_;
    bool disabled_;
    bool service_ = false;
    bool fading_ = false;
    uint32_t startedAt_ = 0;
    uint32_t serviceAt_ = 0;
};
