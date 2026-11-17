#include "NetworkServices.h"
#include "Dm4Board.h"
#include "NetworkConfig.h"
#include <WiFi.h>
#include <ESPmDNS.h>

namespace {
constexpr uint32_t connectionPollMs = 500;
constexpr uint32_t ethernetRetryMs = 30000;
constexpr int otaTimeoutMs = 5000;
}

void NetworkServices::begin(std::function<bool()> prepareUpdate, const String& hostname) {
    hostname_ = hostname;
    ota_.setMdnsEnabled(false);
    ota_.setHostname(hostname_.c_str());
    ota_.setPassword(NetworkConfig::ota_password);
    ota_.setTimeout(otaTimeoutMs);
    ota_.onStart([this, prepareUpdate]() {
        updating_ = true;
        if (!prepareUpdate()) {
            updater_.abort();
            Serial.println("ERR OTA aborted: output could not be switched OFF");
            return;
        }
        Serial.println("OTA started; light OFF until update finishes");
    });
    ota_.onEnd([]() { Serial.println("OTA complete; restarting"); });
    ota_.onError([this](ota_error_t error) {
        if (updater_.isRunning()) updater_.abort();
        updating_ = false;
        Serial.printf("ERR OTA %u; local control resumes\n", unsigned(error));
    });
    if (NetworkConfig::ethernet) {
        spi_.begin(Dm4::ethernetClock, Dm4::ethernetMiso, Dm4::ethernetMosi, Dm4::ethernetCs);
        startEthernet();
    }
    if (NetworkConfig::wifi_ssid[0]) {
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(hostname_.c_str());
        WiFi.setAutoReconnect(true);
        WiFi.begin(NetworkConfig::wifi_ssid, NetworkConfig::wifi_password);
    }
    if (!NetworkConfig::ota_password[0]) Serial.println("OTA disabled: no password configured");
}

void NetworkServices::startEthernet() {
    retryAt_ = millis();
    constexpr int phyAddress = 1;
    ethernetStarted_ = ethernet_.begin(ETH_PHY_W5500, phyAddress, Dm4::ethernetCs,
                                      Dm4::ethernetIrq, Dm4::ethernetReset, spi_);
    if (ethernetStarted_) ethernet_.setHostname(hostname_.c_str());
    else Serial.println("ERR Ethernet initialization; retry in 30 seconds");
}

void NetworkServices::pollConnections() {
    const IPAddress ethernetIp = ethernetStarted_ && ethernet_.hasIP()
        ? ethernet_.localIP() : IPAddress();
    const IPAddress wifiIp = WiFi.status() == WL_CONNECTED ? WiFi.localIP() : IPAddress();
    if (ethernetIp != ethernetIp_) {
        ethernetIp_ = ethernetIp;
        Serial.printf("LAN IP %s\n", ethernetIp.toString().c_str());
    }
    if (wifiIp != wifiIp_) {
        wifiIp_ = wifiIp;
        Serial.printf("WiFi IP %s\n", wifiIp.toString().c_str());
    }
    const bool connected = uint32_t(ethernetIp) != 0 || uint32_t(wifiIp) != 0;
    if (connected && !mdnsStarted_) {
        mdnsStarted_ = MDNS.begin(hostname_.c_str());
        if (mdnsStarted_) {
            MDNS.addService("http", "tcp", 80);
            if (NetworkConfig::ota_password[0]) MDNS.enableArduino(3232, true);
            MDNS.addService("fnc-light", "tcp", 80);
        }
    } else if (!connected && mdnsStarted_) {
        MDNS.end();
        mdnsStarted_ = false;
    }
    if (connected && !otaStarted_ && NetworkConfig::ota_password[0]) {
        ota_.begin();
        otaStarted_ = true;
        Serial.println("OTA ready (port 3232)");
    } else if (!connected && otaStarted_) {
        ota_.end();
        otaStarted_ = false;
        Serial.println("OTA offline; local control remains active");
    }
}

void NetworkServices::update() {
    const uint32_t now = millis();
    if (NetworkConfig::ethernet && !ethernetStarted_ && uint32_t(now - retryAt_) >= ethernetRetryMs)
        startEthernet();
    if (uint32_t(now - pollAt_) >= connectionPollMs) {
        pollAt_ = now;
        pollConnections();
    }
    if (otaStarted_) ota_.handle();
}

void NetworkServices::setHostname(const String& hostname) {
    hostname_ = hostname;
    ota_.setHostname(hostname_.c_str());
    WiFi.setHostname(hostname_.c_str());
    if (ethernetStarted_) ethernet_.setHostname(hostname_.c_str());
    if (mdnsStarted_) MDNS.end();
    mdnsStarted_ = false;
}
