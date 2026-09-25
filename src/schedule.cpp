#include "schedule.h"

namespace {
constexpr uint16_t HM(uint8_t h, uint8_t m) {
  return static_cast<uint16_t>(h) * 60u + m;
}
}

// Snapshot of the official Nerdearla Argentina Friday Sep 25, 2026 agenda.
// Text is ASCII-normalized for the tiny OLED font.
const ScheduleEvent kSchedule[] = {
    {HM(10, 0), HM(10, 10), "Welcome remarks", "Gran sala"},
    {HM(10, 10), HM(10, 50), "The Last Software Engineer", "Gran sala"},
    {HM(10, 15), HM(10, 55), "Poor man's lakehouse: MySQL + DuckDB + Parquet", "Sala Abasto"},
    {HM(10, 20), HM(11, 0), "Kubernetes Is the AI OS", "Auditorio"},
    {HM(10, 30), HM(11, 30), "Tips para trabajar en tech", "Container gris"},
    {HM(10, 50), HM(11, 30), "Undoing Our Own Mistakes", "Gran sala"},
    {HM(10, 55), HM(11, 35), "Como funciona realmente una CDN", "Sala Abasto"},
    {HM(11, 0), HM(11, 40), "Open Source in the Age of AI", "Auditorio"},
    {HM(11, 30), HM(12, 10), "Back to the Source", "Gran sala"},
    {HM(11, 35), HM(12, 15), "Scaling Up and Out: How Valkey Does Both", "Sala Abasto"},
    {HM(11, 40), HM(12, 20), "Interview with SJVN", "Auditorio"},
    {HM(12, 10), HM(12, 50), "What's new in AI Audio?", "Gran sala"},
    {HM(12, 15), HM(13, 15), "Almuerzo / Lunch break", "Sala Abasto"},
    {HM(12, 20), HM(13, 0), "El fin del Scraping libre", "Auditorio"},
    {HM(12, 50), HM(13, 45), "Almuerzo / Lunch break", "Gran sala"},
    {HM(13, 0), HM(14, 0), "Almuerzo / Lunch break", "Auditorio"},
    {HM(13, 15), HM(13, 55), "Lo que Wireshark no te muestra", "Sala Abasto"},
    {HM(13, 45), HM(14, 25), "Processing Terabytes of Data in JavaScript", "Gran sala"},
    {HM(13, 55), HM(14, 35), "Migrando 150 apps a OpenTelemetry", "Sala Abasto"},
    {HM(14, 0), HM(14, 40), "La evolucion de JavaScript", "Auditorio"},
    {HM(14, 25), HM(15, 5), "Side projects que por fin hice deploy", "Gran sala"},
    {HM(14, 35), HM(15, 15), "SQL Traffic Routing Without Breaking Consistency", "Sala Abasto"},
    {HM(14, 40), HM(15, 20), "Kubernetes: de orchestration a AI", "Auditorio"},
    {HM(15, 5), HM(15, 45), "La IA no funciona sin modelo de dominio", "Gran sala"},
    {HM(15, 15), HM(15, 55), "Unikernels: final de los contenedores?", "Sala Abasto"},
    {HM(15, 20), HM(16, 0), "Tecnologia aeroespacial con software libre", "Auditorio"},
    {HM(15, 45), HM(16, 25), "Death to the textarea", "Gran sala"},
    {HM(15, 55), HM(16, 35), "Your SSH Keys Are Already Stale", "Sala Abasto"},
    {HM(16, 0), HM(16, 40), "Por que open source lanza carreras", "Auditorio"},
    {HM(16, 25), HM(17, 5), "From VLC to AI: Open Source changing the world", "Gran sala"},
    {HM(16, 35), HM(17, 15), "Millones mirando el mismo partido", "Sala Abasto"},
    {HM(16, 40), HM(17, 20), "Robin Bender Ginn at Nerdearla", "Auditorio"},
    {HM(17, 5), HM(17, 45), "Estamos mergeando codigo que nadie entiende", "Gran sala"},
    {HM(17, 15), HM(17, 55), "It's the Data, Stupid", "Sala Abasto"},
    {HM(17, 20), HM(18, 0), "Building Open Source Teams", "Auditorio"},
    {HM(17, 45), HM(18, 25), "Future of Building with AI: Multimodal Agents", "Gran sala"},
    {HM(17, 55), HM(18, 35), "Secretos y Kubernetes", "Sala Abasto"},
    {HM(18, 0), HM(18, 40), "Open Source AI: weights, data y licencias", "Auditorio"},
    {HM(18, 25), HM(19, 5), "An AI That You Should Be Using More", "Gran sala"},
    {HM(19, 10), HM(20, 0), "El futuro para los devs", "Gran sala"},
};

const size_t kScheduleCount = sizeof(kSchedule) / sizeof(kSchedule[0]);

ScheduleSelection selectScheduleEvent(uint16_t nowMinutes, uint32_t rotationSeed) {
  uint8_t candidates[12];
  uint8_t count = 0;

  for (size_t i = 0; i < kScheduleCount && count < sizeof(candidates); ++i) {
    if (kSchedule[i].startMinutes <= nowMinutes && nowMinutes < kSchedule[i].endMinutes) {
      candidates[count++] = static_cast<uint8_t>(i);
    }
  }

  const bool isCurrent = count > 0;
  if (!isCurrent) {
    uint16_t nextStart = 0xFFFF;
    for (size_t i = 0; i < kScheduleCount; ++i) {
      if (kSchedule[i].startMinutes > nowMinutes && kSchedule[i].startMinutes < nextStart) {
        nextStart = kSchedule[i].startMinutes;
      }
    }
    if (nextStart != 0xFFFF) {
      for (size_t i = 0; i < kScheduleCount && count < sizeof(candidates); ++i) {
        if (kSchedule[i].startMinutes == nextStart) {
          candidates[count++] = static_cast<uint8_t>(i);
        }
      }
    }
  }

  if (count == 0) {
    return {false, false, 0, 0, nullptr};
  }

  const uint8_t selected = static_cast<uint8_t>(rotationSeed % count);
  return {true, isCurrent, count, selected, &kSchedule[candidates[selected]]};
}
