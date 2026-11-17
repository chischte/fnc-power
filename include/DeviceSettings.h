#pragma once
#include <Preferences.h>
#include <WiFi.h>
#include "NetworkConfig.h"
#include <UserConfig.h>
#include <Control.h>

class DeviceSettings {
public:
    void begin() {
        ready_ = storage_.begin("light-settings", false);
        char suffix[13];
        snprintf(suffix, sizeof(suffix), "%012llx", (unsigned long long)ESP.getEfuseMac());
        hostname_ = String(NetworkConfig::hostname).substring(0, 50) + "-" + suffix;
        hostname_.toLowerCase();
        if (ready_) hostname_ = storage_.getString("hostname", hostname_);
        brightness_ = ready_ ? storage_.getUInt("brightness", PowerConfig::brightnessPercent)
                             : PowerConfig::brightnessPercent;
        const uint32_t schedule = ready_ ? storage_.getUInt("schedule", defaultSchedule) : defaultSchedule;
        onTime_ = schedule >> 16;
        offTime_ = schedule & 0xffff;
        if (onTime_ >= minutesPerDay || offTime_ >= minutesPerDay) {
            onTime_ = Config::onTime;
            offTime_ = Config::offTime;
        }
        if (brightness_ > PowerConfig::maxBrightnessPercent) brightness_ = PowerConfig::brightnessPercent;
    }
    const String& hostname() const { return hostname_; }
    uint16_t onTime() const { return onTime_; }
    uint16_t offTime() const { return offTime_; }
    bool setSchedule(uint16_t on, uint16_t off) {
        if (!ready_ || on >= minutesPerDay || off >= minutesPerDay) return false;
        const uint32_t schedule = (uint32_t(on) << 16) | off;
        if (storage_.putUInt("schedule", schedule) != sizeof(uint32_t)) return false;
        onTime_ = on;
        offTime_ = off;
        return true;
    }
    int brightness() const { return brightness_; }
    bool rename(const String& name) {
        if (name.isEmpty() || name.length() > 63 || name[0] == '-' || name[name.length()-1] == '-') return false;
        for (unsigned i = 0; i < name.length(); ++i) {
            const char c = name[i];
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
        }
        if (!ready_ || storage_.putString("hostname", name) == 0) return false;
        hostname_ = name;
        return true;
    }
    bool setBrightness(int value) {
        if (value < 0 || value > PowerConfig::maxBrightnessPercent || !ready_) return false;
        if (storage_.putUInt("brightness", value) != sizeof(uint32_t)) return false;
        brightness_ = value;
        return true;
    }
private:
    static constexpr uint16_t minutesPerDay = 24 * Config::minutesPerHour;
    static constexpr uint32_t defaultSchedule = (uint32_t(Config::onTime) << 16) | Config::offTime;
    uint16_t onTime_ = Config::onTime;
    uint16_t offTime_ = Config::offTime;
    Preferences storage_;
    String hostname_;
    int brightness_ = PowerConfig::brightnessPercent;
    bool ready_ = false;
};
