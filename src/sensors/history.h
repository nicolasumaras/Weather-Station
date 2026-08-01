#pragma once

#include "readings.h"
#include "../config.h"

class HistoryBuffer {
 public:
  void maybeSample(const SensorReadings& r);
  size_t size() const { return count_; }
  size_t capacity() const { return HISTORY_CAPACITY; }
  // Returns number of samples written to out (newest last).
  size_t copyChronological(HistorySample* out, size_t maxOut) const;

 private:
  HistorySample samples_[HISTORY_CAPACITY];
  size_t head_ = 0;
  size_t count_ = 0;
  uint32_t lastSampleMs_ = 0;
};
