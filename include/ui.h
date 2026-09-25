#pragma once

#include <Arduino.h>
#include <OneButton.h>
#include <Preferences.h>
#include <U8g2lib.h>

#include "assets.h"

class UiController {
 public:
  UiController();
  void begin();
  void tickButton();
  void loop();

  void onClick();
  void onDoubleClick();
  void onLongPress();

  void setClock(uint8_t hour, uint8_t minute);

 private:
  enum class Mode : uint8_t { Normal, Config, ClockSet };
  enum Screen : uint8_t {
    Stats = 0,
    NerdLevel,
    Buzzword,
    Schedule,
    GithubQr,
    AgendaQr,
    ScreenCount
  };
  enum Feature : uint8_t {
    FeatureStats = 0,
    FeatureNerd,
    FeatureBuzzword,
    FeatureSchedule,
    FeatureGithubQr,
    FeatureAgendaQr,
    FeatureLoraPopup,
    FeatureCount
  };

  static constexpr uint8_t kConfigClockItem = FeatureCount;
  static constexpr uint8_t kConfigItemCount = FeatureCount + 1;

  bool featureEnabled(uint8_t feature) const;
  void setFeatureEnabled(uint8_t feature, bool enabled);
  bool screenAvailable(Screen screen) const;
  bool anyScreenAvailable() const;
  void advanceScreen();
  void enterConfig();
  void leaveConfig();
  void enterClockSet();
  void saveClock();

  uint16_t clockMinutesNow() const;
  void drawSplash();
  void drawNormal();
  void drawStats();
  void drawNerdLevel();
  void drawBuzzword();
  void drawSchedule();
  void drawQr(const QrBitmap& qr, const char* line1, const char* line2, const char* line3);
  void drawMeshPopup();
  void drawConfig();
  void drawClockSet();
  void drawHeader(const char* title);
  void drawWrapped(const char* text, int x, int y, int width, int lineHeight, int maxLines);
  void drawBar(int x, int y, int width, int height, int percent);
  void drawCentered(const char* text, int y);
  void formatClock(char* out, size_t len, uint16_t minutes) const;

  U8G2_SSD1306_128X64_NONAME_F_SW_I2C display_;
  OneButton button_;
  Preferences prefs_;
  Mode mode_ = Mode::Normal;
  Screen currentScreen_ = Stats;
  uint8_t featureMask_ = 0x7F;
  uint8_t configItem_ = 0;
  uint32_t lastScreenChangeMs_ = 0;
  uint32_t lastRenderMs_ = 0;

  bool clockConfigured_ = false;
  uint16_t clockBaseMinutes_ = 10 * 60;
  uint32_t clockBaseMs_ = 0;
  uint8_t editHour_ = 10;
  uint8_t editMinute_ = 0;
  bool editHours_ = true;
};

extern UiController gUi;
