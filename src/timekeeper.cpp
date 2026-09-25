#include "timekeeper.h"

#include <WiFi.h>
#include <time.h>

namespace {
constexpr char kPrefsNamespace[] = "nerdnet";
constexpr int32_t kArgentinaUtcOffsetSeconds = -3 * 60 * 60;
constexpr uint32_t kWifiTimeoutMs = 7000;
constexpr uint32_t kNtpTimeoutMs = 5000;
constexpr size_t kMaxSerialLine = 192;

bool validClock(const tm& info) {
  return info.tm_year + 1900 >= 2025;
}
}  // namespace

TimeKeeper gTime;

void TimeKeeper::begin() {
  prefs_.begin(kPrefsNamespace, false);
  ssid_ = prefs_.getString("ssid", "");
  password_ = prefs_.getString("pass", "");

  // There is no battery-backed RTC on the Heltec V3. Never pretend a
  // pre-power-loss wall clock is still valid.
  source_ = TimeSource::None;

  Serial.printf("[time] Wi-Fi credentials: %s\n",
                hasWifiCredentials() ? "stored in NVS" : "not configured");

  if (hasWifiCredentials()) {
    syncNtp();
  } else {
    Serial.println("[time] Clock unset; schedule remains disabled");
    Serial.println("[time] Provision Wi-Fi with scripts/configure-wifi.sh or set time manually");
  }
}

void TimeKeeper::loop() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());

    if (c == '\r') continue;

    if (c == '\n') {
      if (serialLine_.length() > 0) {
        handleSerialLine(serialLine_);
        serialLine_ = "";
      }
      continue;
    }

    if (serialLine_.length() < kMaxSerialLine) {
      serialLine_ += c;
    } else {
      serialLine_ = "";
      Serial.println("[serial] Input discarded: line too long");
    }
  }
}

bool TimeKeeper::saveWifiCredentials(const String& ssid,
                                     const String& password) {
  if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 64) {
    Serial.println("[time] Invalid Wi-Fi credential lengths");
    return false;
  }

  ssid_ = ssid;
  password_ = password;
  prefs_.putString("ssid", ssid_);
  prefs_.putString("pass", password_);

  Serial.printf("[time] Wi-Fi credentials saved in ESP32 NVS for SSID '%s'\n",
                ssid_.c_str());
  return true;
}

void TimeKeeper::clearWifiCredentials() {
  ssid_ = "";
  password_ = "";
  prefs_.remove("ssid");
  prefs_.remove("pass");

  // Also erase any AP credentials the Arduino Wi-Fi stack may have cached.
  WiFi.disconnect(true, true);
  Serial.println("[time] Stored Wi-Fi credentials cleared");
}

bool TimeKeeper::syncNtp() {
  if (!hasWifiCredentials()) {
    Serial.println("[time] NTP skipped: no Wi-Fi credentials");
    return false;
  }

  const TimeSource previousSource = source_;
  const uint16_t previousManualMinutes = manualBaseMinutes_;
  const uint32_t previousManualMs = manualBaseMs_;

  Serial.printf("[time] Connecting to '%s' for NTP...\n", ssid_.c_str());

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  delay(100);
  WiFi.begin(ssid_.c_str(), password_.c_str());

  const uint32_t wifiStarted = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - wifiStarted < kWifiTimeoutMs) {
    delay(100);
    yield();
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[time] Wi-Fi connection failed/timed out; clock unchanged");
    WiFi.disconnect(false, false);
    source_ = previousSource;
    manualBaseMinutes_ = previousManualMinutes;
    manualBaseMs_ = previousManualMs;
    return false;
  }

  Serial.printf("[time] Wi-Fi connected; RSSI=%d dBm. Syncing NTP...\n",
                WiFi.RSSI());

  configTime(kArgentinaUtcOffsetSeconds, 0,
             "pool.ntp.org",
             "time.google.com",
             "time.cloudflare.com");

  tm info = {};
  const uint32_t ntpStarted = millis();
  bool synced = false;

  while (millis() - ntpStarted < kNtpTimeoutMs) {
    if (getLocalTime(&info, 500) && validClock(info)) {
      synced = true;
      break;
    }
    yield();
  }

  WiFi.disconnect(false, false);

  if (!synced) {
    Serial.println("[time] NTP failed/timed out; clock unchanged");
    source_ = previousSource;
    manualBaseMinutes_ = previousManualMinutes;
    manualBaseMs_ = previousManualMs;
    return false;
  }

  source_ = TimeSource::Ntp;
  Serial.printf("[time] NTP OK: %04d-%02d-%02d %02d:%02d:%02d ART\n",
                info.tm_year + 1900,
                info.tm_mon + 1,
                info.tm_mday,
                info.tm_hour,
                info.tm_min,
                info.tm_sec);
  return true;
}

void TimeKeeper::setManualTime(uint8_t hour, uint8_t minute) {
  hour %= 24;
  minute %= 60;

  manualBaseMinutes_ = static_cast<uint16_t>(hour) * 60u + minute;
  manualBaseMs_ = millis();
  source_ = TimeSource::Manual;

  Serial.printf("[time] Manual clock set: %02u:%02u\n", hour, minute);
}

uint16_t TimeKeeper::currentMinutes() const {
  if (source_ == TimeSource::Ntp) {
    tm info = {};
    if (getLocalTime(&info, 10) && validClock(info)) {
      return static_cast<uint16_t>(info.tm_hour * 60 + info.tm_min);
    }
    return 0;
  }

  if (source_ == TimeSource::Manual) {
    const uint32_t elapsedMinutes = (millis() - manualBaseMs_) / 60000UL;
    return static_cast<uint16_t>(
        (manualBaseMinutes_ + elapsedMinutes) % (24 * 60));
  }

  return 0;
}

const char* TimeKeeper::sourceName() const {
  switch (source_) {
    case TimeSource::Manual:
      return "MANUAL";
    case TimeSource::Ntp:
      return "NTP";
    default:
      return "UNSET";
  }
}

void TimeKeeper::handleSerialLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  if (line == "NTP") {
    syncNtp();
    return;
  }

  if (line == "STATUS") {
    printStatus();
    return;
  }

  if (line == "CLEAR_WIFI") {
    clearWifiCredentials();
    return;
  }

  if (line.startsWith("WIFI\t")) {
    const int firstTab = line.indexOf('\t');
    const int secondTab = line.indexOf('\t', firstTab + 1);

    if (firstTab < 0 || secondTab < 0) {
      Serial.println("[serial] WIFI command format: WIFI<TAB>ssid<TAB>password");
      return;
    }

    const String ssid = line.substring(firstTab + 1, secondTab);
    const String password = line.substring(secondTab + 1);

    if (saveWifiCredentials(ssid, password)) {
      syncNtp();
    }
    return;
  }

  Serial.printf("[serial] Unknown command: %s\n", line.c_str());
  Serial.println("[serial] Commands: STATUS | NTP | WIFI<TAB>ssid<TAB>password | CLEAR_WIFI");
}

void TimeKeeper::printStatus() const {
  Serial.printf("[status] Wi-Fi configured: %s\n",
                hasWifiCredentials() ? "yes" : "no");
  if (hasWifiCredentials()) {
    Serial.printf("[status] Wi-Fi SSID: %s\n", ssid_.c_str());
  }

  Serial.printf("[status] Clock: %s\n", sourceName());
  if (valid()) {
    const uint16_t now = currentMinutes();
    Serial.printf("[status] Time: %02u:%02u\n", now / 60, now % 60);
  }
}
