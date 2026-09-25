#pragma once

#include <Arduino.h>

struct MeshPacketInfo {
  bool valid = false;
  uint32_t from = 0;
  uint32_t to = 0;
  uint32_t packetId = 0;
  uint8_t channelHash = 0;
  uint8_t hopLimit = 0;
  uint8_t hopStart = 0;
  int16_t rssi = 0;
  float snr = 0;
  size_t length = 0;
  uint32_t receivedAtMs = 0;
};

class RadioMonitor {
 public:
  bool begin();
  void loop();

  bool ready() const { return ready_; }
  int16_t lastError() const { return lastError_; }
  uint32_t totalPackets() const { return totalPackets_; }
  uint16_t packetsInWindow(uint32_t windowMs) const;
  uint8_t uniqueNodesInWindow(uint32_t windowMs) const;
  const MeshPacketInfo& lastPacket() const { return lastPacket_; }

 private:
  static constexpr uint8_t kPacketHistory = 64;
  static constexpr uint8_t kNodeHistory = 32;

  struct NodeSeen {
    uint32_t node = 0;
    uint32_t seenAtMs = 0;
  };

  void rememberPacketTime(uint32_t now);
  void rememberNode(uint32_t node, uint32_t now);
  void processReceivedPacket();

  bool ready_ = false;
  int16_t lastError_ = 0;
  uint32_t totalPackets_ = 0;
  uint32_t packetTimes_[kPacketHistory] = {};
  uint8_t packetTimeHead_ = 0;
  NodeSeen nodes_[kNodeHistory] = {};
  MeshPacketInfo lastPacket_;
};

extern RadioMonitor gRadio;
