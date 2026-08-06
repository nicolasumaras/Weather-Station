#pragma once

#include <Arduino.h>

#include <cstdarg>

// In-memory ring of recent gateway exchanges: inbound webhook deliveries and
// the outbound REST calls we make in reply. RAM only — nothing is written to
// flash, so there is no wear cost per request and the log resets on reboot.
//
// Written from two tasks (async_tcp for inbound, the Arduino loop task for
// outbound), so the ring update runs inside a critical section. Formatting is
// done first, outside the lock, to keep that section short.

static constexpr size_t MESH_LOG_SLOTS = 50;

struct MeshEvent {
  uint32_t unixTs;  // 0 until NTP has synced
  uint32_t uptimeS;
  char dir;      // '<' delivery to us, '>' call we made
  int16_t code;  // HTTP status, 0 when not applicable
  char what[28];
  char detail[64];
};

class MeshEventLog {
 public:
  void add(char dir, const char* what, int code, const char* fmt, ...);

  size_t size() const { return count_; }
  uint32_t dropped() const { return dropped_; }

  // Copies one entry, 0 being the newest. Returns false when out of range.
  bool get(size_t indexFromNewest, MeshEvent* out) const;

  void clear();

 private:
  MeshEvent ring_[MESH_LOG_SLOTS]{};
  size_t head_ = 0;   // next write position
  size_t count_ = 0;  // entries held, saturates at MESH_LOG_SLOTS
  uint32_t dropped_ = 0;
};

extern MeshEventLog meshLog;
