#pragma once

#include "readings.h"

class AnemometerSensor {
 public:
  void begin(int pin);
  void setFactor(float mpsPerPulsePerSec);
  void update(SensorReadings& out);

 private:
  int pin_ = -1;
  float factor_ = 0.0875f;
  volatile uint32_t pulseCount_ = 0;
  uint32_t lastMs_ = 0;
  uint32_t lastCount_ = 0;

  static AnemometerSensor* instance_;
  static void IRAM_ATTR onPulse();
};
