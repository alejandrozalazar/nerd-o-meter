#include "ui.h"

#include <cstdio>

#include "config.h"
#include "radio_monitor.h"
#include "scanners.h"
#include "schedule.h"
#include "timekeeper.h"

namespace {
UiController* uiInstance = nullptr;

void clickThunk() {
  if (uiInstance) uiInstance->onClick();
}

void doubleClickThunk() {
  if (uiInstance) uiInstance->onDoubleClick();
}

void longPressThunk() {
  if (uiInstance) uiInstance->onLongPress();
}

int clampInt(int value, int low, int high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

const char* const kBuzzwords[] = {
    "AGENTIC AI", "MCP", "RAG", "EMBEDDINGS", "VECTOR SEARCH",
    "MULTIMODAL", "OPEN WEIGHTS", "LLM", "KUBERNETES", "AI OS",
    "OPENTELEMETRY", "DUCKDB", "PARQUET", "UNIKERNELS",
    "PROMPT INJECTION", "AGENTS", "CONTEXT", "INFERENCE",
    "OPEN SOURCE AI", "SKILLS"};

constexpr size_t kBuzzwordCount = sizeof(kBuzzwords) / sizeof(kBuzzwords[0]);

constexpr uint32_t kScreenIntervalsMs[] = {2000, 3000, 4200, 6000, 10000, 15000};
constexpr size_t kScreenIntervalCount =
    sizeof(kScreenIntervalsMs) / sizeof(kScreenIntervalsMs[0]);

const char* const kFeatureNames[] = {
    "STATS", "NERD LEVEL", "BUZZWORD", "SCHEDULE",
    "GITHUB QR", "AGENDA QR", "LORA POPUP"};
}  // namespace

UiController gUi;

UiController::UiController()
    : display_(U8G2_R0, Config::PIN_OLED_SCL, Config::PIN_OLED_SDA, Config::PIN_OLED_RST),
      button_(Config::PIN_BUTTON, true, true) {}

void UiController::begin() {
  uiInstance = this;

  pinMode(Config::PIN_VEXT, OUTPUT);
  digitalWrite(Config::PIN_VEXT, LOW);
  delay(50);

  display_.begin();
  display_.setContrast(255);
  display_.setFontMode(1);

  prefs_.begin("nerdometer", false);
  featureMask_ = prefs_.getUChar("features", 0x7F);
  screenIntervalMs_ = prefs_.getUInt("screenMs", Config::SCREEN_MS);
  bool validInterval = false;
  for (size_t i = 0; i < kScreenIntervalCount; ++i) {
    if (screenIntervalMs_ == kScreenIntervalsMs[i]) {
      validInterval = true;
      break;
    }
  }
  if (!validInterval) screenIntervalMs_ = Config::SCREEN_MS;

  editHour_ = prefs_.getUChar("lastHour", 10);
  editMinute_ = prefs_.getUChar("lastMin", 0);
  if (editHour_ > 23) editHour_ = 10;
  if (editMinute_ > 59) editMinute_ = 0;

  // No battery-backed RTC: a stored time would silently become wrong after reboot.
  clockConfigured_ = false;

  button_.setClickMs(300);
  button_.setPressMs(1100);
  button_.attachClick(clickThunk);
  button_.attachDoubleClick(doubleClickThunk);
  button_.attachLongPressStart(longPressThunk);

  display_.clearBuffer();
  drawSplash();
  display_.sendBuffer();
  delay(Config::SPLASH_MS);

  lastScreenChangeMs_ = millis();
}

void UiController::tickButton() {
  button_.tick();
}

void UiController::loop() {
  const uint32_t now = millis();

  if (mode_ == Mode::Normal && now - lastScreenChangeMs_ >= screenIntervalMs_) {
    advanceScreen();
  }

  if (now - lastRenderMs_ < 100) return;
  lastRenderMs_ = now;

  display_.clearBuffer();
  if (mode_ == Mode::Config) {
    drawConfig();
  } else if (mode_ == Mode::ClockSet) {
    drawClockSet();
  } else if (mode_ == Mode::ResetConfirm) {
    drawResetConfirm();
  } else {
    drawNormal();
  }
  display_.sendBuffer();
}

void UiController::onClick() {
  if (mode_ == Mode::Normal) {
    advanceScreen();
  } else if (mode_ == Mode::Config) {
    configItem_ = (configItem_ + 1) % kConfigItemCount;
  } else if (mode_ == Mode::ResetConfirm) {
    mode_ = Mode::Config;
    configItem_ = kConfigResetItem;
  } else {
    if (editHours_) {
      editHour_ = (editHour_ + 1) % 24;
    } else {
      editMinute_ = (editMinute_ + 5) % 60;
    }
  }
}

void UiController::onDoubleClick() {
  if (mode_ == Mode::Normal) return;

  if (mode_ == Mode::Config) {
    if (configItem_ == kConfigClockItem) {
      enterClockSet();
    } else if (configItem_ == kConfigScreenTimeItem) {
      cycleScreenInterval();
    } else if (configItem_ == kConfigResetItem) {
      mode_ = Mode::ResetConfirm;
    } else {
      setFeatureEnabled(configItem_, !featureEnabled(configItem_));
    }
    return;
  }

  if (mode_ == Mode::ResetConfirm) {
    mode_ = Mode::Config;
    configItem_ = kConfigResetItem;
    return;
  }

  editHours_ = !editHours_;
}

void UiController::onLongPress() {
  if (mode_ == Mode::Normal) {
    enterConfig();
  } else if (mode_ == Mode::Config) {
    leaveConfig();
  } else if (mode_ == Mode::ResetConfirm) {
    clearStoredSettings();
  } else {
    saveClock();
  }
}

bool UiController::featureEnabled(uint8_t feature) const {
  return feature < FeatureCount && (featureMask_ & (1u << feature));
}

void UiController::setFeatureEnabled(uint8_t feature, bool enabled) {
  if (feature >= FeatureCount) return;

  if (enabled) {
    featureMask_ |= (1u << feature);
  } else {
    featureMask_ &= ~(1u << feature);
  }
  prefs_.putUChar("features", featureMask_);
}

bool UiController::screenAvailable(Screen screen) const {
  if (!featureEnabled(static_cast<uint8_t>(screen))) return false;
  if (screen == Schedule && !clockConfigured_) return false;
  return true;
}

bool UiController::anyScreenAvailable() const {
  for (uint8_t i = 0; i < ScreenCount; ++i) {
    if (screenAvailable(static_cast<Screen>(i))) return true;
  }
  return false;
}

void UiController::advanceScreen() {
  for (uint8_t i = 0; i < ScreenCount; ++i) {
    currentScreen_ = static_cast<Screen>(
        (static_cast<uint8_t>(currentScreen_) + 1) % ScreenCount);
    if (screenAvailable(currentScreen_)) {
      lastScreenChangeMs_ = millis();
      return;
    }
  }
  lastScreenChangeMs_ = millis();
}

void UiController::enterConfig() {
  mode_ = Mode::Config;
  configItem_ = 0;
}

void UiController::leaveConfig() {
  mode_ = Mode::Normal;
  if (anyScreenAvailable() && !screenAvailable(currentScreen_)) advanceScreen();
  lastScreenChangeMs_ = millis();
}

void UiController::enterClockSet() {
  mode_ = Mode::ClockSet;
  editHours_ = true;

  if (clockConfigured_) {
    const uint16_t now = clockMinutesNow();
    editHour_ = now / 60;
    editMinute_ = now % 60;
  }
}

void UiController::saveClock() {
  clockBaseMinutes_ = static_cast<uint16_t>(editHour_) * 60u + editMinute_;
  clockBaseMs_ = millis();
  clockConfigured_ = true;

  // Convenience only: the value is offered as the next boot's edit default,
  // but the clock is deliberately considered unset after every reboot.
  prefs_.putUChar("lastHour", editHour_);
  prefs_.putUChar("lastMin", editMinute_);

  mode_ = Mode::Config;
  configItem_ = kConfigClockItem;
  Serial.printf("[clock] Set to %02u:%02u for Nerdearla Sep 25\n",
                editHour_, editMinute_);
}

void UiController::setClock(uint8_t hour, uint8_t minute) {
  hour %= 24;
  minute %= 60;
  clockBaseMinutes_ = static_cast<uint16_t>(hour) * 60u + minute;
  clockBaseMs_ = millis();
  clockConfigured_ = true;
  editHour_ = hour;
  editMinute_ = minute;
  Serial.printf("[clock] External sync applied: %02u:%02u\n", hour, minute);
}

void UiController::cycleScreenInterval() {
  size_t current = 0;
  for (size_t i = 0; i < kScreenIntervalCount; ++i) {
    if (screenIntervalMs_ == kScreenIntervalsMs[i]) {
      current = i;
      break;
    }
  }

  current = (current + 1) % kScreenIntervalCount;
  screenIntervalMs_ = kScreenIntervalsMs[current];
  prefs_.putUInt("screenMs", screenIntervalMs_);

  Serial.printf("[ui] Screen interval: %.1f s\n",
                screenIntervalMs_ / 1000.0f);
}

void UiController::clearStoredSettings() {
  Serial.println("[settings] Clearing Nerd-O-Meter persistent settings");
  prefs_.clear();
  gTime.clearWifiCredentials();

  display_.clearBuffer();
  drawHeader("SETTINGS CLEARED");
  display_.setFont(u8g2_font_6x10_tf);
  drawCentered("RESTARTING...", 38);
  display_.sendBuffer();

  delay(500);
  ESP.restart();
}

uint16_t UiController::clockMinutesNow() const {
  if (!clockConfigured_) return 0;

  const uint32_t elapsedMinutes = (millis() - clockBaseMs_) / 60000UL;
  return static_cast<uint16_t>((clockBaseMinutes_ + elapsedMinutes) % (24 * 60));
}

void UiController::drawSplash() {
  display_.setDrawColor(1);

  // Tiny 1-bit avatar inspired by the Mii/pixel-art concept.
  display_.drawFrame(4, 7, 43, 43);
  display_.drawBox(8, 4, 31, 5);
  display_.drawBox(5, 9, 40, 8);
  display_.drawBox(4, 14, 7, 20);
  display_.drawBox(40, 14, 7, 20);
  display_.drawTriangle(10, 16, 25, 13, 10, 23);
  display_.drawTriangle(25, 13, 40, 16, 25, 23);

  display_.drawHLine(12, 25, 10);
  display_.drawHLine(29, 25, 10);
  display_.drawBox(15, 28, 4, 4);
  display_.drawBox(32, 28, 4, 4);

  display_.drawVLine(25, 29, 8);
  display_.drawHLine(25, 37, 5);

  display_.drawTriangle(14, 40, 25, 36, 22, 44);
  display_.drawTriangle(36, 40, 25, 36, 28, 44);
  display_.drawFrame(16, 40, 19, 14);
  display_.drawBox(22, 48, 7, 8);

  display_.drawLine(4, 63, 15, 54);
  display_.drawLine(15, 54, 25, 58);
  display_.drawLine(25, 58, 35, 54);
  display_.drawLine(35, 54, 47, 63);

  display_.setFont(u8g2_font_6x10_tf);
  display_.drawStr(52, 12, "NERD-O-");
  display_.drawStr(52, 23, "METER");
  display_.drawHLine(52, 26, 73);

  display_.setFont(u8g2_font_5x7_tf);
  display_.drawStr(54, 37, "WiFi BLE LoRa");
  display_.drawStr(54, 49, "SCAN+EXPLORE");
  display_.drawStr(72, 60, "+NERD");
}

void UiController::drawNormal() {
  const MeshPacketInfo& packet = gRadio.lastPacket();
  if (featureEnabled(FeatureLoraPopup) && packet.valid &&
      millis() - packet.receivedAtMs <= Config::LORA_POPUP_MS) {
    drawMeshPopup();
    return;
  }

  if (!anyScreenAvailable()) {
    drawHeader("NERD-O-METER");
    display_.setFont(u8g2_font_6x10_tf);
    drawCentered("ALL SCREENS OFF", 34);
    display_.setFont(u8g2_font_5x7_tf);
    drawCentered("hold button -> config", 53);
    return;
  }

  if (!screenAvailable(currentScreen_)) advanceScreen();

  switch (currentScreen_) {
    case Stats:
      drawStats();
      break;
    case NerdLevel:
      drawNerdLevel();
      break;
    case Buzzword:
      drawBuzzword();
      break;
    case Schedule:
      drawSchedule();
      break;
    case GithubQr:
      drawQr(kGithubQr, "SOURCE", "NERD-O-", "METER");
      break;
    case AgendaQr:
      drawQr(kAgendaQr, "NERDEARLA", "AGENDA", "SCAN ME");
      break;
    default:
      drawStats();
      break;
  }
}

void UiController::drawHeader(const char* title) {
  display_.setFont(u8g2_font_6x10_tf);
  display_.drawBox(0, 0, 128, 11);
  display_.setDrawColor(0);
  display_.drawStr(3, 9, title);
  display_.setDrawColor(1);
}

void UiController::drawStats() {
  drawHeader("RADIO SCANNER");
  display_.setFont(u8g2_font_6x10_tf);

  char line[24];
  snprintf(line, sizeof(line), "WiFi  %3d APs", gScanners.wifiCount());
  display_.drawStr(4, 24, line);

  snprintf(line, sizeof(line), "BLE   %3d dev", gScanners.bleCount());
  display_.drawStr(4, 36, line);

  snprintf(line, sizeof(line), "Mesh  %3u /60s",
           gRadio.packetsInWindow(Config::MESH_ACTIVITY_WINDOW_MS));
  display_.drawStr(4, 48, line);

  snprintf(line, sizeof(line), "Nodes %3u /5m",
           gRadio.uniqueNodesInWindow(Config::NODE_WINDOW_MS));
  display_.drawStr(4, 60, line);
}

void UiController::drawNerdLevel() {
  drawHeader("NERD-O-METER");

  const int wifi = clampInt(gScanners.wifiCount(), 0, 120);
  const int ble = clampInt(gScanners.bleCount(), 0, 80);
  const int mesh = clampInt(
      gRadio.packetsInWindow(Config::MESH_ACTIVITY_WINDOW_MS), 0, 20);

  const int wifiScore = wifi * 35 / 120;
  const int bleScore = ble * 35 / 80;
  const int meshScore = mesh * 30 / 20;
  const int nerd = clampInt(wifiScore + bleScore + meshScore, 0, 100);

  display_.setFont(u8g2_font_5x7_tf);
  display_.drawStr(2, 21, "WIFI");
  display_.drawStr(2, 31, "BLE");
  display_.drawStr(2, 41, "MESH");

  drawBar(30, 16, 92, 6, wifi * 100 / 120);
  drawBar(30, 26, 92, 6, ble * 100 / 80);
  drawBar(30, 36, 92, 6, mesh * 100 / 20);

  char level[24];
  snprintf(level, sizeof(level), "NERD LEVEL: %d%%", nerd);
  display_.setFont(u8g2_font_6x10_tf);
  drawCentered(level, 59);
}

void UiController::drawBuzzword() {
  drawHeader("BUZZWORD OF THE MOMENT");
  const size_t index = (millis() / screenIntervalMs_) % kBuzzwordCount;

  display_.setFont(u8g2_font_9x15B_tf);
  drawWrapped(kBuzzwords[index], 4, 31, 120, 17, 2);

  display_.setFont(u8g2_font_5x7_tf);
  drawCentered("Nerdearla '26 edition", 62);
}

void UiController::drawSchedule() {
  const uint16_t now = clockMinutesNow();
  const ScheduleSelection selected =
      selectScheduleEvent(now, millis() / 3500UL);

  char clockText[8];
  formatClock(clockText, sizeof(clockText), now);

  char header[24];
  snprintf(header, sizeof(header), "NERDEARLA  %s", clockText);
  drawHeader(header);

  if (!selected.hasEvent) {
    display_.setFont(u8g2_font_6x10_tf);
    drawCentered("DONE FOR TODAY", 34);
    display_.setFont(u8g2_font_5x7_tf);
    drawCentered("agenda QR -> next screen", 54);
    return;
  }

  const ScheduleEvent& event = *selected.event;
  char start[8];
  formatClock(start, sizeof(start), event.startMinutes);

  char timeText[24];
  snprintf(timeText, sizeof(timeText), "%s %s  %u/%u",
           selected.isCurrent ? "NOW" : "NEXT",
           start,
           selected.selectedCandidate + 1,
           selected.candidateCount);

  display_.setFont(u8g2_font_5x7_tf);
  display_.drawStr(2, 19, timeText);
  display_.drawStr(2, 27, event.room);
  display_.drawHLine(0, 30, 128);
  drawWrapped(event.title, 2, 39, 124, 8, 3);
}

void UiController::drawQr(const QrBitmap& qr,
                          const char* line1,
                          const char* line2,
                          const char* line3) {
  constexpr int scale = 2;
  constexpr int x0 = 1;
  constexpr int y0 = 1;
  const int side = qr.size * scale;

  // Cyan/light square with black QR modules: conventional QR polarity.
  display_.setDrawColor(1);
  display_.drawBox(x0, y0, side, side);
  display_.setDrawColor(0);

  for (uint8_t y = 0; y < qr.size; ++y) {
    const uint32_t row = pgm_read_dword(&qr.rows[y]);
    for (uint8_t x = 0; x < qr.size; ++x) {
      if (row & (1UL << x)) {
        display_.drawBox(x0 + x * scale, y0 + y * scale, scale, scale);
      }
    }
  }

  display_.setDrawColor(1);
  const int tx = 66;

  display_.setFont(u8g2_font_6x10_tf);
  display_.drawStr(tx, 18, line1);

  display_.setFont(u8g2_font_9x15B_tf);
  display_.drawStr(tx, 37, line2);
  display_.drawStr(tx, 53, line3);

  display_.setFont(u8g2_font_5x7_tf);
  display_.drawStr(tx, 63, "scan -> phone");
}

void UiController::drawMeshPopup() {
  const MeshPacketInfo& p = gRadio.lastPacket();
  drawHeader("MESHTASTIC RX!");

  display_.setFont(u8g2_font_6x10_tf);
  char line[28];

  snprintf(line, sizeof(line), "FROM !%08lX",
           static_cast<unsigned long>(p.from));
  display_.drawStr(3, 25, line);

  snprintf(line, sizeof(line), "RSSI %d  SNR %+.1f", p.rssi, p.snr);
  display_.drawStr(3, 38, line);

  snprintf(line, sizeof(line), "CH %u  HOPS %u/%u",
           p.channelHash, p.hopLimit, p.hopStart);
  display_.drawStr(3, 51, line);

  snprintf(line, sizeof(line), "TOTAL %lu",
           static_cast<unsigned long>(gRadio.totalPackets()));
  display_.drawStr(3, 63, line);
}

void UiController::drawConfig() {
  drawHeader("CONFIG - HOLD TO EXIT");

  display_.setFont(u8g2_font_5x7_tf);
  char pos[12];
  snprintf(pos, sizeof(pos), "%u/%u", configItem_ + 1, kConfigItemCount);
  display_.drawStr(106, 20, pos);

  if (configItem_ == kConfigClockItem) {
    display_.setFont(u8g2_font_9x15B_tf);
    display_.drawStr(4, 37, "CLOCK");

    display_.setFont(u8g2_font_6x10_tf);
    if (clockConfigured_) {
      char timeText[8];
      formatClock(timeText, sizeof(timeText), clockMinutesNow());
      display_.drawStr(4, 50, timeText);
    } else {
      display_.setFont(u8g2_font_5x7_tf);
      display_.drawStr(4, 50, "NOT SET = SCHEDULE OFF");
    }

    display_.setFont(u8g2_font_5x7_tf);
    display_.drawStr(4, 62, "double: set time");
    return;
  }

  if (configItem_ == kConfigScreenTimeItem) {
    display_.setFont(u8g2_font_9x15B_tf);
    display_.drawStr(4, 38, "SCREEN TIME");

    char intervalText[24];
    snprintf(intervalText, sizeof(intervalText), "%.1f sec",
             screenIntervalMs_ / 1000.0f);
    display_.setFont(u8g2_font_6x10_tf);
    display_.drawStr(4, 52, intervalText);

    display_.setFont(u8g2_font_5x7_tf);
    display_.drawStr(4, 63, "double: next preset");
    return;
  }

  if (configItem_ == kConfigResetItem) {
    display_.setFont(u8g2_font_9x15B_tf);
    display_.drawStr(4, 38, "RESET CFG");
    display_.setFont(u8g2_font_5x7_tf);
    display_.drawStr(4, 52, "screens + WiFi + defaults");
    display_.drawStr(4, 63, "double: confirmation");
    return;
  }

  display_.setFont(u8g2_font_9x15B_tf);
  display_.drawStr(4, 38, kFeatureNames[configItem_]);

  display_.setFont(u8g2_font_6x10_tf);
  display_.drawStr(4, 52,
                   featureEnabled(configItem_) ? "[ ON ]" : "[ OFF ]");

  display_.setFont(u8g2_font_5x7_tf);
  display_.drawStr(4, 63, "click next / double toggle");
}

void UiController::drawResetConfirm() {
  drawHeader("RESET CONFIG?");

  display_.setFont(u8g2_font_6x10_tf);
  drawCentered("ERASE SAVED SETTINGS", 28);
  drawCentered("INCLUDING WIFI", 40);

  display_.setFont(u8g2_font_5x7_tf);
  drawCentered("HOLD = ERASE + RESTART", 54);
  drawCentered("click/double = cancel", 63);
}

void UiController::drawClockSet() {
  drawHeader("SET CLOCK - HOLD SAVE");

  char timeText[8];
  snprintf(timeText, sizeof(timeText), "%02u:%02u", editHour_, editMinute_);

  display_.setFont(u8g2_font_logisoso20_tf);
  drawCentered(timeText, 40);

  display_.setFont(u8g2_font_5x7_tf);
  drawCentered(editHours_ ? "HOURS: click +1" : "MINUTES: click +5", 52);
  drawCentered("double: switch field", 63);
}

void UiController::drawWrapped(const char* text,
                               int x,
                               int y,
                               int width,
                               int lineHeight,
                               int maxLines) {
  String remaining(text);
  int line = 0;

  while (remaining.length() > 0 && line < maxLines) {
    String current;
    int pos = 0;

    while (pos < static_cast<int>(remaining.length())) {
      int space = remaining.indexOf(' ', pos);
      if (space < 0) space = remaining.length();

      const String word = remaining.substring(pos, space);
      const String candidate =
          current.length() ? current + " " + word : word;

      if (display_.getStrWidth(candidate.c_str()) > width &&
          current.length()) {
        break;
      }

      current = candidate;
      pos = space + 1;
      if (space == static_cast<int>(remaining.length())) break;
    }

    if (current.length() == 0) {
      int cut = remaining.length() < 18 ? remaining.length() : 18;
      current = remaining.substring(0, cut);
      pos = cut;
    }

    display_.drawStr(x, y + line * lineHeight, current.c_str());

    if (pos > static_cast<int>(remaining.length())) {
      pos = remaining.length();
    }
    remaining = remaining.substring(pos);
    remaining.trim();
    ++line;
  }
}

void UiController::drawBar(int x,
                           int y,
                           int width,
                           int height,
                           int percent) {
  percent = clampInt(percent, 0, 100);

  display_.drawFrame(x, y, width, height);
  const int fill = (width - 2) * percent / 100;
  if (fill > 0) {
    display_.drawBox(x + 1, y + 1, fill, height - 2);
  }
}

void UiController::drawCentered(const char* text, int y) {
  const int width = display_.getStrWidth(text);
  int x = (128 - width) / 2;
  if (x < 0) x = 0;
  display_.drawStr(x, y, text);
}

void UiController::formatClock(char* out,
                               size_t len,
                               uint16_t minutes) const {
  snprintf(out, len, "%02u:%02u", minutes / 60, minutes % 60);
}
