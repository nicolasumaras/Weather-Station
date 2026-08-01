#include "anemometer.h"

#include <Arduino.h>

#include "../config.h"

AnemometerSensor* AnemometerSensor::instance_ = nullptr;

void IRAM_ATTR AnemometerSensor::onPulse() {
  if (instance_) {
    instance_->pulseCount_++;
  }
}

void AnemometerSensor::begin(int pin) {
  pin_ = pin;
  instance_ = this;
  pinMode(pin_, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin_), onPulse, FALLING);
  lastMs_ = millis();
  lastCount_ = 0;
  pulseCount_ = 0;
}

void AnemometerSensor::setFactor(float mpsPerPulsePerSec) {
  if (mpsPerPulsePerSec > 0.0f) {
    factor_ = mpsPerPulsePerSec;
  }
}

void AnemometerSensor::update(SensorReadings& out) {
  uint32_t now = millis();
  uint32_t elapsed = now - lastMs_;
  if (elapsed < WIND_SAMPLE_MS) {
    return;
  }

  noInterrupts();
  uint32_t count = pulseCount_;
  interrupts();

  uint32_t delta = count - lastCount_;
  lastCount_ = count;
  lastMs_ = now;

  float seconds = elapsed / 1000.0f;
  float pps = (seconds > 0.0f) ? (delta / seconds) : 0.0f;
  out.windPulsesPerSec = static_cast<uint32_t>(pps + 0.5f);
  out.windMps = pps * factor_;
  out.windKmh = out.windMps * 3.6f;
  out.windValid = true;
}
