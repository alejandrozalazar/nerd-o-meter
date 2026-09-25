#pragma once

#include <Arduino.h>

class EnvironmentScanners {
 public:
  void begin();
  void loop();

  int wifiCount() const { return wifiCount_; }
  int bleCount() const { return bleCount_; }
  bool scanning() const { return scanning_; }

 private:
  void startWifiScan();
  void pollWifiScan();
  void scanBle();

  int wifiCount_ = 0;
  int bleCount_ = 0;
  bool scanning_ = false;
  bool wifiScanRunning_ = false;
  uint32_t lastWifiScanMs_ = 0;
  uint32_t lastBleScanMs_ = 0;
};

extern EnvironmentScanners gScanners;
