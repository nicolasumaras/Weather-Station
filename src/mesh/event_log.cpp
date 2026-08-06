#include "event_log.h"

#include <ctime>
#include <cstring>

MeshEventLog meshLog;

namespace {
portMUX_TYPE logMux = portMUX_INITIALIZER_UNLOCKED;
}

void MeshEventLog::add(char dir, const char* what, int code, const char* fmt, ...) {
  // Build the entry on the caller's stack first: vsnprintf must not run inside
  // the critical section, and this keeps the lock to a handful of copies.
  MeshEvent e{};
  const time_t now = time(nullptr);
  e.unixTs = (now > 1600000000L) ? static_cast<uint32_t>(now) : 0;
  e.uptimeS = millis() / 1000;
  e.dir = dir;
  e.code = static_cast<int16_t>(code);
  strncpy(e.what, what ? what : "", sizeof(e.what) - 1);
  e.what[sizeof(e.what) - 1] = '\0';

  if (fmt) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(e.detail, sizeof(e.detail), fmt, args);
    va_end(args);
  }

  portENTER_CRITICAL(&logMux);
  ring_[head_] = e;
  head_ = (head_ + 1) % MESH_LOG_SLOTS;
  if (count_ < MESH_LOG_SLOTS) {
    count_++;
  } else {
    dropped_++;  // oldest entry just fell off the end
  }
  portEXIT_CRITICAL(&logMux);
}

bool MeshEventLog::get(size_t indexFromNewest, MeshEvent* out) const {
  bool ok = false;
  portENTER_CRITICAL(&logMux);
  if (indexFromNewest < count_) {
    // head_ points at the next write slot, so the newest is head_ - 1.
    const size_t pos = (head_ + MESH_LOG_SLOTS - 1 - indexFromNewest) % MESH_LOG_SLOTS;
    *out = ring_[pos];
    ok = true;
  }
  portEXIT_CRITICAL(&logMux);
  return ok;
}

void MeshEventLog::clear() {
  portENTER_CRITICAL(&logMux);
  head_ = 0;
  count_ = 0;
  dropped_ = 0;
  portEXIT_CRITICAL(&logMux);
}
