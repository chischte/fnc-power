#include <Arduino.h>
#include <Control.h>
#include "Dm4Hardware.h"
#include "ModeStore.h"
#include "NetworkServices.h"
#include "NetworkClock.h"
#include "RtcClock.h"
#include "SerialClock.h"
#include "StatusPage.h"
#include "DeviceSettings.h"
#include "ScheduleText.h"

namespace {
constexpr uint32_t clockPollMs = 200;
constexpr uint32_t inputPollMs = 5;
constexpr uint32_t hardwareRetryMs = 1000;
constexpr uint32_t rtcSetRetryMs = 30000;
static_assert(Config::outputRange == Dm4::dacMaximum, "Control and DAC ranges must match");

class Application {
public:
    void begin() {
        Serial.begin(115200);
        initializeHardware();
        if (!mode_.begin()) Serial.println("ERR NVS; mode changes cannot be saved");
        control_ = LightControl(mode_.disabled());
        settings_.begin();
        control_.setBrightness(settings_.brightness());
        control_.setSchedule(settings_.onTime(), settings_.offTime());
        refreshClock();
        network_.begin([this]() {
            button_ = Button();
            buttonArmed_ = false;
            return hardware_.writeOutput(0);
        }, settings_.hostname());
        statusPage_.begin([this]() { return status(); }, [this](const ClockTime& time) {
            const bool success = clock_.set(time);
            refreshClock();
            return success;
        }, [this](const String& command, const String& value) {
            return applyCommand(command, value);
        });
        Serial.println("fnc-power DM4 ready; TIME? or TIME YYYY-MM-DD HH:MM:SS");
    }

    void update() {
        network_.update();
        if (network_.updating()) return;
        const uint32_t now = millis();
        if (serial_.poll() || uint32_t(now - clockAt_) >= clockPollMs) {
            clockAt_ = now;
            refreshClock();
        }
        statusPage_.update();
        if (!hardwareReady_) {
            if (uint32_t(now - hardwareAt_) >= hardwareRetryMs) initializeHardware();
            return;
        }
        if (uint32_t(now - inputAt_) < inputPollMs) return;
        inputAt_ = now;
        updateButton(now);
        output_ = control_.output(now, hour_, clockValid_, minute_);
        const bool outputOk = hardware_.writeOutput(output_);
        reportChange(outputOk, outputOk_, "DAC OK", "ERR DAC; output may retain last voltage");
    }

private:
    bool applyCommand(const String& command, const String& value) {
        if (command == "schedule") {
            uint16_t on, off;
            if (value.length() != 11 || value[5] != ',' ||
                !parseScheduleTime(value.substring(0, 5).c_str(), on) ||
                !parseScheduleTime(value.substring(6).c_str(), off)) return false;
            if (!settings_.setSchedule(on, off)) return false;
            control_.setSchedule(on, off);
            return true;
        }
        if (command == "name") {
            if (!settings_.rename(value)) return false;
            network_.setHostname(settings_.hostname());
            return true;
        }
        if (command == "service" && (value == "on" || value == "off")) {
            control_.setService(value == "on", millis());
            return true;
        }
        if (command != "brightness" && command != "reset") return false;
        int percent = PowerConfig::brightnessPercent;
        if (command == "brightness") {
            if (value.isEmpty() || value.length() > 3) return false;
            for (unsigned i = 0; i < value.length(); ++i)
                if (value[i] < '0' || value[i] > '9') return false;
            percent = value.toInt();
        }
        if (!settings_.setBrightness(percent)) return false;
        control_.setBrightness(percent);
        return true;
    }

    DeviceStatus status() const {
        DeviceStatus result;
        result.hostname = settings_.hostname();
        result.brightness = settings_.brightness();
        result.time = clockTime_;
        result.clockValid = clockValid_;
        result.rtcValid = rtcValid_;
        result.rtcSetFromNetwork = rtcSetFromNetwork_;
        result.disabled = control_.disabled();
        result.service = control_.serviceActive();
        result.feedback = control_.feedbackActive();
        result.scheduledOn = clockValid_ && Config::isScheduledOn(
            hour_ * Config::minutesPerHour + minute_, settings_.onTime(), settings_.offTime());
        result.hardwareOk = hardwareReady_;
        result.outputOk = hardwareReady_ && outputOk_;
        result.output = output_;
        result.outputMaximum = Config::outputRange;
        result.onTime = settings_.onTime();
        result.offTime = settings_.offTime();
        return result;
    }

    void initializeHardware() {
        hardwareAt_ = millis();
        if (!busReady_) {
            busReady_ = Wire.begin(Dm4::sda, Dm4::scl, Dm4::i2cFrequency);
            Wire.setTimeOut(Dm4::i2cTimeoutMs);
        }
        hardwareReady_ = busReady_ && hardware_.begin();
        Serial.println(hardwareReady_ ? "DM4 I/O OK" : "ERR DM4 I/O; retry in 1 second");
    }

    void updateButton(uint32_t now) {
        bool pressed;
        const bool inputOk = hardware_.readButton(pressed);
        reportChange(inputOk, inputOk_, "INPUT OK", "ERR INPUT; waiting for valid released button");
        if (!inputOk) {
            button_ = Button();
            buttonArmed_ = false;
            return;
        }
        // A bus fault or held button at boot must not cause a synthetic press.
        if (!buttonArmed_) {
            buttonArmed_ = !pressed;
            return;
        }
        const uint32_t longPressMs = control_.disabled()
            ? Config::enableLongPressMs : Config::disableLongPressMs;
        const ButtonEvent event = button_.update(pressed, now, longPressMs);
        control_.handle(event, now);
        if (event == ButtonEvent::longPress) {
            if (!mode_.save(control_.disabled())) Serial.println("ERR NVS; mode was not saved");
            Serial.println(control_.disabled() ? "MODE OFF" : "MODE AUTO");
        }
    }

    void refreshClock() {
        ClockTime time;
        rtcValid_ = clock_.read(time);
        const bool networkValid = networkClock_.poll(!rtcValid_, network_.connected(), time);
        if (!rtcValid_ && networkValid &&
            (!rtcSetAttempted_ || uint32_t(millis() - rtcSetAt_) >= rtcSetRetryMs)) {
            rtcSetAttempted_ = true;
            rtcSetAt_ = millis();
            rtcValid_ = clock_.set(time);
            if (rtcValid_) {
                rtcSetFromNetwork_ = true;
                Serial.println("RTC set from NTP; RTC is now the primary clock");
            } else Serial.println("ERR RTC write; using NTP clock, retry in 30 seconds");
        }
        if (rtcValid_) rtcSetAttempted_ = false;
        const bool valid = rtcValid_ || networkValid;
        if (valid != clockValid_ || !clockChecked_)
            Serial.println(valid ? "TIME OK" : "TIME unknown; automatic light stays ON, waiting for NTP");
        clockChecked_ = true;
        clockValid_ = valid;
        if (valid) {
            clockTime_ = time;
            hour_ = time.hour;
            minute_ = time.minute;
        }
    }

    static void reportChange(bool ok, bool& previous, const char* recovery, const char* error) {
        if (ok != previous) Serial.println(ok ? recovery : error);
        previous = ok;
    }

    I2cBus bus_{Wire};
    Dm4Hardware hardware_{Wire, bus_};
    RtcClock clock_{bus_};
    SerialClock serial_{clock_};
    ModeStore mode_;
    DeviceSettings settings_;
    NetworkServices network_;
    NetworkClock networkClock_;
    StatusPage statusPage_;
    ClockTime clockTime_{};
    Button button_;
    LightControl control_;
    uint32_t clockAt_ = 0;
    uint32_t inputAt_ = 0;
    uint32_t hardwareAt_ = 0;
    uint32_t rtcSetAt_ = 0;
    uint8_t hour_ = 0;
    uint8_t minute_ = 0;
    uint16_t output_ = 0;
    bool clockValid_ = false;
    bool rtcValid_ = false;
    bool rtcSetAttempted_ = false;
    bool rtcSetFromNetwork_ = false;
    bool clockChecked_ = false;
    bool hardwareReady_ = false;
    bool busReady_ = false;
    bool buttonArmed_ = false;
    bool inputOk_ = true;
    bool outputOk_ = true;
};

Application& application() {
    // Arduino requires free setup/loop callbacks; keep state inside one owner.
    static Application instance;
    return instance;
}
}

void setup() { application().begin(); }
void loop() { application().update(); }
