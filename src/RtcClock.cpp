#include "RtcClock.h"
#include <Controllino.h>
#include <SPI.h>

namespace {
constexpr uint8_t controlRegister = 0x00;
constexpr uint8_t secondsRegister = 0x02;
constexpr uint8_t readCommand = 0x90;
constexpr uint8_t writeCommand = 0x10;
constexpr uint8_t stopClock = 0x20;
constexpr uint8_t unsupportedControl = 0xAC; // Test, STOP, 12-hour and override modes.
constexpr uint8_t oscillatorStopped = 0x80;
constexpr uint32_t spiFrequency = 1000000;
uint8_t bcd(uint8_t value) { return (value / 10) * 16 + value % 10; }
uint8_t decimal(uint8_t value) { return (value >> 4) * 10 + (value & 15); }
bool validBcd(uint8_t value) { return (value & 15) <= 9 && (value >> 4) <= 9; }
}

void RtcClock::begin() {
    // The library's RTC_init issues a software reset. Preserve the running clock instead.
    pinMode(CONTROLLINO_ETHERNET_CHIP_SELECT, OUTPUT);
    digitalWrite(CONTROLLINO_ETHERNET_CHIP_SELECT, HIGH);
    SPI.begin();
    Controllino_RTCSSInit();
    Controllino_SetRTCSS(LOW);
}

void RtcClock::transfer(uint8_t address, uint8_t* bytes, uint8_t count, bool reading) {
    SPI.beginTransaction(SPISettings(spiFrequency, MSBFIRST, SPI_MODE0));
    Controllino_SetRTCSS(HIGH);
    SPI.transfer((reading ? readCommand : writeCommand) | address);
    for (uint8_t i = 0; i < count; ++i) {
        const uint8_t received = SPI.transfer(reading ? 0 : bytes[i]);
        if (reading) bytes[i] = received;
    }
    Controllino_SetRTCSS(LOW);
    SPI.endTransaction();
}

bool RtcClock::valid(const ClockTime& time) {
    if (time.year > 99 || time.month < 1 || time.month > 12 || time.weekday > 6 ||
        time.hour > 23 || time.minute > 59 || time.second > 59) return false;
    const uint8_t monthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    const uint8_t days = monthDays[time.month - 1] + (time.month == 2 && time.year % 4 == 0);
    return time.day >= 1 && time.day <= days;
}

bool RtcClock::read(ClockTime& time) {
    uint8_t control;
    transfer(controlRegister, &control, 1, true);
    uint8_t bytes[7];
    // A burst keeps the calendar snapshot consistent across a second/day rollover.
    transfer(secondsRegister, bytes, sizeof(bytes), true);
    if ((control & unsupportedControl) || (bytes[0] & oscillatorStopped)) return false;
    for (uint8_t value : bytes) if (!validBcd(value)) return false;
    time = {decimal(bytes[6]), decimal(bytes[5]), decimal(bytes[3]),
            decimal(bytes[4]), decimal(bytes[2]), decimal(bytes[1]), decimal(bytes[0])};
    return valid(time);
}

bool RtcClock::set(const ClockTime& time) {
    if (!valid(time)) return false;
    uint8_t control = stopClock;
    transfer(controlRegister, &control, 1, false);
    uint8_t bytes[] = {bcd(time.second), bcd(time.minute), bcd(time.hour),
                      bcd(time.day), bcd(time.weekday), bcd(time.month), bcd(time.year)};
    transfer(secondsRegister, bytes, sizeof(bytes), false);
    control = 0; // Restart in 24-hour mode; writing seconds clears the oscillator-stop flag.
    transfer(controlRegister, &control, 1, false);
    ClockTime actual;
    return read(actual) && actual.year == time.year && actual.month == time.month &&
           actual.day == time.day && actual.hour == time.hour && actual.minute == time.minute;
}
