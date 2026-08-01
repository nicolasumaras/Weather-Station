#include "history.h"

#include <ctime>

void HistoryBuffer::maybeSample(const SensorReadings& r) {
  uint32_t nowMs = millis();
  if (lastSampleMs_ != 0 && (nowMs - lastSampleMs_) < HISTORY_SAMPLE_MS) {
    return;
  }
  lastSampleMs_ = nowMs;

  HistorySample s;
  if (r.ntpSynced && r.unixTime > 0) {
    s.ts = r.unixTime;
    s.unix = true;
  } else {
    s.ts = nowMs / 1000;
    s.unix = false;
  }
  s.pm2_5 = r.pmValid ? r.pm2_5 : NAN;
  s.windMps = r.windValid ? r.windMps : NAN;
  s.usvPerHour = r.geigerValid ? r.usvPerHour : NAN;
  s.pressureHpa = r.bmeValid ? r.pressureHpa : NAN;
  s.temperatureC = r.bmeValid ? r.temperatureC : NAN;
  s.humidityPct = r.bmeValid ? r.humidityPct : NAN;

  samples_[head_] = s;
  head_ = (head_ + 1) % HISTORY_CAPACITY;
  if (count_ < HISTORY_CAPACITY) {
    count_++;
  }
}

size_t HistoryBuffer::copyChronological(HistorySample* out, size_t maxOut) const {
  size_t n = count_ < maxOut ? count_ : maxOut;
  size_t start = (head_ + HISTORY_CAPACITY - count_) % HISTORY_CAPACITY;
  if (count_ > maxOut) {
    start = (head_ + HISTORY_CAPACITY - n) % HISTORY_CAPACITY;
  }
  for (size_t i = 0; i < n; ++i) {
    out[i] = samples_[(start + i) % HISTORY_CAPACITY];
  }
  return n;
}
