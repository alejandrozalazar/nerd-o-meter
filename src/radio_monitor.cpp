#include "radio_monitor.h"

#include <RadioLib.h>
#include <SPI.h>

#include "config.h"

namespace {
SX1262 radio = new Module(Config::PIN_LORA_NSS, Config::PIN_LORA_DIO1,
                           Config::PIN_LORA_RST, Config::PIN_LORA_BUSY);
volatile bool packetReceived = false;

void IRAM_ATTR onPacketReceived() {
  packetReceived = true;
}

uint32_t readLe32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) |
         (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}
}  // namespace

RadioMonitor gRadio;

bool RadioMonitor::begin() {
  SPI.begin(Config::PIN_LORA_SCK, Config::PIN_LORA_MISO,
            Config::PIN_LORA_MOSI, Config::PIN_LORA_NSS);

  lastError_ = radio.begin(
      Config::MESH_FREQUENCY_MHZ,
      Config::MESH_BANDWIDTH_KHZ,
      Config::MESH_SPREADING_FACTOR,
      Config::MESH_CODING_RATE,
      Config::MESH_SYNC_WORD,
      10,
      Config::MESH_PREAMBLE_LENGTH,
      Config::MESH_TCXO_VOLTAGE,
      false);

  if (lastError_ != RADIOLIB_ERR_NONE) {
    Serial.printf("[radio] SX1262 init failed: %d\n", lastError_);
    return false;
  }

  radio.setDio2AsRfSwitch(true);
  radio.setCRC(2);
  radio.setPacketReceivedAction(onPacketReceived);

  lastError_ = radio.startReceive();
  if (lastError_ != RADIOLIB_ERR_NONE) {
    Serial.printf("[radio] startReceive failed: %d\n", lastError_);
    return false;
  }

  ready_ = true;
  Serial.printf("[radio] Meshtastic ANZ/LongFast RX at %.3f MHz\n",
                Config::MESH_FREQUENCY_MHZ);
  return true;
}

void RadioMonitor::loop() {
  if (!ready_ || !packetReceived) {
    return;
  }

  noInterrupts();
  packetReceived = false;
  interrupts();
  processReceivedPacket();
}

void RadioMonitor::processReceivedPacket() {
  const size_t length = radio.getPacketLength();
  if (length == 0 || length > 255) {
    radio.startReceive();
    return;
  }

  uint8_t buffer[255];
  const int16_t state = radio.readData(buffer, length);
  const uint32_t now = millis();

  if (state == RADIOLIB_ERR_NONE && length >= 16) {
    lastPacket_.valid = true;
    lastPacket_.to = readLe32(buffer + 0);
    lastPacket_.from = readLe32(buffer + 4);
    lastPacket_.packetId = readLe32(buffer + 8);
    const uint8_t flags = buffer[12];
    lastPacket_.channelHash = buffer[13];
    lastPacket_.hopLimit = flags & 0x07;
    lastPacket_.hopStart = (flags >> 5) & 0x07;
    lastPacket_.rssi = static_cast<int16_t>(radio.getRSSI());
    lastPacket_.snr = radio.getSNR();
    lastPacket_.length = length;
    lastPacket_.receivedAtMs = now;

    ++totalPackets_;
    rememberPacketTime(now);
    rememberNode(lastPacket_.from, now);

    Serial.printf(
        "[mesh] #%lu from=!%08lX ch=%u len=%u RSSI=%d SNR=%.1f hops=%u/%u\n",
        static_cast<unsigned long>(totalPackets_),
        static_cast<unsigned long>(lastPacket_.from),
        lastPacket_.channelHash,
        static_cast<unsigned>(lastPacket_.length),
        lastPacket_.rssi,
        lastPacket_.snr,
        lastPacket_.hopLimit,
        lastPacket_.hopStart);
  } else if (state != RADIOLIB_ERR_NONE && state != RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.printf("[radio] receive error: %d\n", state);
  }

  lastError_ = radio.startReceive();
}

void RadioMonitor::rememberPacketTime(uint32_t now) {
  packetTimes_[packetTimeHead_] = now;
  packetTimeHead_ = (packetTimeHead_ + 1) % kPacketHistory;
}

void RadioMonitor::rememberNode(uint32_t node, uint32_t now) {
  if (node == 0) return;

  uint8_t oldestIndex = 0;
  uint32_t oldestAge = 0;
  for (uint8_t i = 0; i < kNodeHistory; ++i) {
    if (nodes_[i].node == node) {
      nodes_[i].seenAtMs = now;
      return;
    }
    if (nodes_[i].node == 0) {
      nodes_[i] = {node, now};
      return;
    }
    const uint32_t age = now - nodes_[i].seenAtMs;
    if (age >= oldestAge) {
      oldestAge = age;
      oldestIndex = i;
    }
  }
  nodes_[oldestIndex] = {node, now};
}

uint16_t RadioMonitor::packetsInWindow(uint32_t windowMs) const {
  const uint32_t now = millis();
  uint16_t count = 0;
  for (uint8_t i = 0; i < kPacketHistory; ++i) {
    if (packetTimes_[i] != 0 && now - packetTimes_[i] <= windowMs) ++count;
  }
  return count;
}

uint8_t RadioMonitor::uniqueNodesInWindow(uint32_t windowMs) const {
  const uint32_t now = millis();
  uint8_t count = 0;
  for (uint8_t i = 0; i < kNodeHistory; ++i) {
    if (nodes_[i].node != 0 && now - nodes_[i].seenAtMs <= windowMs) ++count;
  }
  return count;
}
