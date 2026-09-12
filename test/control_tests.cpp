#include <Control.h>
#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (false)
#ifdef _WIN32
#define TEST extern "C" __declspec(dllexport) int
#else
#define TEST extern "C" int
#endif

TEST schedule() {
    LightControl light;
    for (uint8_t hour = 0; hour < 24; ++hour) {
        LightControl initial;
        CHECK(initial.output(0, hour, true) == ((hour < 8 || hour >= 20) ? 43 : 0));
    }
    CHECK(light.output(0, 20, false) == 0);
    CHECK(light.output(0, 24, true) == 0);
    return 0;
}

TEST service_timeout() {
    LightControl light;
    light.handle(ButtonEvent::shortPress, 100);
    CHECK(light.output(100, 12, true) == 204);
    CHECK(light.output(600099, 12, true) == 204);
    CHECK(light.output(600100, 12, true) == 0);
    light.handle(ButtonEvent::shortPress, 600200);
    CHECK(light.output(1100000, 20, true) == 204);
    CHECK(light.output(1200200, 20, true) == 43);
    return 0;
}

TEST service_toggle_and_boundaries() {
    LightControl light;
    light.handle(ButtonEvent::shortPress, 0);
    CHECK(light.output(100, 7, true) == 204);
    CHECK(light.output(200, 8, true) == 204);
    light.handle(ButtonEvent::shortPress, 500000);
    CHECK(light.output(500000, 8, true) == 0);
    CHECK(light.output(1100000, 8, true) == 0);
    light.handle(ButtonEvent::shortPress, 1200000);
    CHECK(light.output(1200001, 0, false) == 204);
    CHECK(light.output(1800000, 0, false) == 0);
    return 0;
}

TEST disable_and_enable() {
    LightControl light;
    light.handle(ButtonEvent::shortPress, 0);
    CHECK(light.output(0, 20, true) == 204);
    light.handle(ButtonEvent::longPress, 100);
    CHECK(light.disabled());
    CHECK(light.output(100, 20, true) == 204);
    CHECK(light.output(350, 20, true) == 229);
    CHECK(light.output(600, 20, true) == 255);
    CHECK(light.output(3100, 20, true) == 128);
    CHECK(light.output(5605, 20, true) == 0);
    light.handle(ButtonEvent::shortPress, 6000);
    CHECK(light.output(6000, 20, true) == 0);
    light.handle(ButtonEvent::longPress, 7000);
    CHECK(!light.disabled());
    CHECK(light.output(7000, 20, true) == 0);
    CHECK(light.output(9500, 20, true) == 127);
    CHECK(light.output(12000, 20, true) == 255);
    CHECK(light.output(13000, 20, true) == 149);
    CHECK(light.output(13250, 20, true) == 43);
    LightControl restored(true);
    CHECK(restored.output(0, 20, true) == 0);
    return 0;
}

TEST enable_during_day() {
    LightControl light(true);
    light.handle(ButtonEvent::longPress, 0);
    CHECK(light.output(0, 12, true) == 0);
    light.handle(ButtonEvent::shortPress, 1000);
    CHECK(light.output(5000, 12, true) == 255);
    CHECK(light.output(6000, 12, true) == 128);
    light.handle(ButtonEvent::shortPress, 6000);
    CHECK(light.output(6250, 12, true) == 0);
    return 0;
}

TEST timer_rollover() {
    constexpr uint32_t start = UINT32_MAX - 1000;
    LightControl light;
    light.handle(ButtonEvent::shortPress, start);
    CHECK(light.output(uint32_t(start + 599999), 12, true) == 204);
    CHECK(light.output(uint32_t(start + 600000), 12, true) == 0);
    light.handle(ButtonEvent::longPress, start);
    CHECK(light.output(uint32_t(start + 250), 12, true) == 127);
    CHECK(light.output(uint32_t(start + 500), 12, true) == 255);
    CHECK(light.output(uint32_t(start + 3000), 12, true) == 128);
    CHECK(light.output(uint32_t(start + 5500), 12, true) == 0);
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
    CHECK(button.update(true, 1529) == ButtonEvent::none);
    CHECK(button.update(true, 1530) == ButtonEvent::longPress);
    CHECK(button.update(true, 10000) == ButtonEvent::none);
    CHECK(button.update(false, 11000) == ButtonEvent::none);
    CHECK(button.update(false, 11030) == ButtonEvent::none);
    CHECK(button.update(true, 12000) == ButtonEvent::none);
    CHECK(button.update(true, 12030) == ButtonEvent::none);
    CHECK(button.update(true, 13530) == ButtonEvent::longPress);
    return 0;
}

TEST button_rollover() {
    Button button;
    constexpr uint32_t start = UINT32_MAX - 15;
    CHECK(button.update(true, start) == ButtonEvent::none);
    CHECK(button.update(true, uint32_t(start + 30)) == ButtonEvent::none);
    CHECK(button.update(true, uint32_t(start + 1530)) == ButtonEvent::longPress);
    CHECK(button.update(false, uint32_t(start + 2100)) == ButtonEvent::none);
    CHECK(button.update(false, uint32_t(start + 2130)) == ButtonEvent::none);
    return 0;
}

TEST automatic_ramps() {
    LightControl light;
    CHECK(light.output(0, 19, true) == 0);
    CHECK(light.output(100, 20, true) == 0);
    CHECK(light.output(30100, 20, true) == 21);
    CHECK(light.output(60099, 20, true) == 42);
    CHECK(light.output(60100, 20, true) == 43);
    CHECK(light.output(100000, 7, true) == 43);
    CHECK(light.output(100100, 8, true) == 43);
    CHECK(light.output(130100, 8, true) == 22);
    CHECK(light.output(160099, 8, true) == 1);
    CHECK(light.output(160100, 8, true) == 0);
    return 0;
}

TEST automatic_ramp_overrides() {
    LightControl light;
    CHECK(light.output(0, 19, true) == 0);
    light.handle(ButtonEvent::shortPress, 0);
    CHECK(light.output(580000, 20, true) == 204);
    CHECK(light.output(600000, 20, true) == 14);
    CHECK(light.output(640000, 20, true) == 43);
    light.handle(ButtonEvent::longPress, 650000);
    CHECK(light.output(650000, 20, true) == 43);
    CHECK(light.output(650250, 20, true) == 149);
    CHECK(light.output(650500, 20, true) == 255);
    CHECK(light.output(655500, 20, true) == 0);
    CHECK(light.output(700000, 8, true) == 0);
    light.handle(ButtonEvent::longPress, 720000);
    CHECK(light.output(720000, 8, true) == 0);
    CHECK(light.output(725000, 8, true) == 255);
    CHECK(light.output(730000, 8, true) == 22);
    CHECK(light.output(760000, 8, true) == 0);
    return 0;
}

TEST automatic_ramp_clock_changes() {
    LightControl light;
    CHECK(light.output(0, 19, true) == 0);
    CHECK(light.output(100, 20, true) == 0);
    CHECK(light.output(30100, 20, true) == 21);
    CHECK(light.output(30100, 12, true) == 21);
    CHECK(light.output(60100, 12, true) == 11);
    CHECK(light.output(90100, 12, true) == 0);
    CHECK(light.output(100000, 20, true) == 0);
    CHECK(light.output(130000, 20, false) == 0);
    CHECK(light.output(130100, 20, true) == 43);
    return 0;
}

TEST automatic_ramp_rollover() {
    constexpr uint32_t start = UINT32_MAX - 1000;
    LightControl light;
    CHECK(light.output(start, 19, true) == 0);
    CHECK(light.output(start, 20, true) == 0);
    CHECK(light.output(uint32_t(start + 30000), 20, true) == 21);
    CHECK(light.output(uint32_t(start + 60000), 20, true) == 43);
    CHECK(light.output(uint32_t(start + 70000), 8, true) == 43);
    CHECK(light.output(uint32_t(start + 100000), 8, true) == 22);
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
    CHECK(light.output(0, 20, true, 60) == 0);
    return 0;
}

TEST brightness_limit() {
    CHECK(Config::percentOutput(17, 100) == 43);
    CHECK(Config::percentOutput(17, 70) == 43);
    CHECK(Config::percentOutput(80, 70) == 179);
    CHECK(Config::percentOutput(80, 100) == 204);
    CHECK(Config::percentOutput(100, 100) == 255);
    CHECK(Config::percentOutput(100, 0) == 0);
    CHECK(Config::percentOutput(0, 100) == 0);
    return 0;
}

TEST service_toggle_at_night() {
    LightControl light;
    CHECK(light.output(0, 20, true) == 43);
    light.handle(ButtonEvent::shortPress, 100);
    CHECK(light.output(100, 20, true) == 204);
    light.handle(ButtonEvent::shortPress, 200);
    CHECK(light.output(200, 20, true) == 43);
    light.handle(ButtonEvent::shortPress, 300);
    CHECK(light.output(600299, 20, true) == 204);
    CHECK(light.output(600300, 20, true) == 43);
    return 0;
}

TEST service_toggle_during_ramp() {
    LightControl light;
    CHECK(light.output(0, 19, true) == 0);
    light.handle(ButtonEvent::shortPress, 100);
    CHECK(light.output(100, 20, true) == 204);
    light.handle(ButtonEvent::shortPress, 30100);
    CHECK(light.output(30100, 20, true) == 21);
    CHECK(light.output(60100, 20, true) == 43);
    return 0;
}

TEST service_press_at_expiry() {
    constexpr uint32_t start = UINT32_MAX - 1000;
    LightControl light;
    light.handle(ButtonEvent::shortPress, start);
    CHECK(light.output(start, 12, true) == 204);
    // handle() runs before output() in the application, including at the deadline.
    const uint32_t deadline = start + Config::serviceMs;
    light.handle(ButtonEvent::shortPress, deadline);
    CHECK(light.output(deadline, 12, true) == 204);
    light.handle(ButtonEvent::shortPress, uint32_t(deadline + 100));
    CHECK(light.output(uint32_t(deadline + 100), 12, true) == 0);
    return 0;
}
TEST feedback_from_daylight_off() {
    LightControl light;
    CHECK(light.output(0, 12, true) == 0);
    light.handle(ButtonEvent::longPress, 100);
    CHECK(light.output(100, 12, true) == 0);
    CHECK(light.output(350, 12, true) == 127);
    CHECK(light.output(600, 12, true) == 255);
    CHECK(light.output(3100, 12, true) == 128);
    CHECK(light.output(5600, 12, true) == 0);
    CHECK(light.disabled());
    return 0;
}

TEST feedback_restart_is_continuous() {
    LightControl light(true);
    light.handle(ButtonEvent::longPress, 0);
    CHECK(light.output(2500, 20, true) == 127);
    light.handle(ButtonEvent::longPress, 2500);
    CHECK(light.output(2500, 20, true) == 127);
    CHECK(light.output(2750, 20, true) == 191);
    CHECK(light.output(3000, 20, true) == 255);
    CHECK(light.output(5500, 20, true) == 128);
    light.handle(ButtonEvent::longPress, 5500);
    CHECK(light.output(5500, 20, true) == 128);
    CHECK(light.output(5750, 20, true) == 64);
    CHECK(light.output(6000, 20, true) == 0);
    CHECK(light.output(11000, 20, true) == 255);
    CHECK(light.output(12250, 20, true) == 43);
    return 0;
}

TEST feedback_return_tracks_automatic_ramp() {
    LightControl light(true);
    CHECK(light.output(0, 19, true) == 0);
    light.handle(ButtonEvent::longPress, 0);
    CHECK(light.output(0, 20, true) == 0);
    CHECK(light.output(5000, 20, true) == 255);
    CHECK(light.output(6000, 20, true) == 130);
    CHECK(light.output(6250, 20, true) == 4);
    CHECK(light.output(60000, 20, true) == 43);
    return 0;
}

TEST enable_blinks_twice() {
    LightControl light(true);
    light.handle(ButtonEvent::longPress, 0);
    CHECK(light.output(4999, 20, true) == 254);
    CHECK(light.output(5000, 20, true) == 255);
    CHECK(light.output(5149, 20, true) == 255);
    CHECK(light.output(5150, 20, true) == 0);
    CHECK(light.output(5299, 20, true) == 0);
    CHECK(light.output(5300, 20, true) == 255);
    CHECK(light.output(5449, 20, true) == 255);
    CHECK(light.output(5450, 20, true) == 0);
    CHECK(light.output(5599, 20, true) == 0);
    CHECK(light.output(5600, 20, true) == 255);
    CHECK(light.output(5749, 20, true) == 255);
    CHECK(light.output(5750, 20, true) == 255);
    CHECK(light.output(6000, 20, true) == 149);
    CHECK(light.output(6250, 20, true) == 43);
    CHECK(light.output(10000, 20, true) == 43);
    return 0;
}

TEST enable_blinks_across_rollover() {
    constexpr uint32_t start = UINT32_MAX - 5200;
    LightControl light(true);
    light.handle(ButtonEvent::longPress, start);
    CHECK(light.output(uint32_t(start + 5150), 12, true) == 0);
    light.handle(ButtonEvent::shortPress, uint32_t(start + 5200));
    CHECK(light.output(uint32_t(start + 5300), 12, true) == 255);
    CHECK(light.output(uint32_t(start + 5450), 12, true) == 0);
    CHECK(light.output(uint32_t(start + 5600), 12, true) == 255);
    CHECK(light.output(uint32_t(start + 6000), 12, true) == 128);
    CHECK(light.output(uint32_t(start + 6250), 12, true) == 0);
    return 0;
}