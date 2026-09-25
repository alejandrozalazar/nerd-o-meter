#pragma once

#include <Arduino.h>

struct ScheduleEvent {
  uint16_t startMinutes;
  uint16_t endMinutes;
  const char* title;
  const char* room;
};

struct ScheduleSelection {
  bool hasEvent;
  bool isCurrent;
  uint8_t candidateCount;
  uint8_t selectedCandidate;
  const ScheduleEvent* event;
};

extern const ScheduleEvent kSchedule[];
extern const size_t kScheduleCount;

ScheduleSelection selectScheduleEvent(uint16_t nowMinutes, uint32_t rotationSeed);
