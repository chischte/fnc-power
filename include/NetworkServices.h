#pragma once
#include <ArduinoOTA.h>
#include <ETH.h>
#include <SPI.h>
#include <functional>

class NetworkServices {
public:
    void begin(std::function<bool()> prepareUpdate, const String& hostname);
    void setHostname(const String& hostname);
    void update();
    bool updating() const { return updating_; }
    bool connected() const { return uint32_t(ethernetIp_) != 0 || uint32_t(wifiIp_) != 0; }
private:
    void startEthernet();
    void pollConnections();
    ETHClass ethernet_;
    SPIClass spi_{FSPI};
    UpdateClass updater_;
    ArduinoOTAClass ota_{&updater_};
    IPAddress ethernetIp_;
    IPAddress wifiIp_;
    uint32_t pollAt_ = 0;
    uint32_t retryAt_ = 0;
    bool ethernetStarted_ = false;
    bool otaStarted_ = false;
    bool updating_ = false;
    bool mdnsStarted_ = false;
    String hostname_;
};
