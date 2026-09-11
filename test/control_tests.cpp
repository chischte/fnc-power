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
    CHECK(light.output(100, 12, true) == 255);
    CHECK(light.output(600099, 12, true) == 255);
    CHECK(light.output(600100, 12, true) == 0);
    light.handle(ButtonEvent::shortPress, 600200);
    CHECK(light.output(1100000, 20, true) == 255);
    CHECK(light.output(1200200, 20, true) == 43);
    return 0;
}

TEST service_restart_and_boundaries() {
    LightControl light;
    light.handle(ButtonEvent::shortPress, 0);
    CHECK(light.output(100, 7, true) == 255);
    CHECK(light.output(200, 8, true) == 255);
    light.handle(ButtonEvent::shortPress, 500000);
    CHECK(light.output(600000, 8, true) == 255);
    CHECK(light.output(1100000, 8, true) == 0);
    light.handle(ButtonEvent::shortPress, 1200000);
    CHECK(light.output(1200001, 0, false) == 255);
    CHECK(light.output(1800000, 0, false) == 0);
    return 0;
}

TEST disable_and_enable() {
    LightControl light;
    light.handle(ButtonEvent::shortPress, 0);
    light.handle(ButtonEvent::longPress, 100);
    CHECK(light.disabled());
    CHECK(light.output(100, 20, true) == 255);
    CHECK(light.output(1600, 20, true) == 128);
    CHECK(light.output(3105, 20, true) == 0);
    light.handle(ButtonEvent::shortPress, 4000);
    CHECK(light.output(4000, 20, true) == 0);
    light.handle(ButtonEvent::longPress, 5000);
    CHECK(!light.disabled());
    CHECK(light.output(5000, 20, true) == 0);
    CHECK(light.output(6500, 20, true) == 127);
    CHECK(light.output(8005, 20, true) == 255);
    CHECK(light.output(8006, 20, true) == 43);
    LightControl restored(true);
    CHECK(restored.output(0, 20, true) == 0);
    return 0;
}

TEST enable_during_day() {
    LightControl light(true);
    light.handle(ButtonEvent::longPress, 0);
    CHECK(light.output(0, 12, true) == 0);
    light.handle(ButtonEvent::shortPress, 1000);
    CHECK(light.output(3000, 12, true) == 255);
    CHECK(light.output(3001, 12, true) == 0);
    return 0;
}

TEST timer_rollover() {
    constexpr uint32_t start = UINT32_MAX - 1000;
    LightControl light;
    light.handle(ButtonEvent::shortPress, start);
    CHECK(light.output(uint32_t(start + 599999), 12, true) == 255);
    CHECK(light.output(uint32_t(start + 600000), 12, true) == 0);
    light.handle(ButtonEvent::longPress, start);
    CHECK(light.output(uint32_t(start + 1500), 12, true) == 128);
    CHECK(light.output(uint32_t(start + 3001), 12, true) == 0);
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
    CHECK(button.update(true, 2029) == ButtonEvent::none);
    CHECK(button.update(true, 2030) == ButtonEvent::longPress);
    CHECK(button.update(true, 10000) == ButtonEvent::none);
    CHECK(button.update(false, 11000) == ButtonEvent::none);
    CHECK(button.update(false, 11030) == ButtonEvent::none);
    CHECK(button.update(true, 12000) == ButtonEvent::none);
    CHECK(button.update(true, 12030) == ButtonEvent::none);
    CHECK(button.update(true, 14030) == ButtonEvent::longPress);
    return 0;
}

TEST button_rollover() {
    Button button;
    constexpr uint32_t start = UINT32_MAX - 15;
    CHECK(button.update(true, start) == ButtonEvent::none);
    CHECK(button.update(true, uint32_t(start + 30)) == ButtonEvent::none);
    CHECK(button.update(true, uint32_t(start + 2030)) == ButtonEvent::longPress);
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
    CHECK(light.output(580000, 20, true) == 255);
    CHECK(light.output(600000, 20, true) == 14);
    CHECK(light.output(640000, 20, true) == 43);
    light.handle(ButtonEvent::longPress, 650000);
    CHECK(light.output(650000, 20, true) == 255);
    CHECK(light.output(653000, 20, true) == 0);
    CHECK(light.output(700000, 8, true) == 0);
    light.handle(ButtonEvent::longPress, 720000);
    CHECK(light.output(720000, 8, true) == 0);
    CHECK(light.output(723000, 8, true) == 255);
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
