#pragma once

#include <Arduino.h>
#include <Preferences.h>

enum class TimeSource : uint8_t {
  None = 0,
  Manual,
  Ntp,
};

class TimeKeeper {
 public:
  void begin();
  void loop();

  bool valid() const { return source_ != TimeSource::None; }
  uint16_t currentMinutes() const;
  TimeSource source() const { return source_; }
  const char* sourceName() const;

  bool hasWifiCredentials() const { return ssid_.length() > 0; }
  const String& wifiSsid() const { return ssid_; }

  bool saveWifiCredentials(const String& ssid, const String& password);
  void clearWifiCredentials();
  bool syncNtp();
  void setManualTime(uint8_t hour, uint8_t minute);

 private:
  void handleSerialLine(String line);
  void printStatus() const;

  Preferences prefs_;
  String ssid_;
  String password_;
  String serialLine_;

  TimeSource source_ = TimeSource::None;
  uint16_t manualBaseMinutes_ = 0;
  uint32_t manualBaseMs_ = 0;
};

extern TimeKeeper gTime;
