#include "bme280_sensor.h"

#include <Adafruit_BME280.h>
#include <Wire.h>

namespace {
Adafruit_BME280 bme;
}

bool Bme280Sensor::begin(int sda, int scl) {
  Wire.begin(sda, scl);
  ready_ = bme.begin(0x76, &Wire);
  if (!ready_) {
    ready_ = bme.begin(0x77, &Wire);
  }
  if (ready_) {
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X2,
                    Adafruit_BME280::SAMPLING_X16,
                    Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::FILTER_X16,
                    Adafruit_BME280::STANDBY_MS_500);
  }
  return ready_;
}

void Bme280Sensor::update(SensorReadings& out) {
  if (!ready_) {
    out.bmeValid = false;
    return;
  }
  out.temperatureC = bme.readTemperature();
  out.humidityPct = bme.readHumidity();
  out.pressureHpa = bme.readPressure() / 100.0f;
  out.bmeValid = !isnan(out.temperatureC) && !isnan(out.pressureHpa);
}
