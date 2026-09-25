#include <Arduino.h>

#include "config.h"
#include "radio_monitor.h"
#include "scanners.h"
#include "timekeeper.h"
#include "ui.h"

void setup() {
  Serial.begin(Config::SERIAL_BAUD);
  delay(200);
  Serial.println();
  Serial.println("=== NERD-O-METER boot ===");

  gUi.begin();

  // NTP is optional. Credentials live only in the ESP32 NVS and are never
  // compiled into this public repository.
  gTime.begin();
  if (gTime.valid()) {
    const uint16_t now = gTime.currentMinutes();
    gUi.setClock(now / 60, now % 60);
  }

  gRadio.begin();
  gScanners.begin();

  Serial.println("[system] Ready");
  Serial.println("[button] click=next | hold=config | config: click=next, double=toggle");
}

void loop() {
  gUi.tickButton();
  gTime.loop();
  gRadio.loop();
  gScanners.loop();
  gUi.loop();
  delay(5);
}
