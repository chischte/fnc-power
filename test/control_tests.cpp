#include <Control.h>
#include "ScheduleText.h"
#include "RtcRegisters.h"
#include "ClockTimeText.h"
#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (false)
#ifdef _WIN32
#define TEST extern "C" __declspec(dllexport) int
#else
#define TEST extern "C" int
#endif

TEST clock_text_validation() {
    ClockTime time{};
    CHECK(parseClockTime("2026-09-24 22:30:45", time));
    CHECK(time.year == 26 && time.month == 9 && time.day == 24 && time.weekday == 4);
    CHECK(time.hour == 22 && time.minute == 30 && time.second == 45);
    CHECK(parseClockTime("2000-01-01 00:00:00", time) && time.weekday == 6);
    CHECK(parseClockTime("2024-02-29 23:59:59", time) && time.weekday == 4);
    CHECK(!parseClockTime("2025-02-29 00:00:00", time));
    CHECK(!parseClockTime("2100-01-01 00:00:00", time));
    CHECK(!parseClockTime("2026-09-24 24:00:00", time));
    CHECK(!parseClockTime("2026-09-24 22:60:00", time));
    CHECK(!parseClockTime("2026-09-24 22:30:60", time));
    CHECK(!parseClockTime("2026-09-24 22:30:45x", time));
    CHECK(!parseClockTime("2026-09-24", time));
    CHECK(!parseClockTime("", time));
    CHECK(!parseClockTime("2026-0x-24 22:30:45", time));
    return 0;
}

TEST schedule() {
    LightControl light;
    for (uint8_t hour = 0; hour < 24; ++hour) {
        LightControl initial;
        CHECK(initial.output(0, hour, true) == ((hour < 8 || hour >= 20) ? 1433 : 0));
    }
    CHECK(light.output(0, 20, false) == Config::nightOutput);
    CHECK(light.output(0, 24, true) == Config::nightOutput);
    return 0;
}

TEST service_timeout() {
    LightControl light;
    light.handle(ButtonEvent::shortPress, 100);
    CHECK(light.output(100, 12, true) == 0);
    CHECK(light.output(600099, 12, true) == 4095);
    CHECK(light.output(600100, 12, true) == 0);
    light.handle(ButtonEvent::shortPress, 600200);
    CHECK(light.output(1100000, 20, true) == 4095);
    CHECK(light.output(1200200, 20, true) == 1433);
    return 0;
}

TEST service_toggle_and_boundaries() {
    LightControl light;
    light.handle(ButtonEvent::shortPress, 0);
    CHECK(light.output(100, 7, true) == 81);
    CHECK(light.output(200, 8, true) == 163);
    light.handle(ButtonEvent::shortPress, 500000);
    CHECK(light.output(500000, 8, true) == 0);
    CHECK(light.output(1100000, 8, true) == 0);
    light.handle(ButtonEvent::shortPress, 1200000);
    CHECK(light.output(1205000, 0, false) == 4095);
    CHECK(light.output(1800000, 0, false) == Config::nightOutput);
    return 0;
}

TEST disable_and_enable() {
    LightControl light;
    light.handle(ButtonEvent::shortPress, 0);
    CHECK(light.output(0, 20, true) == 0);
    light.handle(ButtonEvent::longPress, 100);
    CHECK(light.disabled());
    CHECK(light.output(100, 20, true) == 4095);
    CHECK(light.output(2600, 20, true) == 2048);
    CHECK(light.output(5100, 20, true) == 0);
    light.handle(ButtonEvent::shortPress, 6000);
    CHECK(light.output(6000, 20, true) == 0);
    light.handle(ButtonEvent::longPress, 7000);
    CHECK(!light.disabled());
    CHECK(light.output(7000, 20, true) == 0);
    CHECK(light.output(9500, 20, true) == 2047);
    CHECK(light.output(12000, 20, true) == 4095);
    CHECK(light.output(14300, 20, true) == 2764);
    CHECK(light.output(14500, 20, true) == 1433);
    LightControl restored(true);
    CHECK(restored.output(0, 20, true) == 0);
    return 0;
}

TEST enable_during_day() {
    LightControl light(true);
    light.handle(ButtonEvent::longPress, 0);
    CHECK(light.output(0, 12, true) == 0);
    CHECK(light.output(5000, 12, true) == 4095);
    CHECK(light.output(7300, 12, true) == 2048);
    CHECK(light.output(7500, 12, true) == 0);
    return 0;
}

TEST timer_rollover() {
    constexpr uint32_t start = UINT32_MAX - 1000;
    LightControl light;
    light.handle(ButtonEvent::shortPress, start);
    CHECK(light.output(uint32_t(start + 599999), 12, true) == 4095);
    CHECK(light.output(uint32_t(start + 600000), 12, true) == 0);
    light.handle(ButtonEvent::longPress, start);
    CHECK(light.output(start, 12, true) == 4095);
    CHECK(light.output(uint32_t(start + 2500), 12, true) == 2048);
    CHECK(light.output(uint32_t(start + 5000), 12, true) == 0);
    return 0;
}

TEST button_debounce() {
    Button button;
    CHECK(button.update(true, 0) == ButtonEvent::none);
    CHECK(button.update(false, 10) == ButtonEvent::none);
    CHECK(button.update(true, 20) == ButtonEvent::none);
    CHECK(button.update(true, 49) == ButtonEvent::none);
    CHECK(button.update(true, 50) == ButtonEvent::none);
    CHECK(button.update(false, 100) == ButtonEvent::none);
    CHECK(button.update(true, 110) == ButtonEvent::none);
    CHECK(button.update(false, 120) == ButtonEvent::none);
    CHECK(button.update(false, 149) == ButtonEvent::none);
    CHECK(button.update(false, 150) == ButtonEvent::shortPress);
    CHECK(button.update(false, 200) == ButtonEvent::none);
    return 0;
}

TEST button_long_once() {
    Button button;
    CHECK(button.update(true, 0) == ButtonEvent::none);
    CHECK(button.update(true, 30) == ButtonEvent::none);
    CHECK(button.update(true, 1279) == ButtonEvent::none);
    CHECK(button.update(true, 1280) == ButtonEvent::longPress);
    CHECK(button.update(true, 10000) == ButtonEvent::none);
    CHECK(button.update(false, 11000) == ButtonEvent::none);
    CHECK(button.update(false, 11030) == ButtonEvent::none);
    CHECK(button.update(true, 12000) == ButtonEvent::none);
    CHECK(button.update(true, 12030) == ButtonEvent::none);
    CHECK(button.update(true, 13280) == ButtonEvent::longPress);
    return 0;
}

TEST button_rollover() {
    Button button;
    constexpr uint32_t start = UINT32_MAX - 15;
    CHECK(button.update(true, start) == ButtonEvent::none);
    CHECK(button.update(true, uint32_t(start + 30)) == ButtonEvent::none);
    CHECK(button.update(true, uint32_t(start + 1280)) == ButtonEvent::longPress);
    CHECK(button.update(false, uint32_t(start + 2100)) == ButtonEvent::none);
    CHECK(button.update(false, uint32_t(start + 2130)) == ButtonEvent::none);
    return 0;
}

TEST automatic_ramps() {
    LightControl light;
    CHECK(light.output(0, 19, true) == 0);
    CHECK(light.output(100, 20, true) == 0);
    CHECK(light.output(30100, 20, true) == 716);
    CHECK(light.output(60099, 20, true) == 1432);
    CHECK(light.output(60100, 20, true) == 1433);
    CHECK(light.output(100000, 7, true) == 1433);
    CHECK(light.output(100100, 8, true) == 1433);
    CHECK(light.output(130100, 8, true) == 717);
    CHECK(light.output(160099, 8, true) == 1);
    CHECK(light.output(160100, 8, true) == 0);
    return 0;
}

TEST automatic_ramp_overrides() {
    LightControl light;
    CHECK(light.output(0, 19, true) == 0);
    light.handle(ButtonEvent::shortPress, 0);
    CHECK(light.output(580000, 20, true) == 4095);
    CHECK(light.output(600000, 20, true) == 477);
    CHECK(light.output(640000, 20, true) == 1433);
    light.handle(ButtonEvent::longPress, 650000);
    CHECK(light.output(650000, 20, true) == 4095);
    CHECK(light.output(652500, 20, true) == 2048);
    CHECK(light.output(655000, 20, true) == 0);
    CHECK(light.output(700000, 8, true) == 0);
    light.handle(ButtonEvent::longPress, 720000);
    CHECK(light.output(720000, 8, true) == 0);
    CHECK(light.output(725000, 8, true) == 4095);
    CHECK(light.output(730000, 8, true) == 717);
    CHECK(light.output(760000, 8, true) == 0);
    return 0;
}

TEST automatic_ramp_clock_changes() {
    LightControl light;
    CHECK(light.output(0, 19, true) == 0);
    CHECK(light.output(100, 20, true) == 0);
    CHECK(light.output(30100, 20, true) == 716);
    CHECK(light.output(30100, 12, true) == 716);
    CHECK(light.output(60100, 12, true) == 358);
    CHECK(light.output(90100, 12, true) == 0);
    CHECK(light.output(100000, 20, true) == 0);
    CHECK(light.output(130000, 20, false) == Config::nightOutput);
    CHECK(light.output(130100, 20, true) == 1433);
    return 0;
}

TEST automatic_ramp_rollover() {
    constexpr uint32_t start = UINT32_MAX - 1000;
    LightControl light;
    CHECK(light.output(start, 19, true) == 0);
    CHECK(light.output(start, 20, true) == 0);
    CHECK(light.output(uint32_t(start + 30000), 20, true) == 716);
    CHECK(light.output(uint32_t(start + 60000), 20, true) == 1433);
    CHECK(light.output(uint32_t(start + 70000), 8, true) == 1433);
    CHECK(light.output(uint32_t(start + 100000), 8, true) == 717);
    CHECK(light.output(uint32_t(start + 130000), 8, true) == 0);
    return 0;
}

TEST configurable_schedule() {
    CHECK(Config::isScheduledOn(390, 390, 1125));
    CHECK(!Config::isScheduledOn(389, 390, 1125));
    CHECK(Config::isScheduledOn(1124, 390, 1125));
    CHECK(!Config::isScheduledOn(1125, 390, 1125));
    CHECK(!Config::isScheduledOn(0, 390, 1125));
    CHECK(Config::isScheduledOn(0, 1230, 495));
    CHECK(!Config::isScheduledOn(1229, 1230, 495));
    CHECK(Config::isScheduledOn(1230, 1230, 495));
    CHECK(Config::isScheduledOn(494, 1230, 495));
    CHECK(!Config::isScheduledOn(495, 1230, 495));
    for (uint16_t minute = 0; minute < 1440; ++minute)
        CHECK(!Config::isScheduledOn(minute, 480, 480));
    LightControl light;
    CHECK(light.output(0, 20, true, 60) == Config::nightOutput);
    return 0;
}

TEST brightness_limit() {
    CHECK(Config::percentOutput(17, 100) == 696);
    CHECK(Config::percentOutput(17, 70) == 696);
    CHECK(Config::percentOutput(80, 70) == 2867);
    CHECK(Config::percentOutput(80, 100) == 3276);
    CHECK(Config::percentOutput(100, 100) == 4095);
    CHECK(Config::percentOutput(100, 0) == 0);
    CHECK(Config::percentOutput(0, 100) == 0);
    return 0;
}

TEST service_toggle_at_night() {
    LightControl light;
    CHECK(light.output(0, 20, true) == 1433);
    light.handle(ButtonEvent::shortPress, 100);
    CHECK(light.output(100, 20, true) == 1433);
    light.handle(ButtonEvent::shortPress, 200);
    CHECK(light.output(200, 20, true) == 1433);
    light.handle(ButtonEvent::shortPress, 300);
    CHECK(light.output(600299, 20, true) == 4095);
    CHECK(light.output(600300, 20, true) == 1433);
    return 0;
}

TEST service_toggle_during_ramp() {
    LightControl light;
    CHECK(light.output(0, 19, true) == 0);
    light.handle(ButtonEvent::shortPress, 100);
    CHECK(light.output(100, 20, true) == 0);
    light.handle(ButtonEvent::shortPress, 30100);
    CHECK(light.output(30100, 20, true) == 716);
    CHECK(light.output(60100, 20, true) == 1433);
    return 0;
}

TEST service_press_at_expiry() {
    constexpr uint32_t start = UINT32_MAX - 1000;
    LightControl light;
    light.handle(ButtonEvent::shortPress, start);
    CHECK(light.output(start, 12, true) == 0);
    CHECK(light.output(uint32_t(start + 5000), 12, true) == 4095);
    // handle() runs before output() in the application, including at the deadline.
    const uint32_t deadline = start + Config::serviceMs;
    light.handle(ButtonEvent::shortPress, deadline);
    CHECK(light.output(deadline, 12, true) == 4095);
    light.handle(ButtonEvent::shortPress, uint32_t(deadline + 100));
    CHECK(light.output(uint32_t(deadline + 100), 12, true) == 0);
    return 0;
}
TEST feedback_from_daylight_off() {
    LightControl light;
    CHECK(light.output(0, 12, true) == 0);
    light.handle(ButtonEvent::longPress, 100);
    CHECK(light.output(100, 12, true) == 4095);
    CHECK(light.output(2600, 12, true) == 2048);
    CHECK(light.output(5100, 12, true) == 0);
    CHECK(light.disabled());
    return 0;
}

TEST feedback_restart() {
    LightControl light(true);
    light.handle(ButtonEvent::longPress, 0);
    CHECK(light.output(2500, 20, true) == 2047);
    light.handle(ButtonEvent::longPress, 2500);
    CHECK(light.output(2500, 20, true) == 4095);
    CHECK(light.output(5000, 20, true) == 2048);
    light.handle(ButtonEvent::longPress, 5000);
    CHECK(light.output(5000, 20, true) == 2048);
    CHECK(light.output(5200, 20, true) == 1024);
    CHECK(light.output(5400, 20, true) == 0);
    CHECK(light.output(10400, 20, true) == 4095);
    CHECK(light.output(12900, 20, true) == 1433);
    return 0;
}

TEST feedback_return_tracks_automatic_ramp() {
    LightControl light(true);
    CHECK(light.output(0, 19, true) == 0);
    light.handle(ButtonEvent::longPress, 0);
    CHECK(light.output(0, 20, true) == 0);
    CHECK(light.output(5000, 20, true) == 4095);
    CHECK(light.output(7300, 20, true) == 2135);
    CHECK(light.output(7500, 20, true) == 179);
    CHECK(light.output(60000, 20, true) == 1433);
    return 0;
}

TEST enable_blinks_three_times() {
    LightControl light(true);
    light.handle(ButtonEvent::longPress, 0);
    CHECK(light.output(4999, 20, true) == 4094);
    CHECK(light.output(5000, 20, true) == 4095);
    CHECK(light.output(5299, 20, true) == 4095);
    CHECK(light.output(5300, 20, true) == 0);
    CHECK(light.output(5599, 20, true) == 0);
    CHECK(light.output(5600, 20, true) == 4095);
    CHECK(light.output(5899, 20, true) == 4095);
    CHECK(light.output(5900, 20, true) == 0);
    CHECK(light.output(6199, 20, true) == 0);
    CHECK(light.output(6200, 20, true) == 4095);
    CHECK(light.output(6499, 20, true) == 4095);
    CHECK(light.output(6500, 20, true) == 0);
    CHECK(light.output(6799, 20, true) == 0);
    CHECK(light.output(6800, 20, true) == 4095);
    CHECK(light.output(7099, 20, true) == 4095);
    CHECK(light.output(7100, 20, true) == 4095);
    CHECK(light.output(7300, 20, true) == 2764);
    CHECK(light.output(7500, 20, true) == 1433);
    CHECK(light.output(10000, 20, true) == 1433);
    return 0;
}

TEST enable_blinks_across_rollover() {
    constexpr uint32_t start = UINT32_MAX - 5350;
    LightControl light(true);
    light.handle(ButtonEvent::longPress, start);
    CHECK(light.output(uint32_t(start + 5300), 12, true) == 0);
    CHECK(light.output(uint32_t(start + 5600), 12, true) == 4095);
    CHECK(light.output(uint32_t(start + 5900), 12, true) == 0);
    CHECK(light.output(uint32_t(start + 6200), 12, true) == 4095);
    CHECK(light.output(uint32_t(start + 7300), 12, true) == 2048);
    CHECK(light.output(uint32_t(start + 7500), 12, true) == 0);
    return 0;
}
TEST service_ramp_from_day_and_night() {
    LightControl day;
    CHECK(day.output(0, 12, true) == 0);
    day.handle(ButtonEvent::shortPress, 100);
    CHECK(day.output(100, 12, true) == 0);
    CHECK(day.output(2600, 12, true) == 2047);
    CHECK(day.output(5099, 12, true) == 4094);
    CHECK(day.output(5100, 12, true) == 4095);
    day.handle(ButtonEvent::shortPress, 5200);
    CHECK(day.output(5200, 12, true) == 0);

    LightControl night;
    CHECK(night.output(0, 20, true) == 1433);
    night.handle(ButtonEvent::shortPress, 100);
    CHECK(night.output(100, 20, true) == 1433);
    CHECK(night.output(2600, 20, true) == 2764);
    CHECK(night.output(5100, 20, true) == 4095);
    night.handle(ButtonEvent::shortPress, 5200);
    CHECK(night.output(5200, 20, true) == 1433);
    return 0;
}

TEST service_ramp_cancel_and_rollover() {
    constexpr uint32_t start = UINT32_MAX - 500;
    LightControl light;
    CHECK(light.output(start, 20, true) == 1433);
    light.handle(ButtonEvent::shortPress, start);
    CHECK(light.output(uint32_t(start + 2500), 20, true) == 2764);
    light.handle(ButtonEvent::shortPress, uint32_t(start + 2500));
    CHECK(light.output(uint32_t(start + 2500), 20, true) == 1433);
    light.handle(ButtonEvent::shortPress, uint32_t(start + 2600));
    CHECK(light.output(uint32_t(start + 5100), 20, true) == 2764);
    CHECK(light.output(uint32_t(start + 7600), 20, true) == 4095);
    return 0;
}

TEST button_enable_threshold() {
    Button button;
    CHECK(button.update(true, 0, Config::enableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, 30, Config::enableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, 779, Config::enableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, 780, Config::enableLongPressMs) == ButtonEvent::longPress);
    CHECK(button.update(true, 1280, Config::disableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(false, 1500, Config::disableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(false, 1530, Config::disableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, 2000, Config::disableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, 2030, Config::disableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, 2780, Config::disableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, 3279, Config::disableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, 3280, Config::disableLongPressMs) == ButtonEvent::longPress);
    return 0;
}

TEST button_enable_rollover_and_short_press() {
    Button button;
    constexpr uint32_t start = UINT32_MAX - 100;
    CHECK(button.update(true, start, Config::enableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, uint32_t(start + 30), Config::enableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, uint32_t(start + 779), Config::enableLongPressMs) == ButtonEvent::none);
    CHECK(button.update(true, uint32_t(start + 780), Config::enableLongPressMs) == ButtonEvent::longPress);
    Button shortButton;
    CHECK(shortButton.update(true, 0, Config::enableLongPressMs) == ButtonEvent::none);
    CHECK(shortButton.update(true, 30, Config::enableLongPressMs) == ButtonEvent::none);
    CHECK(shortButton.update(false, 700, Config::enableLongPressMs) == ButtonEvent::none);
    CHECK(shortButton.update(false, 730, Config::enableLongPressMs) == ButtonEvent::shortPress);
    return 0;
}

TEST service_when_automatic_disabled() {
    LightControl light(true);
    CHECK(light.output(0, 20, true) == 0);
    light.handle(ButtonEvent::shortPress, 100);
    CHECK(light.disabled());
    CHECK(light.output(100, 20, true) == 0);
    CHECK(light.output(2600, 20, true) == Config::serviceOutput / 2);
    CHECK(light.output(5100, 20, true) == Config::serviceOutput);
    light.handle(ButtonEvent::shortPress, 5200);
    CHECK(light.output(5200, 20, true) == 0);
    CHECK(light.disabled());
    light.handle(ButtonEvent::shortPress, 6000);
    CHECK(light.output(605999, 20, true) == Config::serviceOutput);
    CHECK(light.output(606000, 20, true) == 0);
    CHECK(light.disabled());
    return 0;
}

TEST service_disabled_without_clock_and_rollover() {
    constexpr uint32_t start = UINT32_MAX - 500;
    LightControl light(true);
    light.handle(ButtonEvent::shortPress, start);
    CHECK(light.output(uint32_t(start + 2500), 0, false) == Config::serviceOutput / 2);
    CHECK(light.output(uint32_t(start + 5000), 0, false) == Config::serviceOutput);
    CHECK(light.output(uint32_t(start + 600000), 0, false) == 0);
    CHECK(light.disabled());
    return 0;
}

TEST service_interrupts_mode_feedback() {
    for (uint8_t mode = 0; mode < 2; ++mode) {
        const bool initiallyDisabled = mode != 0;
        LightControl light(initiallyDisabled);
        light.output(0, 12, true);
        light.handle(ButtonEvent::longPress, 0);
        const uint16_t current = light.output(1000, 12, true);
        light.handle(ButtonEvent::shortPress, 1000);
        CHECK(light.output(1000, 12, true) == current);
        CHECK(light.output(6000, 12, true) == Config::serviceOutput);
        CHECK(light.disabled() == !initiallyDisabled);
        light.handle(ButtonEvent::shortPress, 6100);
        CHECK(light.output(6100, 12, true) == 0);
        CHECK(light.output(8000, 12, true) == 0);
        CHECK(light.disabled() == !initiallyDisabled);
    }
    return 0;
}

TEST unknown_time_keeps_automatic_light_on() {
    LightControl light;
    CHECK(light.output(0, 0, false) == Config::nightOutput);
    CHECK(light.output(60000, 12, false) == Config::nightOutput);
    CHECK(light.output(61000, 12, true) == 0);
    CHECK(light.output(62000, 12, false) == Config::nightOutput);
    CHECK(light.output(63000, 20, true) == Config::nightOutput);
    CHECK(light.output(64000, 20, true, 60) == Config::nightOutput);
    return 0;
}

TEST unknown_time_preserves_manual_override_and_service() {
    LightControl disabled(true);
    CHECK(disabled.output(0, 0, false) == 0);
    LightControl light;
    CHECK(light.output(0, 0, false) == Config::nightOutput);
    light.handle(ButtonEvent::shortPress, 100);
    CHECK(light.output(5100, 0, false) == Config::serviceOutput);
    CHECK(light.output(600100, 0, false) == Config::nightOutput);
    return 0;
}

TEST twelve_bit_ramp_resolution() {
    AutomaticLight automatic;
    CHECK(automatic.output(0, 19, true) == 0);
    CHECK(automatic.output(0, 20, true) == 0);
    uint16_t previous = 0;
    unsigned transitions = 0;
    for (uint32_t ms = 1; ms <= 60000; ++ms) {
        const uint16_t value = automatic.output(ms, 20, true);
        CHECK(value >= previous && value <= 1433);
        CHECK(value - previous <= 1);
        if (value != previous) ++transitions;
        previous = value;
    }
    CHECK(transitions == 1433);
    FeedbackRamp feedback;
    feedback.start(true, 4095, 0);
    CHECK(feedback.output(0, 0) == 4095);
    CHECK(feedback.output(2500, 0) == 2048);
    CHECK(feedback.output(5000, 0) == 0);
    return 0;
}

TEST ds3231_calendar() {
    const uint8_t raw[] = {0x59, 0x58, 0x23, 0x07, 0x29, 0x02, 0x24};
    ClockTime time;
    CHECK(RtcRegisters::decode(raw, time));
    CHECK(time.year == 24 && time.month == 2 && time.day == 29 && time.weekday == 6);
    CHECK(time.hour == 23 && time.minute == 58 && time.second == 59);
    uint8_t encoded[7];
    RtcRegisters::encode(time, encoded);
    for (uint8_t i = 0; i < 7; ++i) CHECK(encoded[i] == raw[i]);
    encoded[6] = 0x25;
    CHECK(!RtcRegisters::decode(encoded, time));
    encoded[6] = 0x24;
    encoded[5] = 0x82;
    CHECK(!RtcRegisters::decode(encoded, time));
    encoded[5] = 0x02;
    encoded[0] = 0x5a;
    CHECK(!RtcRegisters::decode(encoded, time));
    encoded[0] = 0x60;
    CHECK(!RtcRegisters::decode(encoded, time));
    encoded[0] = 0;
    encoded[3] = 0;
    CHECK(!RtcRegisters::decode(encoded, time));
    return 0;
}

TEST ds3231_twelve_hour_mode() {
    uint8_t raw[] = {0, 0, 0x52, 1, 1, 1, 0x26};
    ClockTime time;
    CHECK(RtcRegisters::decode(raw, time) && time.hour == 0);
    raw[2] = 0x72;
    CHECK(RtcRegisters::decode(raw, time) && time.hour == 12);
    raw[2] = 0x61;
    CHECK(RtcRegisters::decode(raw, time) && time.hour == 13);
    raw[2] = 0x40;
    CHECK(!RtcRegisters::decode(raw, time));
    raw[2] = 0x24;
    CHECK(!RtcRegisters::decode(raw, time));
    return 0;
}

TEST runtime_brightness_and_service() {
    LightControl light;
    CHECK(light.output(0, 21, true) == Config::nightOutput);
    light.setBrightness(50);
    CHECK(light.output(0, 21, true) == Config::nightOutput);
    CHECK(light.output(60000, 21, true) == Config::percentOutput(50, 100));
    light.setService(true, 60000);
    light.setService(true, 61000);
    CHECK(light.output(65000, 21, true) == Config::serviceOutput);
    light.setService(false, 65000);
    CHECK(light.output(65000, 21, true) == Config::percentOutput(50, 100));
    light.setBrightness(35);
    CHECK(light.output(65000, 21, false) == Config::nightOutput);
    return 0;
}

TEST runtime_schedule() {
    LightControl light;
    light.setSchedule(9 * 60, 17 * 60);
    CHECK(light.output(0, 10, true) == Config::nightOutput);
    light.setSchedule(20 * 60, 8 * 60);
    CHECK(light.output(0, 10, true) == Config::nightOutput);
    CHECK(light.output(60000, 10, true) == 0);
    CHECK(light.output(60000, 21, true) == 0);
    CHECK(light.output(120000, 21, true) == Config::nightOutput);
    light.setSchedule(0, 0);
    light.output(120000, 21, true);
    CHECK(light.output(180000, 21, true) == 0);
    uint16_t minutes;
    CHECK(parseScheduleTime("23:59", minutes) && minutes == 1439);
    CHECK(parseScheduleTime("00:00", minutes) && minutes == 0);
    CHECK(!parseScheduleTime("24:00", minutes));
    CHECK(!parseScheduleTime("20:60", minutes));
    CHECK(!parseScheduleTime("2:00", minutes));
    CHECK(!parseScheduleTime("", minutes));
    CHECK(!parseScheduleTime("20:00x", minutes));
    return 0;
}
