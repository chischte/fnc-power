#include "StatusPage.h"
#include "ClockTimeText.h"
#include "NetworkConfig.h"
#include <errno.h>
#include <sys/socket.h>

namespace {
constexpr uint32_t requestTimeoutMs = 2000;
constexpr uint32_t listenRetryMs = 1000;
constexpr size_t maximumRequestBytes = 2048;
constexpr size_t bytesPerUpdate = 128;

String render(const DeviceStatus& status) {
    char time[32] = "Unbekannt";
    if (status.clockValid) {
        snprintf(time, sizeof(time), "20%02u-%02u-%02u %02u:%02u:%02u",
                 status.time.year, status.time.month, status.time.day,
                 status.time.hour, status.time.minute, status.time.second);
    }
    const char* mode = status.feedback ? "Taster-Rueckmeldung"
        : status.service ? "Servicelicht" : status.disabled ? "Dauerhaft AUS" : "Automatik";
    char details[1200];
    snprintf(details, sizeof(details),
        "<dl><dt>Geraetezeit (lokal)</dt><dd>%s</dd>"
        "<dt>RTC</dt><dd>%s</dd><dt>Zeitquelle</dt><dd>%s</dd>"
        "<dt>RTC in diesem Lauf per NTP gestellt</dt><dd>%s</dd>"
        "<dt>Betriebsmodus</dt><dd>%s</dd>"
        "<dt>Automatik freigegeben</dt><dd>%s</dd>"
        "<dt>Zeitplan</dt><dd>%02u:%02u bis %02u:%02u; aktuell %s</dd>"
        "<dt>OUT4 Sollwert</dt><dd>%u / %u (%.1f %%, ca. %.3f V)</dd>"
        "<dt>Hardware / Ausgang schreiben</dt><dd>%s / %s</dd></dl>",
        time, status.rtcValid ? "OK" : "Ungueltig oder nicht lesbar",
        status.rtcValid ? "RTC" : status.clockValid ? "NTP / ESP32-Uhr" : "Keine; Automatik dauerhaft EIN",
        status.rtcSetFromNetwork ? "Ja" : "Nein", mode,
        status.disabled ? "Nein" : "Ja", status.onTime / 60, status.onTime % 60,
        status.offTime / 60, status.offTime % 60,
        !status.clockValid ? "unbekannt" : status.scheduledOn ? "EIN" : "AUS",
        status.output, status.outputMaximum, 100.0 * status.output / status.outputMaximum,
        10.0 * status.output / status.outputMaximum,
        status.hardwareOk ? "OK" : "Fehler", status.outputOk ? "OK" : "Nicht bestaetigt");
    return String("<!doctype html><html lang='de'><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<meta http-equiv='refresh' content='5'><title>fnc-power Status</title>"
        "<style>body{font:18px system-ui;max-width:650px;margin:3rem auto;padding:0 1rem;"
        "background:#f4f6f8;color:#17212b}dt{margin-top:1rem;font-weight:600}"
        "dd{margin:.3rem 0}p{font-size:15px}</style><h1>fnc-power DM4</h1>")
        + details + "<p>Taster: IN4. Aktualisierung alle 5 Sekunden.</p>"
        "<p>Spannung ist aus dem Sollwert berechnet, nicht gemessen.</p></html>";
}
}

void StatusPage::begin(std::function<DeviceStatus()> status,
                       std::function<bool(const ClockTime&)> setTime) {
    status_ = status;
    setTime_ = setTime;
    server_.begin();
}

void StatusPage::closeClient() {
    client_.stop();
    request_ = "";
    response_ = "";
    sent_ = 0;
}

void StatusPage::respond() {
    if (request_.startsWith("POST /time HTTP/1.")) {
        respondTime();
        return;
    }
    const bool root = request_.startsWith("GET / HTTP/1.");
    sendResponse(root ? "200 OK" : "404 Not Found", root ? render(status_()) : String("Not found"));
}

String StatusPage::header(const char* name) const {
    String value;
    bool found = false;
    int start = request_.indexOf("\r\n") + 2;
    while (start > 1) {
        const int end = request_.indexOf("\r\n", start);
        if (end <= start) break;
        const int colon = request_.indexOf(':', start);
        if (colon > start && colon < end && request_.substring(start, colon).equalsIgnoreCase(name)) {
            if (found) return String(); // Reject ambiguous duplicate headers.
            found = true;
            value = request_.substring(colon + 1, end);
            value.trim();
        }
        start = end + 2;
    }
    return value;
}

void StatusPage::respondTime() {
    if (!NetworkConfig::ota_password[0] || header("X-OTA-Password") != NetworkConfig::ota_password) {
        sendResponse("403 Forbidden", "Forbidden");
        return;
    }
    ClockTime time;
    if (header("Content-Length") != "0" || !header("Transfer-Encoding").isEmpty() ||
        !parseClockTime(header("X-Local-Time").c_str(), time)) {
        sendResponse("400 Bad Request", "Use X-Local-Time: YYYY-MM-DD HH:MM:SS and an empty body");
        return;
    }
    if (!setTime_(time)) {
        sendResponse("503 Service Unavailable", "RTC write or verification failed");
        return;
    }
    sendResponse("200 OK", "OK TIME");
}

void StatusPage::sendResponse(const char* result, const String& body) {
    response_ = String("HTTP/1.1 ") + result + "\r\n";
    response_ += "Content-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\n"
                 "Connection: close\r\nContent-Length: ";
    response_ += String(body.length()) + "\r\n\r\n" + body;
}

void StatusPage::update() {
    const uint32_t now = millis();
    if (!server_) {
        if (uint32_t(now - retryAt_) >= listenRetryMs) {
            retryAt_ = now;
            server_.begin();
        }
        return;
    }
    if (!client_) {
        closeClient();
        client_ = server_.accept();
        if (!client_) return;
        acceptedAt_ = now;
    }
    if (uint32_t(now - acceptedAt_) >= requestTimeoutMs) {
        closeClient();
        return;
    }
    if (response_.isEmpty()) {
        for (size_t count = 0; count < bytesPerUpdate && client_.available(); ++count) {
            const int byte = client_.read();
            if (byte < 0) break;
            request_ += char(byte);
            if (request_.length() > maximumRequestBytes) {
                closeClient();
                return;
            }
            if (request_.endsWith("\r\n\r\n")) {
                respond();
                break;
            }
        }
        return;
    }
    // A slow browser must not stall the light-control loop.
    const int written = ::send(client_.fd(), response_.c_str() + sent_,
                               response_.length() - sent_, MSG_DONTWAIT);
    if (written < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) closeClient();
        return;
    }
    sent_ += written;
    if (sent_ == response_.length()) closeClient();
}
