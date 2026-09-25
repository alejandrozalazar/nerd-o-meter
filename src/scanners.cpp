#include "scanners.h"

#include <NimBLEDevice.h>
#include <WiFi.h>

#include "config.h"

EnvironmentScanners gScanners;

void EnvironmentScanners::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);

  NimBLEDevice::init("");
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(false);
  scan->setInterval(80);
  scan->setWindow(40);

  lastWifiScanMs_ = 0;
  lastBleScanMs_ = millis();
  startWifiScan();
}

void EnvironmentScanners::loop() {
  pollWifiScan();
  const uint32_t now = millis();

  if (!wifiScanRunning_ && now - lastWifiScanMs_ >= Config::WIFI_SCAN_INTERVAL_MS) {
    startWifiScan();
  }

  if (!wifiScanRunning_ && now - lastBleScanMs_ >= Config::BLE_SCAN_INTERVAL_MS) {
    scanBle();
    lastBleScanMs_ = millis();
  }
}

void EnvironmentScanners::startWifiScan() {
  if (wifiScanRunning_) return;

  Serial.println("[scan] Wi-Fi scan started");
  const int16_t state = WiFi.scanNetworks(true, true);
  if (state == WIFI_SCAN_FAILED) {
    Serial.println("[scan] Wi-Fi scan failed to start");
    lastWifiScanMs_ = millis();
    return;
  }
  wifiScanRunning_ = true;
  scanning_ = true;
}

void EnvironmentScanners::pollWifiScan() {
  if (!wifiScanRunning_) return;

  const int16_t result = WiFi.scanComplete();
  if (result == WIFI_SCAN_RUNNING) return;

  if (result >= 0) {
    wifiCount_ = result;
    Serial.printf("[scan] Wi-Fi APs: %d\n", wifiCount_);
  } else {
    Serial.printf("[scan] Wi-Fi scan error: %d\n", result);
  }

  WiFi.scanDelete();
  wifiScanRunning_ = false;
  scanning_ = false;
  lastWifiScanMs_ = millis();
}

void EnvironmentScanners::scanBle() {
  scanning_ = true;
  Serial.println("[scan] BLE scan...");
  NimBLEScan* scan = NimBLEDevice::getScan();
  NimBLEScanResults results = scan->start(Config::BLE_SCAN_SECONDS, false);
  bleCount_ = results.getCount();
  scan->clearResults();
  Serial.printf("[scan] BLE devices: %d\n", bleCount_);
  scanning_ = false;
}
