#pragma once

#include <Arduino.h>

namespace Config {

constexpr char PROJECT_NAME[] = "NERD-O-METER";
constexpr char PROJECT_URL[] = "https://github.com/alejandrozalazar/nerd-o-meter";
constexpr char AGENDA_URL[] = "https://nerdearla.com/en/argentina/schedule/";

constexpr uint8_t PIN_BUTTON = 0;
constexpr uint8_t PIN_VEXT = 36;
constexpr uint8_t PIN_OLED_SDA = 17;
constexpr uint8_t PIN_OLED_SCL = 18;
constexpr uint8_t PIN_OLED_RST = 21;

constexpr uint8_t PIN_LORA_NSS = 8;
constexpr uint8_t PIN_LORA_SCK = 9;
constexpr uint8_t PIN_LORA_MOSI = 10;
constexpr uint8_t PIN_LORA_MISO = 11;
constexpr uint8_t PIN_LORA_RST = 12;
constexpr uint8_t PIN_LORA_BUSY = 13;
constexpr uint8_t PIN_LORA_DIO1 = 14;

// Meshtastic Argentina community convention: ANZ + LongFast.
// Receive-only; this firmware never transmits LoRa packets.
constexpr float MESH_FREQUENCY_MHZ = 919.875f;
constexpr float MESH_BANDWIDTH_KHZ = 250.0f;
constexpr uint8_t MESH_SPREADING_FACTOR = 11;
constexpr uint8_t MESH_CODING_RATE = 5;  // LoRa 4/5
constexpr uint8_t MESH_SYNC_WORD = 0x2B;
constexpr uint16_t MESH_PREAMBLE_LENGTH = 16;
constexpr float MESH_TCXO_VOLTAGE = 1.8f;

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t SPLASH_MS = 1700;
constexpr uint32_t SCREEN_MS = 4200;
constexpr uint32_t LORA_POPUP_MS = 3200;
constexpr uint32_t WIFI_SCAN_INTERVAL_MS = 30000;
constexpr uint32_t BLE_SCAN_INTERVAL_MS = 35000;
constexpr uint32_t BLE_SCAN_SECONDS = 1;
constexpr uint32_t MESH_ACTIVITY_WINDOW_MS = 60000;
constexpr uint32_t NODE_WINDOW_MS = 300000;

constexpr uint8_t EVENT_DAY = 25;
constexpr uint8_t EVENT_MONTH = 9;
constexpr uint16_t EVENT_YEAR = 2026;

}  // namespace Config
