#pragma once

#include "readings.h"

class Bme280Sensor {
 public:
  bool begin(int sda, int scl);
  void update(SensorReadings& out);

 private:
  bool ready_ = false;
};
