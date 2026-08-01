#include "ssap10.h"

#include "../config.h"

void Ssap10Sensor::begin(int rxPin, int txPin) {
  serial_.begin(SSAP10_BAUD, SERIAL_8N1, rxPin, txPin);
  idx_ = 0;
}

bool Ssap10Sensor::parseFrame(const uint8_t* frame, SensorReadings& out) {
  if (frame[0] != 0x42 || frame[1] != 0x4D) {
    return false;
  }

  uint16_t sum = 0;
  for (int i = 0; i < 30; ++i) {
    sum += frame[i];
  }
  uint16_t checksum = (static_cast<uint16_t>(frame[30]) << 8) | frame[31];
  if (sum != checksum) {
    return false;
  }

  // Atmospheric environment concentrations (bytes 10-15): PM1.0, PM2.5, PM10
  out.pm1_0 = static_cast<float>((frame[10] << 8) | frame[11]);
  out.pm2_5 = static_cast<float>((frame[12] << 8) | frame[13]);
  out.pm10 = static_cast<float>((frame[14] << 8) | frame[15]);
  out.pmValid = true;
  return true;
}

void Ssap10Sensor::update(SensorReadings& out) {
  while (serial_.available()) {
    uint8_t b = static_cast<uint8_t>(serial_.read());
    if (idx_ == 0 && b != 0x42) {
      continue;
    }
    if (idx_ == 1 && b != 0x4D) {
      idx_ = (b == 0x42) ? 1 : 0;
      if (idx_ == 1) {
        buf_[0] = 0x42;
      }
      continue;
    }
    buf_[idx_++] = b;
    if (idx_ >= 32) {
      parseFrame(buf_, out);
      idx_ = 0;
    }
  }
}
