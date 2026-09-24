#pragma once

// Nach Änderungen neu bauen und auf den KinCony DM4 hochladen.
namespace PowerConfig {
// Absolute Prozentwerte des 0–10-V-Ausgangs; 100 % entsprechen 10 V.
// Minimale Beleuchtungsstärke bei verschiedenen Prozentwerten (in der Box gemessen):
//   % | min lux
// ----|--------
//  17 |      44
//  30 |     260
//  35 |     369
//  40 |     510
//  50 |     800
// Ziel: mindestens 300 lux; bei 35 % mit 369 lux erreicht.
constexpr int brightnessPercent = 35;
// Helligkeit bei kurzem Tastendruck, ebenfalls durch das Maximum begrenzt.
constexpr int serviceBrightnessPercent = 80;
// Obergrenze für Automatik, Servicelicht und Taster-Rückmeldungen.
constexpr int maxBrightnessPercent = 100;

// Lokale RTC-Uhrzeit, 24-Stunden-Format. Gleiche Zeiten bedeuten immer Aus.
constexpr int onHour = 20;
constexpr int onMinute = 0;
constexpr int offHour = 8;
constexpr int offMinute = 0;

static_assert(brightnessPercent >= 0 && brightnessPercent <= 100,
              "brightnessPercent must be between 0 and 100");
static_assert(serviceBrightnessPercent >= 0 && serviceBrightnessPercent <= 100,
              "serviceBrightnessPercent must be between 0 and 100");
static_assert(maxBrightnessPercent >= 0 && maxBrightnessPercent <= 100,
              "maxBrightnessPercent must be between 0 and 100");
static_assert(onHour >= 0 && onHour <= 23 && offHour >= 0 && offHour <= 23,
              "Hours must be between 0 and 23");
static_assert(onMinute >= 0 && onMinute <= 59 && offMinute >= 0 && offMinute <= 59,
              "Minutes must be between 0 and 59");
}
