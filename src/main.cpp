#include <Arduino.h>

#include "config.h"
#include "radio_monitor.h"
#include "scanners.h"
#include "ui.h"

void setup() {
  Serial.begin(Config::SERIAL_BAUD);
  delay(200);
  Serial.println();
  Serial.println("=== NERD-O-METER boot ===");

  gUi.begin();
  gRadio.begin();
  gScanners.begin();

  Serial.println("[system] Ready");
  Serial.println("[button] click=next | hold=config | config: click=next, double=toggle");
}

void loop() {
  gUi.tickButton();
  gRadio.loop();
  gScanners.loop();
  gUi.loop();
  delay(5);
}
