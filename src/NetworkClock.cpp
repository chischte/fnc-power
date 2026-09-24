#include "NetworkClock.h"
#include <Arduino.h>
#include <esp_sntp.h>
#include <time.h>

namespace {
// Europe/Zurich: CET in winter, CEST in summer.
constexpr char localTimezone[] = "CET-1CEST,M3.5.0,M10.5.0/3";
}

bool NetworkClock::poll(bool needed, bool connected, ClockTime& result) {
    if (!needed) {
        if (started_) esp_sntp_stop();
        started_ = synchronized_ = false;
        return false;
    }
    if (!started_) {
        if (!connected) return false;
        esp_sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
        configTzTime(localTimezone, "ch.pool.ntp.org", "pool.ntp.org", "time.cloudflare.com");
        started_ = true;
    }
    if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) synchronized_ = true;
    // A plausible ESP32 system clock alone is not proof of a fresh NTP reply.
    if (!synchronized_) return false;
    const time_t now = time(nullptr);
    tm local{};
    if (!localtime_r(&now, &local) || local.tm_year < 100 || local.tm_year > 199) return false;
    const ClockTime current = {uint8_t(local.tm_year - 100), uint8_t(local.tm_mon + 1),
        uint8_t(local.tm_mday), uint8_t(local.tm_wday), uint8_t(local.tm_hour),
        uint8_t(local.tm_min), uint8_t(local.tm_sec)};
    if (!validClockTime(current)) return false;
    result = current;
    return true;
}
