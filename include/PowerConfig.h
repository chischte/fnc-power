#pragma once

// Nach Änderungen neu bauen und auf den CONTROLLINO hochladen.
namespace PowerConfig {
// Absolute Prozentwerte des 0–10-V-Ausgangs; 100 % entsprechen 10 V.
constexpr int brightnessPercent = 17;
// Obergrenze für Automatik, Servicelicht und Taster-Rückmeldungen.
constexpr int maxBrightnessPercent = 100;

// Lokale RTC-Uhrzeit, 24-Stunden-Format. Gleiche Zeiten bedeuten immer Aus.
constexpr int onHour = 20;
constexpr int onMinute = 0;
constexpr int offHour = 8;
constexpr int offMinute = 0;

static_assert(brightnessPercent >= 0 && brightnessPercent <= 100,
              "brightnessPercent must be between 0 and 100");
static_assert(maxBrightnessPercent >= 0 && maxBrightnessPercent <= 100,
              "maxBrightnessPercent must be between 0 and 100");
static_assert(onHour >= 0 && onHour <= 23 && offHour >= 0 && offHour <= 23,
              "Hours must be between 0 and 23");
static_assert(onMinute >= 0 && onMinute <= 59 && offMinute >= 0 && offMinute <= 59,
              "Minutes must be between 0 and 59");
}
