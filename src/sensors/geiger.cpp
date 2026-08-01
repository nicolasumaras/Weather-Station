#include "geiger.h"

#include <Arduino.h>

#include "../config.h"

GeigerSensor* GeigerSensor::instance_ = nullptr;

void IRAM_ATTR GeigerSensor::onPulse() {
  if (instance_) {
    instance_->pulseCount_++;
  }
}

void GeigerSensor::begin(int pin) {
  pin_ = pin;
  instance_ = this;
  pinMode(pin_, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin_), onPulse, FALLING);
  windowStartMs_ = millis();
  windowStartCount_ = 0;
  pulseCount_ = 0;
}

void GeigerSensor::setFactor(float usvPerCpm) {
  if (usvPerCpm > 0.0f) {
    factor_ = usvPerCpm;
  }
}

void GeigerSensor::update(SensorReadings& out) {
  uint32_t now = millis();
  uint32_t elapsed = now - windowStartMs_;
  if (elapsed < GEIGER_WINDOW_MS) {
    return;
  }

  noInterrupts();
  uint32_t count = pulseCount_;
  interrupts();

  uint32_t delta = count - windowStartCount_;
  windowStartCount_ = count;
  windowStartMs_ = now;

  float minutes = elapsed / 60000.0f;
  float cpm = (minutes > 0.0f) ? (delta / minutes) : 0.0f;
  out.cpm = cpm;
  out.usvPerHour = cpm * factor_;
  out.geigerValid = true;
}
