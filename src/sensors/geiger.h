#pragma once

#include "readings.h"

class GeigerSensor {
 public:
  void begin(int pin);
  void setFactor(float usvPerCpm);
  void update(SensorReadings& out);

 private:
  int pin_ = -1;
  float factor_ = 0.0057f;
  volatile uint32_t pulseCount_ = 0;
  uint32_t windowStartMs_ = 0;
  uint32_t windowStartCount_ = 0;

  static GeigerSensor* instance_;
  static void IRAM_ATTR onPulse();
};
