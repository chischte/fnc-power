#include <Arduino.h>
#include <Controllino.h>
#include <EEPROM.h>
#include <Control.h>
#include "RtcClock.h"
#include "SerialClock.h"

namespace {
constexpr uint8_t dimmerPin = CONTROLLINO_AO0;
constexpr uint8_t buttonPin = CONTROLLINO_A0;
constexpr int modeAddress = 0;
constexpr uint8_t disabledMarker = 0xA5;
constexpr uint8_t automaticMarker = 0x5A;
constexpr uint32_t clockPollMs = 200;

class Application {
public:
    void begin() {
        pinMode(dimmerPin, OUTPUT);
        analogWrite(dimmerPin, 0);
        pinMode(buttonPin, INPUT);
        Serial.begin(115200);
        control_ = LightControl(EEPROM.read(modeAddress) == disabledMarker);
        clock_.begin();
        refreshClock();
        Serial.println(F("fnc-power ready; TIME? or TIME YYYY-MM-DD HH:MM:SS"));
    }
    void update() {
        const uint32_t now = millis();
        if (serial_.poll() || uint32_t(now - clockAt_) >= clockPollMs) {
            clockAt_ = now;
            refreshClock();
        }
        const uint32_t longPressMs = control_.disabled()
            ? Config::enableLongPressMs : Config::disableLongPressMs;
        const ButtonEvent event = button_.update(digitalRead(buttonPin) == HIGH, now, longPressMs);
        control_.handle(event, now);
        if (event == ButtonEvent::longPress) {
            EEPROM.update(modeAddress, control_.disabled() ? disabledMarker : automaticMarker);
            Serial.println(control_.disabled() ? F("MODE OFF") : F("MODE AUTO"));
        }
        analogWrite(dimmerPin, control_.output(now, hour_, clockValid_, minute_));
    }
private:
    void refreshClock() {
        ClockTime time;
        const bool valid = clock_.read(time);
        if (valid != clockValid_ || !clockChecked_) {
            Serial.println(valid ? F("RTC OK") : F("ERR RTC invalid; schedule OFF until TIME is set"));
        }
        clockChecked_ = true;
        clockValid_ = valid;
        if (valid) {
            hour_ = time.hour;
            minute_ = time.minute;
        }
    }
    Button button_;
    LightControl control_;
    RtcClock clock_;
    SerialClock serial_{clock_};
    uint32_t clockAt_ = 0;
    uint8_t hour_ = 0;
    uint8_t minute_ = 0;
    bool clockValid_ = false;
    bool clockChecked_ = false;
};

Application& application() {
    // Arduino requires free setup/loop callbacks; keep shared state inside one owner.
    static Application instance;
    return instance;
}
}

void setup() { application().begin(); }
void loop() { application().update(); }
