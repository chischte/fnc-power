#include "RtcClock.h"
#include "Dm4Board.h"
#include "RtcRegisters.h"

bool RtcClock::read(ClockTime& time) {
    uint8_t flags[2];
    if (!bus_.readRegisters(Dm4::rtcAddress, RtcRegisters::control, flags, sizeof(flags))) return false;
    if ((flags[0] & RtcRegisters::oscillatorDisabled) ||
        (flags[1] & RtcRegisters::oscillatorStopped)) return false;
    uint8_t bytes[7];
    // A single burst reads the DS3231's buffered calendar coherently.
    return bus_.readRegisters(Dm4::rtcAddress, RtcRegisters::seconds, bytes, sizeof(bytes)) &&
           RtcRegisters::decode(bytes, time);
}

bool RtcClock::set(const ClockTime& time) {
    if (!valid(time)) return false;
    uint8_t flags[2];
    if (!bus_.readRegisters(Dm4::rtcAddress, RtcRegisters::control, flags, sizeof(flags))) return false;
    const uint8_t control = flags[0] & ~RtcRegisters::oscillatorDisabled;
    if (!bus_.writeRegisters(Dm4::rtcAddress, RtcRegisters::control, &control, 1)) return false;
    uint8_t bytes[7];
    RtcRegisters::encode(time, bytes);
    if (!bus_.writeRegisters(Dm4::rtcAddress, RtcRegisters::seconds, bytes, sizeof(bytes))) return false;
    const uint8_t status = flags[1] & ~RtcRegisters::oscillatorStopped;
    if (!bus_.writeRegisters(Dm4::rtcAddress, RtcRegisters::status, &status, 1)) return false;
    ClockTime actual;
    return read(actual) && actual.year == time.year && actual.month == time.month &&
           actual.day == time.day && actual.hour == time.hour && actual.minute == time.minute;
}
