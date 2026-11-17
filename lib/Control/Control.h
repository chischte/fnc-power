#pragma once
#include <stdint.h>
#include <UserConfig.h>

namespace Config {
constexpr uint32_t debounceMs = 30;
constexpr uint32_t enableLongPressMs = 750;
constexpr uint32_t disableLongPressMs = 1250;
constexpr uint32_t serviceFadeMs = 5000;
constexpr uint32_t serviceMs = 10UL * 60 * 1000;
constexpr uint32_t fadeMs = 5000;
constexpr uint32_t feedbackTransitionMs = 400;
constexpr uint32_t feedbackBlinkMs = 300;
constexpr uint8_t feedbackBlinkCount = 3;
constexpr uint32_t feedbackBlinkSequenceMs = feedbackBlinkMs * (1 + 2 * feedbackBlinkCount);
constexpr uint32_t automaticFadeMs = 60000;
constexpr uint16_t minutesPerHour = 60;
constexpr uint16_t outputRange = 4095;
constexpr uint16_t percentOutput(int percent, int maximum) {
    return (uint32_t(outputRange) * uint16_t(percent < maximum ? percent : maximum) + 50) / 100;
}
constexpr uint16_t serviceOutput = percentOutput(PowerConfig::serviceBrightnessPercent,
                                               PowerConfig::maxBrightnessPercent);
constexpr uint16_t fullOutput = percentOutput(PowerConfig::maxBrightnessPercent, 100);
constexpr uint16_t nightOutput = percentOutput(PowerConfig::brightnessPercent,
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
    ButtonEvent update(bool pressed, uint32_t now,
                       uint32_t longPressMs = Config::disableLongPressMs) {
        if (pressed != raw_) {
            raw_ = pressed;
            changedAt_ = now;
        }
        if (raw_ != stable_ && uint32_t(now - changedAt_) >= Config::debounceMs) {
            stable_ = raw_;
            if (stable_) {
                pressedAt_ = now;
                longPressMs_ = longPressMs;
                longReported_ = false;
            } else if (!longReported_) {
                return ButtonEvent::shortPress;
            }
        }
        if (stable_ && raw_ && !longReported_ &&
            uint32_t(now - pressedAt_) >= longPressMs_) {
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
    uint32_t longPressMs_ = Config::disableLongPressMs;
};

class AutomaticLight {
public:
    void setBrightness(uint16_t value) { brightness_ = value; }
    void setSchedule(uint16_t on, uint16_t off) { onTime_ = on; offTime_ = off; }
    uint16_t output(uint32_t now, uint8_t hour, bool clockValid, uint8_t minute = 0) {
        if (!clockValid || hour > 23 || minute > 59) {
            initialized_ = false;
            return brightness_;
        }
        const uint16_t time = hour * Config::minutesPerHour + minute;
        const uint16_t target = Config::isScheduledOn(time, onTime_, offTime_)
            ? brightness_ : 0;
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
    uint16_t level(uint32_t now) const {
        if (from_ == target_) return target_;
        const uint32_t elapsed = now - startedAt_;
        if (elapsed >= Config::automaticFadeMs) return target_;
        const int32_t difference = int32_t(target_) - from_;
        return from_ + difference * int32_t(elapsed) / int32_t(Config::automaticFadeMs);
    }
    uint16_t onTime_ = Config::onTime;
    uint16_t offTime_ = Config::offTime;
    uint16_t brightness_ = Config::nightOutput;
    bool initialized_ = false;
    uint16_t from_ = 0;
    uint16_t target_ = 0;
    uint32_t startedAt_ = 0;
};

class FeedbackRamp {
public:
    void start(bool disabling, uint16_t currentOutput, uint32_t now) {
        disabling_ = disabling;
        from_ = currentOutput;
        startedAt_ = now;
        active_ = true;
        preparationMs_ = !disabling && currentOutput != 0 ? Config::feedbackTransitionMs : 0;
    }
    bool active() const { return active_; }
    void cancel() { active_ = false; }
    uint16_t output(uint32_t now, uint16_t automaticOutput) {
        uint32_t elapsed = now - startedAt_;
        const uint16_t initial = disabling_ ? Config::fullOutput : 0;
        if (elapsed < preparationMs_)
            return interpolate(from_, initial, elapsed, preparationMs_);
        elapsed -= preparationMs_;
        const uint16_t final = disabling_ ? 0 : Config::fullOutput;
        if (elapsed < Config::fadeMs)
            return interpolate(initial, final, elapsed, Config::fadeMs);
        elapsed -= Config::fadeMs;
        if (!disabling_) {
            // Hold the ramp peak, then show distinct off/on pulses.
            if (elapsed < Config::feedbackBlinkSequenceMs)
                return (elapsed / Config::feedbackBlinkMs) % 2 == 0 ? Config::fullOutput : 0;
            elapsed -= Config::feedbackBlinkSequenceMs;
        }
        if (!disabling_ && elapsed < Config::feedbackTransitionMs)
            return interpolate(final, automaticOutput, elapsed, Config::feedbackTransitionMs);
        active_ = false;
        return disabling_ ? 0 : automaticOutput;
    }
private:
    static uint16_t interpolate(uint16_t from, uint16_t to, uint32_t elapsed, uint32_t duration) {
        const int32_t difference = int32_t(to) - from;
        return from + difference * int32_t(elapsed) / int32_t(duration);
    }
    bool active_ = false;
    bool disabling_ = false;
    uint16_t from_ = 0;
    uint32_t startedAt_ = 0;
    uint32_t preparationMs_ = 0;
};

class LightControl {
public:
    explicit LightControl(bool disabled = false) : disabled_(disabled) {}
    void handle(ButtonEvent event, uint32_t now) {
        if (event == ButtonEvent::longPress) {
            disabled_ = !disabled_;
            service_ = false;
            feedback_.start(disabled_, lastOutput_, now);
        } else if (event == ButtonEvent::shortPress) {
            feedback_.cancel();
            service_ = !service_ || uint32_t(now - serviceAt_) >= Config::serviceMs;
            if (service_) {
                serviceAt_ = now;
                serviceFrom_ = lastOutput_;
            }
        }
    }
    uint16_t output(uint32_t now, uint8_t hour, bool clockValid, uint8_t minute = 0) {
        // Keep the schedule progressing while service light or manual feedback overrides it.
        const uint16_t automaticOutput = automatic_.output(now, hour, clockValid, minute);
        lastOutput_ = currentOutput(now, automaticOutput);
        return lastOutput_;
    }
    void setSchedule(uint16_t on, uint16_t off) { automatic_.setSchedule(on, off); }
    void setBrightness(int percent) {
        automatic_.setBrightness(Config::percentOutput(percent, PowerConfig::maxBrightnessPercent));
    }
    void setService(bool enabled, uint32_t now) {
        if (service_ != enabled) handle(ButtonEvent::shortPress, now);
    }
    bool disabled() const { return disabled_; }
    bool serviceActive() const { return service_; }
    bool feedbackActive() const { return feedback_.active(); }
private:
    uint16_t currentOutput(uint32_t now, uint16_t automaticOutput) {
        if (feedback_.active()) return feedback_.output(now, automaticOutput);
        if (service_ && uint32_t(now - serviceAt_) >= Config::serviceMs) service_ = false;
        if (service_) return serviceLevel(now);
        return disabled_ ? 0 : automaticOutput;
    }
    uint16_t serviceLevel(uint32_t now) const {
        const uint32_t elapsed = now - serviceAt_;
        if (elapsed >= Config::serviceFadeMs) return Config::serviceOutput;
        const int32_t difference = int32_t(Config::serviceOutput) - serviceFrom_;
        return serviceFrom_ + difference * int32_t(elapsed) / int32_t(Config::serviceFadeMs);
    }
    AutomaticLight automatic_;
    FeedbackRamp feedback_;
    bool disabled_;
    bool service_ = false;
    uint16_t lastOutput_ = 0;
    uint16_t serviceFrom_ = 0;
    uint32_t serviceAt_ = 0;
};
