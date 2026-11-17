#pragma once
#include <NetworkServer.h>
#include <functional>
#include "ClockTime.h"

struct DeviceStatus {
    String hostname;
    int brightness = 0;
    ClockTime time{};
    bool clockValid = false;
    bool rtcValid = false;
    bool rtcSetFromNetwork = false;
    bool disabled = false;
    bool service = false;
    bool feedback = false;
    bool scheduledOn = false;
    bool hardwareOk = false;
    bool outputOk = false;
    uint16_t output = 0;
    uint16_t outputMaximum = 4095;
    uint16_t onTime = 0;
    uint16_t offTime = 0;
};

class StatusPage {
public:
    void begin(std::function<DeviceStatus()> status, std::function<bool(const ClockTime&)> setTime,
               std::function<bool(const String&, const String&)> command);
    void update();
private:
    void respond();
    void respondTime();
    void respondCommand();
    void sendResponse(const char* result, const String& body);
    String header(const char* name) const;
    void closeClient();
    NetworkServer server_{80, 1};
    NetworkClient client_;
    std::function<DeviceStatus()> status_;
    std::function<bool(const ClockTime&)> setTime_;
    std::function<bool(const String&, const String&)> command_;
    String request_;
    String response_;
    size_t sent_ = 0;
    uint32_t acceptedAt_ = 0;
    uint32_t retryAt_ = 0;
};
