#pragma once

#include <HardwareSerial.h>
#include "readings.h"

class Ssap10Sensor {
 public:
  void begin(int rxPin, int txPin);
  void update(SensorReadings& out);

 private:
  HardwareSerial serial_{1};
  uint8_t buf_[32];
  size_t idx_ = 0;
  bool parseFrame(const uint8_t* frame, SensorReadings& out);
};
